{
  inputs,
  pkgs,
  ...
}: {
  pre-commit-check = inputs.pre-commit-hooks.lib.${pkgs.stdenv.hostPlatform.system}.run {
    default_stages = ["pre-commit"];
    src = ./.;
    hooks = {
      # C/C++ formatting
      clang-format = {
        enable = true;
        types_or = ["c" "c++"];
        excludes = [".*\\.wks$"];
      };

      # Git + General checks
      check-case-conflicts.enable = true;
      check-merge-conflicts.enable = true;
      detect-private-keys.enable = true;
      end-of-file-fixer.enable = true;
      fix-byte-order-marker.enable = true;
      mixed-line-endings.enable = true;
      trim-trailing-whitespace.enable = true;

      # Makefile checks
      check-added-large-files = {
        enable = true;
        args = ["--maxkb=500"];
      };
    };
  };
}
