{
  pkgs ? import <nixpkgs> { },
  checks ? { },
  ...
}:
let
  wk-package = pkgs.callPackage ./default.nix { };

  # Make pre-commit optional to avoid circular dependencies
  hasPreCommit = checks ? pre-commit-check;
  preCommitPackages = if hasPreCommit then checks.pre-commit-check.enabledPackages else [ ];
  preCommitHook = if hasPreCommit then checks.pre-commit-check.shellHook else "";
in
pkgs.mkShell {
  buildInputs = [ pkgs.bashInteractive ] ++ preCommitPackages;

  inputsFrom = [ wk-package ];

  nativeBuildInputs = builtins.attrValues {
    inherit (pkgs)
      # Development tools
      clang-tools
      gdb
      valgrind
      # Build tools
      pkg-config
      wayland-scanner
      scdoc
      # Version control
      git
      pre-commit
      ;
  };

  shellHook = ''
    ${preCommitHook}

    echo ""
    echo "wk development environment"
    echo "=========================="
    echo ""
    echo "Build commands:"
    echo "  make          - Build wk for X11 and Wayland"
    echo "  make x11      - Build wk for X11 only"
    echo "  make wayland  - Build wk for Wayland only"
    echo "  make debug    - Build with debug symbols"
    echo "  make test     - Run test suite"
    echo "  make man      - Build man pages"
    echo "  make clean    - Clean build artifacts"
    echo ""
    echo "Code quality:"
    echo "  pre-commit run --all-files  - Run all pre-commit hooks"
    echo "  clang-format -i <file>      - Format a C file"
    echo "  nix fmt                     - Format Nix files"
    echo ""
  '';
}
