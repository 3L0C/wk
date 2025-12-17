# Migration Guide: v0.1.x to v0.2.0

This guide covers breaking changes and migration steps when upgrading from
wk v0.1.x to v0.2.0.

## Configuration File Changes

### Unified Configuration Header

The separate `key_chords.def.h` file has been removed. Key chord definitions
are now part of `config.def.h`.

**Before (v0.1.x):**
```
config/
  config.def.h        # Menu appearance settings
  config.h            # Your customized settings (gitignored)
  key_chords.def.h    # Default key chord definitions
  key_chords.h        # Your key chords (gitignored)
```

**After (v0.2.0):**
```
config/
  config.def.h        # All settings including key chord macros
  config.h            # Your customized settings (gitignored)
  key_chords.wks      # Optional: wks source for custom key chords
```

### Migration Steps

#### If you use wks files (recommended)

If you already use `.wks` files for your key chords, migration is simple:

1. **Clean and rebuild:**
   ```bash
   make clean
   make from-wks
   ```

2. **Update your config.h** (if customized):
   - Copy any appearance customizations from your old `config.h` to the new
     `config.def.h` format
   - Add new settings:
     ```c
     static uint32_t keepDelay = 75;
     static const int32_t tablePadding = -1;
     static const char* titleFont = "sans-serif, 16";
     static const char* wrapCmd = NULL;
     ```

3. **Update foreground color syntax** (if customized):
   ```c
   /* Old style */
   static const char* foreground[FOREGROUND_COLOR_LAST] = {
       "#DCD7BA", /* Key */
       "#525259", /* Delimiter */
       "#AF9FC9", /* Prefix */
       "#DCD7BA", /* Chord */
   };

   /* New style (add TITLE color) */
   static const char* foreground[FOREGROUND_COLOR_LAST] = {
       [FOREGROUND_COLOR_KEY]       = "#DCD7BA",
       [FOREGROUND_COLOR_DELIMITER] = "#525259",
       [FOREGROUND_COLOR_PREFIX]    = "#AF9FC9",
       [FOREGROUND_COLOR_CHORD]     = "#DCD7BA",
       [FOREGROUND_COLOR_TITLE]     = "#DCD7BA",
   };
   ```

#### If you use key_chords.h directly

If you were editing `key_chords.h` directly with C code, you have two options:

**Option A: Convert to wks format (recommended)**

1. Create a `config/key_chords.wks` file with your key chords in wks syntax
2. Run `make from-wks` to build

The wks format is more maintainable and gains new features automatically.

**Option B: Update C header format**

The transpiled format has changed significantly. Key differences:

```c
/* Old KeyChord structure */
typedef struct KeyChord {
    KeyChordState state;
    Key key;
    const char* description;
    const char* command;
    const char* before;
    const char* after;
    ChordFlags flags;
    struct KeyChord* keyChords;
} KeyChord;

/* New KeyChord structure */
typedef struct KeyChord {
    Key key;
    Property props[PROP_LAST];  /* Properties array replaces individual fields */
    ChordFlag flags;            /* Single uint16_t bitfield */
    Span keyChords;             /* Span instead of raw pointer */
} KeyChord;
```

The property system now handles description, command, hooks, etc. as a
unified array. Manually maintaining this format is error-prone; converting
to wks is strongly recommended.

## Behavioral Changes

### Sorting is Now Default

Key chords are sorted alphabetically by default. If you relied on the
previous unsorted behavior:

**Using CLI:**
```bash
wk --unsorted --key-chords my_keys.wks
```

**Using preprocessor macro:**
```
:unsorted
a "First" %{{...}}
b "Second" %{{...}}
```

### Index Interpolation Timing

Index interpolations (`%(index)`, `%(index+1)`) are now resolved **before**
sorting. This means indices reflect parse order, not display order:

```
# File content
c "Third" +write %{{%(index)}}   # index = 0
a "First" +write %{{%(index)}}   # index = 1
b "Second" +write %{{%(index)}}  # index = 2

# Display order (sorted): a, b, c
# But indices remain: a=1, b=2, c=0
```

If you need indices to match display order, use `:unsorted`.

## New Features to Consider

### Command Wrapping

If you repeat a common wrapper in many commands, consider using `:wrap-cmd`:

```
# Before
a "App 1" %{{uwsm-app -- app1}}
b "App 2" %{{uwsm-app -- app2}}

# After
:wrap-cmd "uwsm-app --"
a "App 1" %{{app1}}
b "App 2" %{{app2}}
```

### User Variables

Replace repeated strings with variables:

```
# Before
[asdfghjkl] "Workspace %(index+1)" %{{hyprctl dispatch workspace %(index)}}

# After
:var "WS" "hyprctl dispatch workspace"
[asdfghjkl] "Workspace %(index+1)" %{{%(WS) %(index)}}
```

### Menu Titles

Add context to your menus:

```
w "Window" +title "Window Management" {
    ...
}
```

### The @goto Meta Command

Create navigation within your menu hierarchy:

```
a "Apps" { ... }
w "Window" {
    m "Move" +keep {
        ... "To %(index+1)" %{{move %(index)}}
        ESC "Back" @goto "w"
        S-ESC "Home" @goto ""
    }
}
```

## Nix Users

### Flake Usage

```nix
{
  inputs.wk.url = "github:3L0C/wk";

  outputs = { self, nixpkgs, wk }: {
    # Use directly
    packages.x86_64-linux.default = wk.packages.x86_64-linux.default;

    # Or with customization
    packages.x86_64-linux.custom = wk.packages.x86_64-linux.default.override {
      backend = "wayland";
      wksFile = ./my-keys.wks;
    };
  };
}
```

### Home Manager / NixOS Module

Override options are available:
- `backend`: `"both"` (default), `"x11"`, or `"wayland"`
- `wksFile`: Path to a .wks file
- `wksContent`: Inline wks string
- `wksDirs`: List of directories to include for `:include` resolution

## Troubleshooting

### Build fails after upgrade

```bash
make clean
rm -f config/config.h config/key_chords.h
make  # or make from-wks
```

### Old config.h causing issues

If you have a customized `config.h`, compare it with the new `config.def.h`
to ensure all required fields are present.

### wks file errors

Run with `--debug` to see parsing details:
```bash
wk --debug --key-chords my_keys.wks
```
