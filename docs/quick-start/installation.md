# Installation

## Installing from source

Building from source is straightforward.

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

Install all dependencies for both backends with one of the
following commands:

````{tab} Arch
```{prompt} bash
sudo pacman -S base-devel cairo pango libx11 libxinerama wayland wayland-protocols libxkbcommon
```
````

````{tab} Fedora
```{prompt} bash
sudo dnf install gcc make pkg-config cairo-devel pango-devel libX11-devel libXinerama-devel wayland-devel wayland-protocols-devel libxkbcommon-devel
```
````

````{tab} Ubuntu
```{prompt} bash
sudo apt install build-essential pkg-config libcairo2-dev libpango1.0-dev libx11-dev libxinerama-dev libwayland-dev wayland-protocols libxkbcommon-dev
```
````

````{tab} Debian
```{prompt} bash
sudo apt install build-essential pkg-config libcairo2-dev libpango1.0-dev libx11-dev libxinerama-dev libwayland-dev wayland-protocols libxkbcommon-dev
```
````

````{tab} openSUSE
```{prompt} bash
sudo zypper install gcc make pkg-config cairo-devel pango-devel libX11-devel libXinerama-devel wayland-devel wayland-protocols-devel libxkbcommon-devel
```
````

````{tab} Void
```{prompt} bash
sudo xbps-install -S base-devel cairo-devel pango-devel libX11-devel libXinerama-devel wayland-devel wayland-protocols libxkbcommon-devel
```
````

```{tip}
If you only need one backend, you can omit the X11 or Wayland
packages. For X11-only, skip `wayland*` and `libxkbcommon`
packages. For Wayland-only, skip `libx11`/`libxinerama`
packages (or their equivalents).
```

### Building

Once you've got all the necessary dependencies installed,
you can run the following:

```{prompt} bash
git clone "https://github.com/3L0C/wk.git"
cd wk
make && sudo make install
```

### Building for a specific backend

If you don't need both X11 and Wayland support, you can
build wk exclusively for one or the other with

````{tab} Wayland
```{prompt} bash
make wayland && sudo make install
```
````

````{tab} X11
```{prompt} bash
make x11 && sudo make install
```
````

### Building with a wks config

You can compile your wks configuration directly into the
binary so that wk launches with your key chords by default
(no need to pass a file at runtime).

Place your wks file at `config/key_chords.wks`, then build
with the `from-wks` target:

```{prompt} bash
cp /path/to/your/config.wks config/key_chords.wks
make from-wks && sudo make install
```

Backend-specific variants are also available:

````{tab} Wayland
```{prompt} bash
make from-wks-wayland && sudo make install
```
````

````{tab} X11
```{prompt} bash
make from-wks-x11 && sudo make install
```
````

```{tip}
If your wks file uses `:include` directives, make sure the
included files are accessible relative to `config/`.
```

## Installing via Nix

### Flakes

```nix
# flake.nix
{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  inputs.wk.url = "github:3L0C/wk";

  outputs = { nixpkgs, wk, ... }: {
    nixosConfigurations.myhost = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [({ pkgs, ... }: {
        environment.systemPackages = [
          wk.packages.${pkgs.stdenv.hostPlatform.system}.wk
        ];
      })];
    };
  };
}
```

#### Using the Overlay

The overlay provides `pkgs.wk` so you don't need the verbose package path:

```nix
({ pkgs, ... }: {
  nixpkgs.overlays = [ wk.overlays.default ];
  environment.systemPackages = [ pkgs.wk ];
})
```

### Without Flakes

```nix
# configuration.nix or home.nix
{ pkgs, ... }:
let
  wk-src = builtins.fetchTarball {
    # Tags: https://github.com/3L0C/wk/tags
    # Or use a commit: .../archive/<commit>.tar.gz
    url = "https://github.com/3L0C/wk/archive/refs/tags/v{version}.tar.gz";
    sha256 = ""; # Nix will tell you the correct hash
  };
  wk = pkgs.callPackage "${wk-src}/default.nix" {};
in {
  environment.systemPackages = [ wk ];  # or home.packages
}
```

```{attention}
The above example is incomplete. You will need to input the desired version number in place of `{version}` in the `url` and the correct `sha256` for the above example to work. To get the correct `sha256`, you can build your system and check the error to get the correct value.
```

### Package Variants

| Package          | Description                   |
|------------------|-------------------------------|
| `wk` / `default` | Both X11 and Wayland backends |
| `wk-x11`         | X11 backend only              |
| `wk-wayland`     | Wayland backend only          |
| `wk-debug`       | Debug build with symbols      |

```nix
wk.packages.${system}.wk-x11
wk.packages.${system}.wk-wayland

# Or via override (with overlay)
pkgs.wk.override { backend = "x11"; }
pkgs.wk.override { backend = "wayland"; }
```
