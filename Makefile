# harbour-salama developer entry points.
# `make check` = CI: clean checkout, no phone, no SDK, no network. Targets usable alone.

BUILD ?= build
JOBS ?= $(shell nproc 2>/dev/null || echo 2)
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=Debug -DSALAMA_COVERAGE=ON
COVERAGE_MIN ?= 80

CXX_SOURCES := $(shell find src tests -name '*.cpp' -o -name '*.h' | sort)
TS_FILES := $(sort $(wildcard translations/harbour-salama*.ts))

.PHONY: all configure build test coverage fmt fmt-apply tidy qml-lint packaging-lint \
        harbour-check harbour-selftest sonar-selftest sonar-reports lint check \
        translations clean

all: build

configure: $(BUILD)/CMakeCache.txt

$(BUILD)/CMakeCache.txt: CMakeLists.txt src/CMakeLists.txt tests/CMakeLists.txt translations/CMakeLists.txt
	cmake -S . -B $(BUILD) $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD) -j $(JOBS)

# One process per test, serial, no retries (second-attempt pass = defect).
test: build
	cd $(BUILD) && ctest --output-on-failure -j 1 --timeout 120

# Drop compiler-made exception branches no test can take (632 of 1732; dragged imported
# Sonar coverage down). Line coverage gate unaffected.
coverage: test
	mkdir -p $(BUILD)/coverage
	gcovr --root . --object-directory $(BUILD) \
	      --filter 'src/' --exclude 'src/main\.cpp' \
	      --exclude-throw-branches --exclude-unreachable-branches \
	      --print-summary --fail-under-line $(COVERAGE_MIN) \
	      --sonarqube $(BUILD)/coverage/sonar-coverage.xml \
	      --xml $(BUILD)/coverage/cobertura.xml \
	      --html-details $(BUILD)/coverage/index.html

fmt:
	clang-format --dry-run --Werror $(CXX_SOURCES)

fmt-apply:
	clang-format -i $(CXX_SOURCES)

tidy: configure
	ci/clang-tidy.sh $(BUILD)

qml-lint:
	ci/qml-lint.sh

packaging-lint:
	ci/packaging-lint.sh

harbour-check:
	ci/harbour-check.sh

harbour-selftest:
	ci/harbour-check-selftest.sh

sonar-selftest:
	ci/sonar-report-selftest.sh

# Gather what SonarQube imports: coverage report, compile DB. sonar-project.properties names
# both under build/; other BUILD needs that file changed. Not in `check`: Sonar is report.
sonar-reports: coverage
	@test -f $(BUILD)/compile_commands.json || { \
		echo "sonar-reports: no $(BUILD)/compile_commands.json" >&2; exit 1; }
	@test "$(BUILD)" = build || \
		echo "sonar-reports: BUILD is $(BUILD); sonar-project.properties names build/" >&2
	@echo "sonar-reports: $(BUILD)/coverage/sonar-coverage.xml and $(BUILD)/compile_commands.json"

lint: fmt qml-lint packaging-lint harbour-check harbour-selftest sonar-selftest

check: lint build test coverage tidy
	@echo "check: all gates green"

# Own sources only: else lupdate follows src/reader/reader.qrc into Readability (no strings
# of ours, syntax parser rejects).
LUPDATE_EXTENSIONS := cpp,h,qml

# Regenerate all catalogs from source in one run. New language: see docs/TRANSLATING.md.
translations:
	lupdate -no-obsolete -locations none -extensions $(LUPDATE_EXTENSIONS) qml src -ts $(TS_FILES)

clean:
	rm -rf $(BUILD)
