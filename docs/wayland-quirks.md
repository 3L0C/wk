# Wayland Quirks

This document describes behavior differences across Wayland
compositors. Please open an issue or PR for any corrections
or additions.

## Pointer Behavior

`wk` is keyboard-driven. Pointer events (clicks, scrolls)
close the menu rather than selecting items.

However, compositors differ in *which* pointer events reach
`wk`:

| Compositor | Clicking outside menu | Scrolling outside menu |
|------------|-----------------------|------------------------|
| Hyprland   | Closes menu           | Closes menu            |
| Niri       | No effect             | No effect              |
| KDE        | Closes menu           | Inconsistent           |

KDE's scroll behavior seems to vary by application. See
[#7](https://github.com/3L0C/wk/issues/7#issuecomment-3691699710)
for details.
