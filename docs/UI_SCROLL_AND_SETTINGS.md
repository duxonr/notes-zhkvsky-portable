# Muted dates, editor-settings geometry and scrollbars

Branch: `fix/popup-scrollbar-and-muted-dates`.
Primary application: `C:\RELEASE\Notes_ZHKVSKY_Portable\Notes.exe`.

- Note-card dates use the same foreground as preview text in both the static and tag-card delegates; titles remain brighter.
- Editor scrollbar visibility is controlled by ScrollBarAsNeeded. The explicit startup hide and the activity/timeout hide timer were removed, so overflow is visible immediately and remains available while idle.
- The shared QWidget vertical-scrollbar stylesheet defines the complete solid track and page regions, a 14 px hit area with a 10 px rounded thumb, and the interface accent #448ac9 on hover (#2f73b2 while pressed). Dark, light and sepia have explicit track/idle-thumb colors. Editor, note-list and folder-tree scrollbars receive the theme class.
- The shared QML scrollbar uses the same width/colors and no longer fades away solely because scrolling stopped.
- Editor-settings QML pane height binds to its content with an 80% parent-height limit. The native container follows root width/height changes instead of always occupying 80% of the main window. Windows uses a frameless container with an opaque theme-colored backing. Mac transparency/shadow behavior is retained.

All content, settings formats, portable paths, Pro features and update configuration remain unchanged. Build: Release, NOTES_PORTABLE=ON, PRO_VERSION=ON, UPDATE_CHECKER=OFF.

Native verification is in `tests/ui_ux_v02.cpp --scroll-popup-only`: immediate and idle scrollbar visibility, 14 px width, date versus preview/title luminance, solid track and blue-hover rendering in three themes, popup geometry at 500/1200/600 px main-window heights, and visible inactive QML scrollbars when content overflows. Geometry evidence: 400/742/480 px container heights exactly match QML and QQuickView heights. The standard integration suite additionally covers note persistence, tags, row actions, Markdown and Kanban Pro.

Initial full backup: `D:\01_PROJECTS\NOTES_ZHKVSKY\Backups\Notes_C_before_scroll_popup_20261117_174431`.
Installation excludes Data and Settings and compares their full file set and SHA-256 hashes. A fresh backup is required if the primary instance was reopened during verification.

No clean Windows installation is available for testing. The previously documented inherited folder-move path-formatting issue is outside this UI correction.
