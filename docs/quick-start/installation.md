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

````{tab} NixOS
Add wk as a flake input, then put the package in
`environment.systemPackages`:

```{code-block} nix
:caption: flake.nix

{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    wk = {
      url = "github:3L0C/wk";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, wk, ... }: {
    nixosConfigurations.CHANGEME = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [({ pkgs, ... }: {
        nixpkgs.overlays = [ wk.overlays.default ];
        environment.systemPackages = [ pkgs.wk ];
      })];
    };
  };
}
```

```{attention}
Replace `CHANGEME` with your system's hostname.
```

```{attention}
This snippet uses an overlay to add `wk` to `pkgs`. If you'd rather
not add this overlay, you can use:

`wk.packages.${pkgs.stdenv.hostPlatform.system}.default`

You may need to use
[`specialArgs`](https://nixos-and-flakes.thiscute.world/nixos-with-flakes/nixos-flake-and-module-system#pass-non-default-parameters-to-submodules)
to pass either `inputs` or `wk` to additional modules.
```
````

````{tab} Home Manager
Add wk as a flake input, then put the package in
`home.packages`:

```{code-block} nix
:caption: flake.nix

{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    wk = {
      url = "github:3L0C/wk";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, home-manager, wk, ... }: {
    homeConfigurations.CHANGEME = home-manager.lib.homeManagerConfiguration {
      pkgs = import nixpkgs {
        system = "x86_64-linux";
        overlays = [ wk.overlays.default ];
      };
      modules = [({ pkgs, ... }: {
        home.packages = [ pkgs.wk ];
      })];
    };
  };
}
```

```{attention}
Replace `CHANGEME` with your username.
```

```{attention}
This snippet uses an overlay to add `wk` to `pkgs`. If you'd rather
not add this overlay, you can use:

`wk.packages.${pkgs.stdenv.hostPlatform.system}.default`

You may need to use
[`extraSpecialArgs`](https://nix-community.github.io/home-manager/nix-flakes/standalone.html?highlight=extraspecialargs#standalone-setup)
to pass either `inputs` or `wk` to additional modules.
```
````

````{tab} Without flakes
Create `wk.nix` and import it in your `configuration.nix`:

```{code-block} nix
:caption: wk.nix

{pkgs, ...}: let
  version = "0.3.5-1";

  wk-src = builtins.fetchTarball {
    url = "https://github.com/3L0C/wk/archive/refs/tags/v${version}.tar.gz";
    sha256 = "01687biymf8wnc7v154a17hzxhp6swzgg1bhmbgdxcws3wl7rmpy";
  };
in {
  nixpkgs.overlays = [ (import "${wk-src}/nix/overlay.nix") ];
  environment.systemPackages = [ pkgs.wk ];
}
```

```{attention}
To upgrade, change to the desired `version`. Replace `sha256`
with `pkgs.lib.fakeHash`. Build the configuration once, and
replace `pkgs.lib.fakeHash` with the expected value.
```

```{note}
Replace `environment.systemPackages` with `home.packages`
for a Home Manager configuration.
```
````

````{tab} Nix package manager
For an imperative install into your user profile:

```{prompt} bash
nix profile install github:3L0C/wk
```

To upgrade later:

```{prompt} bash
nix profile upgrade wk
```

To try wk in a temporary shell without an install:

```{prompt} bash
nix shell github:3L0C/wk
```
````

### Package Variants

The instructions above install wk with X11 and Wayland
support, but backend specific versions are provided:

| Package      | Description              |
|--------------|--------------------------|
| `wk-x11`     | X11 backend only         |
| `wk-wayland` | Wayland backend only     |
| `wk-debug`   | Debug build with symbols |

### Building in a wks config

You can build your config into the binary:

````{tab} wksContent
The `wksContent` override allows you to build your keybinds
into wk, without any additional files:

```{code-block} nix
pkgs.wk.override {
  wksContent = ''
    s "Steam" %{{steam}}
    f "Files" %{{thunar}}
  '';
}
```
````

````{tab} wksFile
The `wksFile` override allows you to build your keybinds
into wk from a given file:

```{code-block} nix
pkgs.wk.override {
  wksFile = ./key_chords.wks;
}
```
````

````{tab} wksDirs
`wksDirs` makes it possible to build more complex
configurations:

```{code-block} nix
pkgs.wk.override {
  wksContent = ''
    # Some common settings
    :delay 0
    :font "monospace, 12"

    :include "niri/main.wks"
  '';

  wksDirs = [
    ./common
    ./niri
  ];
}
```

When using `wksDirs`, the given directories are copied into
the build environment. The contents of the directories are
placed into `config/` under their basename (e.g.,
`./wks/common` -> `config/common`).  This makes it possible
to [`:include`](../reference/wks.md#include-macro) files
across the given directories.

It is best to use unique terminal directories like
`./wks/common` and `./wks/niri` vs. `./common/wks` and
`./niri/wks`. The second form will copy the contents of
`./common/wks` into `config/wks`. Then, the contents of
`./niri/wks` will be copied int `config/wks/wks`.

```{attention}
You must give either `wksContent` or `wksFile` when using
`wksDirs`. It is an error to give both.
```
````
