# Changelog

All notable changes to this project will be documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2025-12-16

This release features a significant internal rewrite with new data structures,
an improved configuration system, and several new features. See
[MIGRATION.md](MIGRATION.md) for upgrade instructions from v0.1.x.

### Added

- **Meta command `@goto`**: Navigate between menu locations without restarting wk.
  Useful for creating "hydra" menus with navigation back to parent menus or root.
  ```
  w "Window" {
      m "Move" +keep {
          ... "Move to %(index+1)" %{{move-window %(index)}}
          BS "Go back" @goto "w"
          S-BS "Go home" @goto ""
      }
  }
  ```

- **Menu titles**: Display a title above the menu using `--title` CLI flag,
  `:title` preprocessor macro, or `+title "..."` flag on chords/prefixes.
  Using `+title` without an argument sets the title to the chord's description.
  Includes `--title-font` and `:title-font` for separate title styling.

- **Command wrapping**: Wrap all commands with a prefix string using
  `--wrap-cmd`, `:wrap-cmd` macro, or `+wrap "..."` flag. Useful for running
  commands through wrappers like `uwsm-app --` or terminal emulators.
  - `+unwrap` flag to opt-out of global wrapping on specific chords
  - Wrapper precedence: chord `+wrap` > inherited `+wrap` > global `:wrap-cmd`

- **User variables** via `:var` preprocessor macro for reusable values:
  ```
  :var "WORKSPACE_CMD" "hyprctl dispatch workspace"
  [asdfghjkl] "Workspace %(index+1)" %{{%(WORKSPACE_CMD) %(index)}}
  ```
  - Variables work in descriptions, commands, and preprocessor macro arguments
  - Meta-variables supported (variable names containing other variables)

- **`keepDelay` setting**: Configurable delay (default 75ms) between keyboard
  ungrab and command execution for `+keep` chords. Prevents focus-stealing
  issues with compositor commands. Set via `:keep-delay N` or `--keep-delay N`.

- **Table padding**: New `--table-padding` / `:table-padding` option for
  additional padding between outermost cells and the border.

- **Shell completions**: Zsh and Bash completion scripts in `completions/`.

- **Nix flake support**: Full Nix packaging with flake.nix including:
  - Backend selection: `pkgs.wk.override { backend = "x11"; }`
  - Build from wks files: `pkgs.wk.override { wksFile = ./keychords.wks; }`
  - Inline wks content: `pkgs.wk.override { wksContent = "..."; }`
  - Directory includes: `pkgs.wk.override { wksDirs = [./wks/shared]; }`

- **Key chord deduplication**: When duplicate keys are defined at the same
  level, the last definition wins (previously caused undefined behavior).

### Changed

- **Configuration architecture redesign**: Merged `key_chords.def.h` into
  `config.def.h`. The separate key chords header file has been removed.  Key
  chords are now defined as part of the main configuration.

- **Default sorting enabled**: Key chords are now sorted by default (matching
  emacs-which-key behavior). Use `--unsorted` or `:unsorted` to disable.
  - Note: Index interpolations (`%(index)`) are resolved before sorting, so
    indices reflect parse order, not final display order.

- **Internal data structures**: Complete rewrite using arena allocation and new
  types (`Span`, `Vector`, `Stack`, `String`, `Property`).  This improves memory
  management and extensibility but changes the transpiled header format.

- **Man pages**: Switched from Org/Pandoc to scdoc for man page generation.
  scdoc is now an optional build dependency.

- **Foreground colors**: Now use designated initializers for clarity:
  ```c
  static const char* foreground[FOREGROUND_COLOR_LAST] = {
      [FOREGROUND_COLOR_KEY]       = "#DCD7BA",
      [FOREGROUND_COLOR_DELIMITER] = "#525259",
      [FOREGROUND_COLOR_PREFIX]    = "#AF9FC9",
      [FOREGROUND_COLOR_CHORD]     = "#DCD7BA",
      [FOREGROUND_COLOR_TITLE]     = "#DCD7BA",
  };
  ```

### Fixed

- Wayland: Menu staying hidden even after delay expired
- Relative includes with symlinked files
- Double escape bug in preprocessor/writer
- Writer not escaping backslashes correctly
- False positive detection for special tokens
- Various compiler stability fixes for edge cases

### Removed

- `key_chords.def.h`: Configuration is now unified in `config.def.h`.  Use `make
  from-wks` to build with custom key chords defined in `config/key_chords.wks`.

- `+ignore-sort` flag: Replaced by making sorting the default and adding
  `--unsorted` / `:unsorted` for the opposite behavior.

## [0.1.3] - 2024-XX-XX

### Added
- Implicit chord array implementation
- `maxWidth` parameter for menu width control

### Fixed
- Wayland menu width/height miscalculation
- Drawing full text when no space for ellipsis

## [0.1.2] - 2024-XX-XX

Initial documented release.

[0.2.0]: https://github.com/3L0C/wk/compare/v0.1.3...v0.2.0
[0.1.3]: https://github.com/3L0C/wk/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/3L0C/wk/releases/tag/v0.1.2
