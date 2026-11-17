# Portable editor formatting and screenshot-card polish

Branch: `feature/editor-format-toolbar`.

## Behavior

- A compact seven-button bar below the note title: heading, bold, italic, underline, bullet list, callout, clear formatting. All icons use the same canvas and accent color; hover retains the editor selection.
- Full Markdown highlighting and Focus Mode default to off. Existing saved width preferences are respected. Basic toolbar formatting remains visual when the optional Markdown switch is off.
- Basic formatting delimiters remain in the portable note source but are hidden in the editor. Inline/fenced code preserves literal markers.
- Lists continue on Enter; an empty item exits the list. Inside a callout, list/heading actions preserve its boundary and Ctrl+Enter continues the block or its list. Plain Enter exits the callout and leaves a blank separator before the next text. Empty list items on Ctrl+Enter return to normal callout text.
- New callouts focus their body. Their thin frame has space above, below and to either side, including wrapped lines. Its upper edge stays in a reserved hidden header row; its lower edge avoids adjacent nonempty paragraphs in older notes. A mouse-transparent overlay paints the frame outside the text viewport without adding undoable layout changes.
- Clearing current-paragraph formatting preserves neighboring paragraphs and the enclosing callout. Each toolbar action can be undone.
- Opening the photo requires the dedicated icon at the right of the thumbnail; the rest of the thumbnail has no open action.
- Cards moved or pasted into callouts retain the quote boundary and surrounding formatting, including source roundtrips and Undo.
- Screenshot frames align with note text. The drag grip is inside the thumbnail; a blue line indicates the destination.
- Screenshot automatic titles use the first nonempty description line; an explicit title takes precedence. Long descriptions retain all text. Pasting into the inline field keeps the insertion cursor at the end but resets the visible scroll position to the beginning.
- Four screenshot actions use a common 14px outline style and 24px hover targets: show original in Explorer, copy original image, copy full description, delete. They do not increase card height. Explorer selects the original attachment and missing files show a nonmodal error.

## Storage and deployment

No SQLite schema change, dependency update, remote publication or user-data migration. Release uses `PRO_VERSION=ON`, `UPDATE_CHECKER=OFF`, `NOTES_PORTABLE=ON`.

Build: `D:\01_PROJECTS\NOTES_ZHKVSKY\build-notes-headings-release`.
Separate candidate: `D:\01_PROJECTS\NOTES_ZHKVSKY\RELEASE\Notes_ZHKVSKY_Formatting`.
Primary installation: `C:\RELEASE\Notes_ZHKVSKY_Portable`. Updates replace program files only; Data and Settings are preserved. Back up the complete installation before replacing its executable.

## Validation

Native screenshot integration suite: `TEST/photo-callout-spacing`, all main, process-restart and storage checks pass. Includes actual text/layout formats with Markdown off, undo, callout cursor/list/heading behavior, long-description paste, full-resolution clipboard copy and missing-original handling.

Standalone Notes.exe was launched with isolated synthetic data. The dedicated photo icon opened its original PNG in ACDSee Quick View. Full general UI checks are not all green: two legacy tag-hover checks failed in `TEST/formatting-final-regression-j3`; that run passed heading, Kanban, data and other checks. The dedicated heading/Kanban process restart check subsequently returned exit code 0. These unrelated hover failures are not reported as passed. Testing on a separate clean Windows machine is unavailable. External editor clipboard checks from the previous screenshot-block stage are not repeated for this formatting change.
