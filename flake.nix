{
  description = "wk";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: let
    outputs = (flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.outputs.legacyPackages.${system};
      wk-package = pkgs.callPackage ./default.nix { };
    in {
      packages = {
        default = wk-package;
        wk = wk-package;
      };
    }));
  in outputs;
}
