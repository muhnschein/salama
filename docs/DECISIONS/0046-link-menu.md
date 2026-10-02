# 0046 — A press held on a link brings up the link sheet

## Context
Issue #31: a press held on a link did nothing, so a link could not be opened in a new tab,
copied, shared or saved, and a picture neither. The engine does tell of every such press:
embedlite-components' `jsscripts/ContextMenuHandler.js` sends `Content:ContextMenu` with
what is under the finger — `types` ("link", "image", "content-text", …), `linkURL`,
`linkTitle`, `linkProtocol`, `mediaURL`, `contentType`. The platform's WebView listens for
it (`import/webview/WebView.qml` in sailfish-components-webview) and its popup opener makes
a menu of its own from the view's popup provider (`import/popups/PopupOpener.qml`,
`ContextMenu.qml`): a full-screen list whose "Open link in a new tab" needs a tab model of
the platform browser's kind, and whose "Open link" hands the link to the system's browser.
Salama never saw that menu; why on the device is not known, and nothing here relies on it.

sailfish-browser draws its own. The design was settled on a canvas with the person who
asked: the menu's sheet (0021) rather than a text list like Silica's ContextMenu — the more
native look, but another object from the menu and taller for the same actions — with a
preview of the page as Safari has, and a picture lifted out of the page rather than
previewed.

## Decision
**The press is read here.** Each page's view carries `components/PageLinkMenu.qml`, which
hears the message, has `EngineMessages::linkTarget()` read it — the link unless it is a
`javascript:` one, the picture if it is an http or https one, its kind (a page, or another
application's: `mailto:`, `tel:`, `sms:`, `geo:`), the link's text as one line, and what
to show of the address — and hands it on when there is a link or a picture. Only the page
in front's press opens anything. Text is left to the platform, which selects it on the same
message. The view's popup provider is told to make `PlatformMenuStandIn.qml` for the
platform's menu: it takes what the opener sets and shows nothing, so the two can never come
up together.

**The sheet is the menu's** (`components/LinkMenu.qml`): a Silica `DockedPanel` from the
foot, modal, on `SheetBackground`, pulled down as the menu is. Its head names what was
pressed as the menu's head names the page — the picture, the application's icon or the
host's initial on a faint tile, the link's text, the host and path, the mailbox or the
number — with the menu's clipboard button. Under it the actions on discs
(`LinkActions.qml`): for a page, New tab, Background tab, Share, Save link; for another
application's link, that application's (Write email, Call, Send message, Show on map) and
Share; after the menu's fading line, for a picture, Open image, Save image, Copy image link.
Each does what it says and puts the sheet away.

- **New tab** opens the link in front, after a picture of the page left, as the omnibar's
  new tab does. **Background tab** is `TabModel::newTabBehind()`: a tab in the current
  group, named by the link's text, with no view until it first comes to the front, as a
  restored tab has none (0003). A banner on the bar says "Opened in a new tab", the name and
  the group when it has one, with Show; it goes after four seconds.
- **Save link** and **Save image** are `DownloadModel::save()`: the engine's `addDownload`
  into the downloads folder (0025), under the name the address ends in — never one made up —
  with the type's ending added to a name that has none, and "(1)" before the ending, as
  Firefox numbers them, rather than over a file or a download that has the name. Only a
  name the file system would refuse is cut.
- **Copy** puts a link on the clipboard as it is, another application's address without its
  scheme, and a picture's address; a Notice says so.

**The preview.** For a link to a page, a row under the head says Hide preview or Show
preview, and saying it is `Settings.linkPreview`, on unless switched off, as Safari's is:
it holds for every link after. While it is on, the page the link leads to is drawn in a
frame, in a `WebView` the browsing page makes (it alone imports `Sailfish.WebView`); a tap
on it opens the link where the page was. The engine draws all its views into one picture
(qtmozembed's `QMozWindow`: every `QuickMozView` draws its platform image), so the page in
front cannot be drawn beside it: the sheet takes a still of the page first, lays it where
the page was, and the page's view is made inactive and hidden until the sheet goes. Hiding
a page pauses what it plays, so no preview is offered while the page in front plays, and
none for a picture. A page that cannot be pictured is not put aside.

**A picture** is lifted out of the dimmed page into the room above the sheet, as wide as
the screen or as tall as there is room for, and pinched closer with two fingers
(`LinkMenuOverlay.qml`, laid over the page so the sheet's slide does not carry it).

No file of it needs the 400-line waiver: the sheet is `LinkMenu`, `LinkMenuHeader`,
`LinkPreview`, `LinkActions` and `LinkMenuOverlay`.

## Consequences
Every long press on a link or a picture has its actions, in the menu's look.

The pressed link is not lit on the page: the message carries where the finger was, not the
link's box, and the page is the engine's to draw.

The preview and the lifted picture are drawn from their addresses by the preview's own view
and by Qt: a picture behind a login may not load for Qt, and then is not lifted. Whether
the engine draws the preview's view where it is asked, and the page again when it comes
back, is on the device checklist (`docs/TESTING.md`); so is the press reaching the
application at all, which a host cannot show.
