{
  description = "wk - Which-Key via X11 and Wayland";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    pre-commit-hooks = {
      url = "github:cachix/git-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    ...
  } @ inputs:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      wk-package = pkgs.callPackage ./default.nix {};
    in {
      packages = {
        default = wk-package;
        wk = wk-package;

        # Backend-specific variants
        wk-x11 = wk-package.override { backend = "x11"; };
        wk-wayland = wk-package.override { backend = "wayland"; };

        # Debug build variant
        wk-debug = wk-package.overrideAttrs (old: {
          dontStrip = true;
          separateDebugInfo = false;
          NIX_CFLAGS_COMPILE = (old.NIX_CFLAGS_COMPILE or "") + " -ggdb -O0";
        });
      };

      apps.default = {
        type = "app";
        program = "${wk-package}/bin/wk";
      };

      # Formatter for 'nix fmt'
      formatter = pkgs.alejandra;

      # Pre-commit checks and build variant checks
      checks = (import ./checks.nix {inherit inputs pkgs;}) // {
        # Build variant checks
        package-default = wk-package;
        package-x11 = wk-package.override { backend = "x11"; };
        package-wayland = wk-package.override { backend = "wayland"; };
        package-with-wks = wk-package.override {
          wksContent = ''
            h "help" %{{echo "test"}}
            t "test" %{{echo "test command"}}
          '';
        };
      };

      # Development shell
      devShells.default = import ./shell.nix {
        inherit pkgs;
        checks = self.checks.${system};
      };
    })
    // {
      overlays.default = final: prev: {
        wk = final.callPackage ./default.nix {};
      };
    };
}
