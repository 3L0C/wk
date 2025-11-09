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
      };

      apps.default = {
        type = "app";
        program = "${wk-package}/bin/wk";
      };

      # Formatter for 'nix fmt'
      formatter = pkgs.alejandra;

      # Pre-commit checks
      checks = import ./checks.nix {inherit inputs pkgs;};

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
