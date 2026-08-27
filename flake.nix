{
  description = "wk - Which-Key via X11 and Wayland";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    pre-commit-hooks = {
      url = "github:cachix/git-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      ...
    }@inputs:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems =
        f:
        nixpkgs.lib.genAttrs systems (
          system: f system (nixpkgs.legacyPackages.${system}.extend self.overlays.default)
        );
    in
    {
      packages = forAllSystems (
        _: pkgs: {
          default = pkgs.wk;
          wk = pkgs.wk;

          wk-x11 = pkgs.wk.override { backend = "x11"; };
          wk-wayland = pkgs.wk.override { backend = "wayland"; };

          wk-debug = pkgs.wk.overrideAttrs (old: {
            dontStrip = true;
            separateDebugInfo = false;
            NIX_CFLAGS_COMPILE = (old.NIX_CFLAGS_COMPILE or "") + " -ggdb -O0";
          });
        }
      );

      formatter = forAllSystems (_: pkgs: pkgs.nixfmt);

      checks = forAllSystems (
        _: pkgs:
        (import ./checks.nix {
          inherit inputs pkgs;
        })
        // {
          package-default = pkgs.wk;
          package-x11 = pkgs.wk.override { backend = "x11"; };
          package-wayland = pkgs.wk.override { backend = "wayland"; };
          package-with-wks = pkgs.wk.override {
            wksContent = ''
              h "help" %{{echo "test"}}
              t "test" %{{echo "test command"}}
            '';
          };
        }
      );

      devShells = forAllSystems (
        system: pkgs: {
          default = import ./shell.nix {
            inherit pkgs;
            checks = self.checks.${system};
          };
        }
      );

      overlays.default = import ./nix/overlay.nix;
    };
}
