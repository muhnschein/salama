# 0025 — Downloads go to a folder of their own

## Context
Where a download is saved is decided by embedlite-components'
`jscomps/HelperAppDialog.js`. With `browser.download.useDownloadDir` true, it saves to
`browser.download.dir` without asking, if that folder exists and can be written to; if not,
it saves to Gecko's preferred folder, `~/Downloads`. With the preference false, it sends
`embed:downloadpicker`, and the platform `WebView` pushes its "Download to" folder picker
(sailfish-components-webview `import/pickers/DownloadPicker.qml`) for every download.
Nothing sets either preference for a `Sailfish.WebView` application:
sailfish-components-webview's `WebEngineSettings::initialize()` sets neither, qtmozembed's
`QMozEngineSettings` starts `useDownloadDir` at false, and the dialog reads a missing one as
false. So every download asked where to go, and a folder had to be chosen each time.

## Decision
Downloads go to `~/Downloads/Salama` without asking. `Storage::defaultDownloadDirectory()`
names the folder, next to the application's other standard locations, and `main()` hands it
to `Core`, which hands it to `DownloadModel`. The model creates the folder, and any missing
parents, when it is constructed. The dialog does not create it: it only checks whether the
folder is there. `DownloadModel.directory` exposes the folder to QML, and the browsing page
sets `WebEngineSettings.downloadDir` to it and `useDownloadDir` to true, next to where it
sets the zoom (0012). Tests pass `Core` a folder in a temporary directory, so they never
create one in the real `~/Downloads`.

The folder is named `Salama`, not the application's name (`salama`). People read and browse
it in the file manager, and there it is written the way the cover writes the name.

## Consequences
No folder picker comes up any more, and there is no setting to choose another folder.
Choosing a folder for each download, or once in Settings, would be a new decision.

The folder is created even before anything has been downloaded, and it is created again at
the next start if it was deleted. If it is deleted while the application is running,
downloads land in `~/Downloads` until the next start. The `Downloads` permission already
covers the folder: Sailjail's `Downloads.permission` whitelists all of `~/Downloads`
(`docs/HARBOUR.md`).

The preferences are the engine's, and it keeps them in its profile. So a build without this
change, run on a profile this build has already used, still saves to `~/Downloads/Salama`
without asking, as long as the folder is there.
