#!/usr/bin/env bash
# Print SonarQube Cloud's verdict on an analysis into job log and step summary.
# Scanner uploads and exits; server processes later; results sit behind login.
# This asks server from runner and prints gate, ratings, issues beside commit.
#
# Reports, never gates: ci.yml decides. Step is continue-on-error; Sonar outage = warning.
#
# Usage:  ci/sonar-report.sh [path/to/report-task.txt]
#
# Scanner writes that file: server, project, task, branch/PR. Makes stub testing
# possible (ci/sonar-report-selftest.sh). From postivene and vuo.
set -euo pipefail

# No `${1:-default}` or `${VAR:+...}`: SonarQube's shell analyser can't parse them
# ("Syntax error at 121:63") and skips whole file.
if [[ "$#" -ge 1 ]]; then
    TASK_FILE=$1
else
    TASK_FILE=.scannerwork/report-task.txt
fi

# printenv not ${VAR:-} (same reason); `|| true` since `set -u` kills on absent var.
SONAR_TOKEN=$(printenv SONAR_TOKEN || true)
SUMMARY=$(printenv GITHUB_STEP_SUMMARY || true)

# Issues to list. 500 = API page max. 100 once truncated 115 issues.
PAGE=500

if [[ ! -f "$TASK_FILE" ]]; then
    echo "no $TASK_FILE -- the scanner did not get as far as uploading" >&2
    exit 1
fi

field() {
    local key=$1
    sed -n "s|^$key=||p" "$TASK_FILE" | head -1
}

SERVER=$(field serverUrl)
KEY=$(field projectKey)
TASK_URL=$(field ceTaskUrl)
DASHBOARD=$(field dashboardUrl)

# Analysed slice, from scanner's dashboard URL, so can't disagree with upload.
SCOPE=$(printf '%s' "$DASHBOARD" | sed -n 's/.*[?&]\(pullRequest=[^&]*\).*/\1/p')
if [[ -z "$SCOPE" ]]; then
    SCOPE=$(printf '%s' "$DASHBOARD" | sed -n 's/.*[?&]\(branch=[^&]*\).*/\1/p')
fi
SCOPE_Q=""
SCOPE_LABEL="default branch"
if [[ -n "$SCOPE" ]]; then
    SCOPE_Q="$SCOPE&"
    SCOPE_LABEL=$SCOPE
fi

# Response bodies land here; nothing kept in shell vars.
BODY=$(mktemp)

# GET, token first. Cloud answers 404 (not 403) when unauthorised; anonymous-first
# once read that as "missing" and reported nothing. Scanner uses token for same
# endpoints (`sonar.qualitygate.wait`). Anonymous = fallback for public project
# where token lacks browse rights.
code=""
fetch() {
    local url=$1
    local auth=$2
    if [[ -n "$auth" ]]; then
        code=$(curl -sS --max-time 30 -u "$auth:" -o "$BODY" -w '%{http_code}' "$url") || return 1
    else
        code=$(curl -sS --max-time 30 -o "$BODY" -w '%{http_code}' "$url") || return 1
    fi
    [[ "$code" = "200" ]]
}

api() {
    local url=$1
    if [[ -n "$SONAR_TOKEN" ]] && fetch "$url" "$SONAR_TOKEN"; then
        return 0
    fi
    if fetch "$url" ""; then
        return 0
    fi
    echo "  cannot read $url (HTTP $code)" >&2
    return 1
}

# Rating 1..5 on wire, A..E for people.
letter() {
    local rating=$1
    echo "$rating" | sed 's/^1.*/A/; s/^2.*/B/; s/^3.*/C/; s/^4.*/D/; s/^5.*/E/'
}

# ------------------------------------------------------------ wait for it
#
# Async. Measures before processing ends = PREVIOUS run's numbers, which look right.
status=""
analysis=""
misses=0
for _ in $(seq 60); do
    if api "$TASK_URL"; then
        misses=0
        status=$(jq -r '.task.status // "?"' "$BODY")
        if [[ "$status" = "SUCCESS" ]]; then
            analysis=$(jq -r '.task.analysisId // ""' "$BODY")
            break
        fi
        if [[ "$status" = "FAILED" ]] || [[ "$status" = "CANCELED" ]]; then
            break
        fi
    else
        # Task briefly invisible after upload: one bad answer no verdict, but permanent
        # failure must not burn five minutes.
        misses=$((misses + 1))
        if [[ "$misses" -ge 5 ]]; then
            echo "the compute task cannot be read; giving up" >&2
            break
        fi
    fi
    sleep 5
done

if [[ "$status" != "SUCCESS" ]]; then
    if [[ -z "$status" ]]; then
        status=unknown
    fi
    echo "the server did not finish processing the report (status: $status)" >&2
    exit 1
fi

out=$(mktemp)
trap 'rm -f "$out" "$BODY"' EXIT

{
    echo "## SonarQube Cloud"
    echo
    echo "\`$KEY\` — $SCOPE_LABEL"
    echo
} >"$out"

# ------------------------------------------------------------ quality gate
if api "$SERVER/api/qualitygates/project_status?analysisId=$analysis"; then
    # Parentheses needed: `|` binds looser than `,` in jq; else pipe hits heading
    # strings -> "Cannot index string with string".
    jq -r '
        "### Quality gate: \(.projectStatus.status)", "",
        ((.projectStatus.conditions // [])[]
         | "- \(.status)  \(.metricKey) \(.comparator) \(.errorThreshold) (actual: \(.actualValue // "none"))")
    ' "$BODY" >>"$out"
    echo >>"$out"
fi

# ------------------------------------------------------------ measures
# new_lines_to_cover gives new_coverage context: 0.0% of one line = unreachable file,
# not untested change.
metrics=ncloc,coverage,line_coverage,duplicated_lines_density,violations,security_hotspots,security_rating,reliability_rating,sqale_rating,new_coverage,new_lines_to_cover,new_violations
if api "$SERVER/api/measures/component?component=$KEY&${SCOPE_Q}metricKeys=$metrics"; then
    {
        echo "### Measures"
        echo
        # New-code value under `period` (Server) or `periods` array (Cloud). Read both.
        jq -r '
            (.component.measures // [])[]
            | "\(.metric)=\(.value // .period.value // (.periods // [])[0].value // "-")"
        ' "$BODY" | while IFS='=' read -r metric value; do
            case "$metric" in
                *_rating) echo "- $metric: $(letter "$value")" ;;
                *)        echo "- $metric: $value" ;;
            esac
        done
        echo
    } >>"$out"
fi

# ------------------------------------------------------------ issues
if api "$SERVER/api/issues/search?componentKeys=$KEY&${SCOPE_Q}resolved=false&ps=$PAGE"; then
    total=$(jq -r '.total // 0' "$BODY")
    {
        echo "### Open issues: $total"
        echo
        if [[ "$total" = "0" ]]; then
            echo "None."
        else
            echo '```'
            jq -r '
                (.issues // [])[]
                | "\(.severity // (.impacts[0].severity? // "?"))  \(.rule)  \(.component | sub("^[^:]*:";""))\(if .line then ":\(.line)" else "" end)  \(.message)"
            ' "$BODY"
            if [[ "$total" -gt "$PAGE" ]]; then
                echo "... $((total - PAGE)) more not listed"
            fi
            echo '```'
        fi
        echo
    } >>"$out"
fi

echo "$DASHBOARD" >>"$out"

cat "$out"
if [[ -n "$SUMMARY" ]]; then
    cat "$out" >>"$SUMMARY"
fi
