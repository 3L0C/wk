# Nix Installation

## Flakes

### Basic Usage

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

### Using the Overlay

The overlay provides `pkgs.wk` so you don't need the verbose package path:

```nix
({ pkgs, ... }: {
  nixpkgs.overlays = [ wk.overlays.default ];
  environment.systemPackages = [ pkgs.wk ];
})
```

## Without Flakes

```nix
# configuration.nix or home.nix
{ pkgs, ... }:
let
  wk-src = builtins.fetchTarball {
    # Tags: https://github.com/3L0C/wk/tags
    # Or use a commit: .../archive/<commit>.tar.gz
    url = "https://github.com/3L0C/wk/archive/refs/tags/v0.2.1.tar.gz";
    sha256 = ""; # Nix will tell you the correct hash
  };
  wk = pkgs.callPackage "${wk-src}/default.nix" {};
in {
  environment.systemPackages = [ wk ];  # or home.packages
}
```

## Package Variants

| Package          | Description                                 |
|------------------|---------------------------------------------|
| `wk` / `default` | Both X11 and Wayland backends               |
| `wk-x11`         | X11 backend only (smaller, no Wayland deps) |
| `wk-wayland`     | Wayland backend only (smaller, no X11 deps) |
| `wk-debug`       | Debug build with symbols                    |

```nix
wk.packages.${system}.wk-x11
wk.packages.${system}.wk-wayland

# Or via override (with overlay)
pkgs.wk.override { backend = "x11"; }
pkgs.wk.override { backend = "wayland"; }
```

## Customization

### Custom Key Chords

```nix
# From file
pkgs.wk.override { wksFile = ./keychords.wks; }

# Inline
pkgs.wk.override {
  wksContent = ''
    h "help" %{{echo "Help!"}}
    a "apps" {
      f "firefox" %{{firefox}}
    }
  '';
}

# Multi-file configs with :include
pkgs.wk.override {
  wksFile = ./wks/main.wks;
  wksDirs = [ ./wks/common ./wks/apps ];  # copied to config/
};
# main.wks can use: :include "common/utils.wks"
```

Validation: `wksContent` and `wksFile` are mutually exclusive. `wksDirs`
requires an entry point (`wksFile` or `wksContent`).

### Patches

```nix
pkgs.wk.override { patches = [ ./fix.patch ]; }
```

## Development

```bash
nix develop github:3L0C/wk
```
