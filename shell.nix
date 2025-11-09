{
  pkgs ? import <nixpkgs> {},
  checks,
  ...
}: let
  wk-package = pkgs.callPackage ./default.nix {};
in
  pkgs.mkShell {
    buildInputs = checks.pre-commit-check.enabledPackages;

    inputsFrom = [wk-package];

    nativeBuildInputs = builtins.attrValues {
      inherit
        (pkgs)
        # Development tools
        clang-tools # clangd LSP, clang-format
        gdb # debugger
        valgrind # memory leak detection
        # Build tools
        pkg-config
        wayland-scanner
        # Version control
        git
        pre-commit
        ;
    };

    shellHook = ''
      ${checks.pre-commit-check.shellHook}

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
      echo "  make clean    - Clean build artifacts"
      echo ""
      echo "Code quality:"
      echo "  pre-commit run --all-files  - Run all pre-commit hooks"
      echo "  clang-format -i <file>      - Format a C file"
      echo "  nix fmt                     - Format Nix files"
      echo ""
    '';
  }
