# Screenshot Blocks MVP — report

Implementation commit: 3f47390c629cddae4b2b39a825c9f490e3383b4b
Branch: feature/screenshot-blocks-mvp. Release / PRO_VERSION=ON / UPDATE_CHECKER=OFF / NOTES_PORTABLE=ON.

Inline objects serialize versioned UUID/JSON markers into the existing SQLite content column. Originals remain in Data/Attachments; thumbnails are lazy cached PNG files in Data/Thumbnails. Metadata and text commit together; no schema migration. Deletion retains files for Undo. Active edit widgets only; two image worker threads and a 32 MiB thumbnail cache.

Verified: {"screenshot-main-report.txt": 72, "screenshot-restart-report.txt": 3, "screenshot-storage-report.txt": 6, "regression": 82}; note restart/order, relative paths with Cyrillic/spaces, Markdown, editable Pro Kanban, copy image pixels/RGBA, full multiline description, no textChanged/autosave from copy, missing/corrupt image errors, retry after write failure, SQLite Recovery and image-aware backup.

Ordinary final Notes.exe launched and inspected. Paint paste: 3840x2160 selection. Notepad paste: full 83-line description. Default viewer opened 3840x2160 original in ACDSee.

Performance: plain 600-line note median open 31 ms before and after; 100 image cards about 29 ms open / 13 ms text insertion in latest native run; process working set about 282 MiB including 4K test/clipboard buffers. CPU and I/O samples are in clipboard-verified.log; these are local desktop measurements.

Limitations: Telegram paste was not verified; clean Windows was not available. Current Qt package has PNG/JPEG support, no WebP plugin. No automatic attachment garbage collection. Clipboard blocked beyond bounded retries gives an error.

Primary C:\RELEASE\Notes_ZHKVSKY_Portable was not updated. All test DB writes belonged to D: test folders. Primary Notes was running independently and its notes.db changed since the initial snapshot; no rollback or replacement was attempted. Backup: D:\01_PROJECTS\NOTES_ZHKVSKY\Backups\Notes_C_before_screenshot_blocks_20261117_180610.

Changed files:
- CMakeLists.txt
- docs/SCREENSHOT_BLOCKS_ARCHITECTURE.md
- docs/SCREENSHOT_BLOCKS_PORTABLE.md
- src/customDocument.h
- src/customdocument.cpp
- src/dbmanager.cpp
- src/isolatedtest.h
- src/mainwindow.cpp
- src/noteeditorlogic.cpp
- src/screenshotblocks.cpp
- src/screenshotblocks.h
- tests/plain_note_benchmark.cpp
- tests/run_ui_ux_v02.py
- tests/screenshot_blocks.cpp
