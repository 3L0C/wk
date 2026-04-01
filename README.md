wk
========

wk is a popup menu for your custom keyboard shortcuts.
Inspired by
[emacs-which-key](https://github.com/justbur/emacs-which-key),
[dmenu](https://tools.suckless.org/dmenu/), and
[bemenu](https://github.com/Cloudef/bemenu).

[](https://github.com/user-attachments/assets/e80ae96f-0691-4c58-b979-a454ce66085d)

## Features

- **X11 and Wayland** - take your workflow wherever you go
- **Custom scripting language** - define key chords with
  [wks](https://3l0c-wk.readthedocs.io/en/latest/reference/wks.html), a
  purpose-built language designed to capture your simple and
  complex workflows
- **Themeable** - customize colors, fonts, borders, and
  padding
- **Flexible input** - compile key chords into the binary,
  load from a file, or pipe via stdin for dynamic scripting

## Documentation

Full documentation is available at
[3l0c-wk.readthedocs.io](https://3l0c-wk.readthedocs.io/).

- [Installation](https://3l0c-wk.readthedocs.io/en/latest/quick-start/installation.html)
- [Getting Started](https://3l0c-wk.readthedocs.io/en/latest/quick-start/getting-started.html)
- [wks Reference](https://3l0c-wk.readthedocs.io/en/latest/reference/wks.html)
- [CLI Reference](https://3l0c-wk.readthedocs.io/en/latest/reference/cli.html)
- [Examples](https://3l0c-wk.readthedocs.io/en/latest/examples/index.html)
- [FAQ](https://3l0c-wk.readthedocs.io/en/latest/faq/index.html)

## Video Demos

Prefer to watch a demonstration rather than read the docs?
Check out the video walkthrough series covering wk basics
and beyond
[here](https://youtube.com/playlist?list=PL20SXZFQEeVPi9vtnxYvMmvOfUalXBWMi&feature=shared).
You might want to watch at 1.5-2 times speed :).

## Installing from source

### Dependencies

Build dependencies include:

- C compiler
- make
- pkg-config
- wayland-scanner (if building with Wayland support)

| Backend | Dependencies                                 |
|---------|----------------------------------------------|
| Common  | cairo, pango, pangocairo                     |
| X11     | x11, xinerama                                |
| Wayland | wayland-client, wayland-protocols, xkbcommon |

<details>
<summary>Arch</summary>

```bash
sudo pacman -S base-devel cairo pango libx11 libxinerama wayland wayland-protocols libxkbcommon
```
</details>

<details>
<summary>Fedora</summary>

```bash
sudo dnf install gcc make pkg-config cairo-devel pango-devel libX11-devel libXinerama-devel wayland-devel wayland-protocols-devel libxkbcommon-devel
```
</details>

<details>
<summary>Ubuntu / Debian</summary>

```bash
sudo apt install build-essential pkg-config libcairo2-dev libpango1.0-dev libx11-dev libxinerama-dev libwayland-dev wayland-protocols libxkbcommon-dev
```
</details>

<details>
<summary>openSUSE</summary>

```bash
sudo zypper install gcc make pkg-config cairo-devel pango-devel libX11-devel libXinerama-devel wayland-devel wayland-protocols-devel libxkbcommon-devel
```
</details>

<details>
<summary>Void</summary>

```bash
sudo xbps-install -S base-devel cairo-devel pango-devel libX11-devel libXinerama-devel wayland-devel wayland-protocols libxkbcommon-devel
```
</details>

### Building

```bash
git clone "https://github.com//3L0C/wk.git"
cd wk
make && sudo make install
```

See the
[installation guide](https://3l0c-wk.readthedocs.io/en/latest/quick-start/installation.html)
for backend-specific builds, building with a custom wks
config, and Nix instructions.

## Contributing

Contributions are welcome! If you find any issues or have
suggestions for improvements, please open an issue or submit
a pull request.

## Acknowledgments

This project would not be where it is without
[dmenu](https://tools.suckless.org/dmenu/) and
[bemenu](https://github.com/Cloudef/bemenu). The Wayland
runtime has been lightly adapted from bemenu. All credit
goes to the people who work on that project for the code
there.
