# FAQ

## Which Wayland compositors are supported?

wk requires compositors that implement the
[wlr-layer-shell](https://gitlab.freedesktop.org/wlroots/wlr-protocols/tree/master/unstable)
protocol. This typically means
[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots/)-based
compositors (Hyprland, Sway, dwl, river, etc.).

For compositors that don't support wlr-layer-shell,
[Xwayland](https://wayland.freedesktop.org/xserver.html) does
work, but the popup menu may only display on one screen.

## Does wk support mouse or touch input?

No. wk is keyboard-driven. Clicking, scrolling, or touching
the menu will close it rather than selecting an item.

Wayland compositors differ in which pointer events reach wk
when interacting *outside* the menu:

| Compositor | Clicking outside menu | Scrolling outside menu |
|------------|-----------------------|------------------------|
| Hyprland   | Closes menu           | Closes menu            |
| Niri       | No effect             | No effect              |
| KDE        | Closes menu           | Inconsistent           |

KDE's scroll behavior seems to vary by application. See issue
[#7](https://github.com/3L0C/wk/issues/7#issuecomment-3691699710)
for details.

## Should I use runtime or built-in configuration?

**Runtime** (`wk --key-chords file.wks`): fastest iteration.
Edit your wks file and re-run wk to see changes immediately.

**Built-in** (`make from-wks`): faster startup since key
chords are compiled into the binary. Best for stable configs.

Start with runtime configuration while developing your
layout, then switch to built-in once it stabilizes.

## Is there editor support for wks files?

There is a [wks-mode](https://github.com/3L0C/wks-mode)
package for Emacs that provides syntax highlighting and
indentation for wks files.

If you use a different editor and would like to see wks
support, please open an
[issue](https://github.com/3L0C/wk/issues).

## Where can I report bugs or request features?

Please open an issue at
[github.com/3L0C/wk/issues](https://github.com/3L0C/wk/issues).
