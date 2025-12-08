![wk.png](./wk-which-key.png)

wk
========

`wk` - Which-Key via X11 and Wayland.
Displays available key chords in a popup window.
Inspired by
[emacs-which-key](https://github.com/justbur/emacs-which-key),
[dmenu](https://tools.suckless.org/dmenu/), and
[bemenu](https://github.com/Cloudef/bemenu).

# Introduction

`wk` offers users a portable, scriptable, and highly
customizable interface for their key chord mappings through
a number of sources. Key chords can be built into the binary
via the [key_chords.def.h](config/key_chords.def.h) header,
read from a [wks file](#wks-Files), or read from stdin with
the same `wks` syntax.

## Video Demos

As you can tell from the length of this README, there is a
lot to go over. If you would prefer a video demo, you can
check out the demo series
[here](https://youtube.com/playlist?list=PL20SXZFQEeVPi9vtnxYvMmvOfUalXBWMi&feature=shared)
where I cover `wk` basics and beyond.

# Building

## Make

```bash
# Make wk for X11 and Wayland (uses default or existing key chords)
make

# Build from wks file (transpile config/key_chords.wks and rebuild)
# This is a three-stage bootstrapping process:
#   1. Build wk with bootstrap key chords
#   2. Transpile key_chords.wks using the built binary
#   3. Rebuild wk with the transpiled key chords
make from-wks

# Make wk for X11 only
make x11

# Make wk for Wayland only
make wayland

# Build from wks file for X11 only
make from-wks-x11

# Build from wks file for Wayland only
make from-wks-wayland

# Install
make clean && make && sudo make install

# Install with custom key chords from wks file
make clean && make from-wks && sudo make install
```

## Nix

```nix
# Default: both backends with default key chords
pkgs.wk

# X11-only build (lightweight, skips Wayland dependencies)
pkgs.wk.override { backend = "x11"; }

# Wayland-only build (skips X11 dependencies)
pkgs.wk.override { backend = "wayland"; }

# Both backends with custom wks file
pkgs.wk.override { wksFile = ./my-keychords.wks; }

# X11-only with inline wks content
pkgs.wk.override {
  backend = "x11";
  wksContent = ''
    h "help" %{{echo "Help!"}}
    q "quit" %{{echo "Quit!"}}
  '';
}

# Wayland-only with wks file
pkgs.wk.override {
  backend = "wayland";
  wksFile = ./keychords.wks;
}

# Structured configs with directories
# Use wksFile for entry point, wksDirs to copy dependency directories
# Directories are copied with full structure preserved
pkgs.wk.override {
  wksFile = ./wks/main.wks;
  wksDirs = [
    ./wks/shared    # Copied as config/shared/
    ./wks/apps      # Copied as config/apps/
  ];
}
# Your main.wks can use:
#   :include "shared/utils.wks"
#   :include "apps/browser.wks"

# Note: wksContent is mutually exclusive with wksFile and wksDirs
# wksDirs requires wksFile (can't be used standalone)
```

# Dependencies

- C compiler
- scdoc to generate man pages

All runtime dependencies below are searched with
`pkg-config`.

| Backend | Dependencies                                                  |
|---------|---------------------------------------------------------------|
| Common  | cairo, pango, pangocairo                                      |
| X11     | x11, xinerama                                                 |
| Wayland | wayland-client, wayland-protocols, xkbcommon, wlr-layer-shell |

## About Wayland support

Wayland is only supported by compositors that implement the
[wlr-layer-shell](https://gitlab.freedesktop.org/wlroots/wlr-protocols/tree/master/unstable)
protocol. Typically
[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots/)-based
compositors. For those not on a `wlroots`-based compositor,
[Xwayland](https://wayland.freedesktop.org/xserver.html)
does work based on my testing, but the popup menu seems to
only display on one screen.

# Usage

```
# Display built-in key chords.
wk

# Display menu at the top of the screen.
wk --top

# Display key chords in a `wks` file.
wk --key-chords my_key_chords.wks

# Try to pre-press keys 'C-c', and 'x'. Both options work the same.
wk --press 'C-cx'
wk --press 'C-c x'

# Transpile the key chords in a `wks` file and print a properly
# formated `key_chords.def.h` header to stdout.
wk --transpile my_key_chords.wks

# Read script from stdin
printf '%s\n' 'a "Chord" %{{echo "Hello, world!"}}' | wk --script

# Everything else
wk --help
usage: wk [options]

options:
    -h, --help                 Display help message and exit.
    -v, --version              Display version number and exit.
    -d, --debug                Print debug information.
    -D, --delay INT            Delay the popup menu by INT milliseconds from
                               startup/last keypress (default 1000 ms).
    -t, --top                  Position menu at top of screen.
    -b, --bottom               Position menu at bottom of screen.
    -s, --script               Read script from stdin to use as key chords.
    -U, --unsorted             Disable sorting of key chords (sorted by default).
    -m, --max-columns INT      Set the maximum menu columns to INT (defualt 5).
    -p, --press KEY(s)         Press KEY(s) before dispalying menu.
    -T, --transpile FILE       Transpile FILE to valid 'key_chords.h' syntax and
                               print to stdout.
    -k, --key-chords FILE      Use FILE for key chords rather than those
                               precompiled.
    -w, --menu-width INT       Set menu width to INT. Set to '-1' for a width
                               equal to 1/2 of the screen width (default -1)
    -g, --menu-gap INT         Set menu gap between top/bottom of screen to INT.
                               Set to '-1' for a gap equal to 1/10th of the
                               screen height (default -1).
    --wrap-cmd STRING          Wrap all commands with STRING, i.e.,
                                   /bin/sh -c STRING cmd
                               This does not apply to hooks (default "").
    --border-width INT         Set border width to INT (default 4).
    --border-radius NUM        Set border radius to NUM degrees. 0 means no curve
                               (default 0).
    --wpadding INT             Set left and right padding around hint text to
                               INT (default 6).
    --hpadding INT             Set top and bottom padding around hint text to
                               INT (default 2).
    --table-padding INT        Set additional padding between the outermost cells
                               and the border to INT. -1 = same as cell padding,
                               0 = no additional padding (default -1).
    --fg COLOR                 Set all menu foreground text to COLOR where color
                               is some hex string i.e. '#F1CD39' (default unset).
    --fg-key COLOR             Set foreground key to COLOR (default '#DCD7BA').
    --fg-delimiter COLOR       Set foreground delimiter to COLOR (default '#525259').
    --fg-prefix COLOR          Set foreground prefix to COLOR (default '#AF9FC9').
    --fg-chord COLOR           Set foreground chord to COLOR (default '#DCD7BA').
    --fg-title COLOR           Set foreground title to COLOR (default '#DCD7BA').
    --title STRING             Set global title displayed above menu to STRING.
    --title-font STRING        Set title font to STRING. Should be a valid Pango
                               font description (default 'sans-serif, 16').
    --bg COLOR                 Set background to COLOR (default '#181616').
    --bd COLOR                 Set border to COLOR (default '#7FB4CA').
    --shell STRING             Set shell to STRING (default '/bin/sh').
    --font STRING              Set font to STRING. Should be a valid Pango font
                               description (default 'monospace, 14').
    --implicit-keys STRING     Set implicit keys to STRING (default 'asdfghjkl;').

run `man 1 wk` for more info on each option.
```

# Configuration

`wk` can be configured at the command line as shown in the
above help message, or your configuration can be built into
the binary by changing the settings in
[config.def.h](config/config.def.h).

Key chords can be customized in two ways:
1. **Direct editing**: Modify [config/key_chords.h](config/key_chords.h) (C code) and run `make`
2. **From wks file**: Create [config/key_chords.wks](config/key_chords.wks) (wks syntax) and run `make from-wks`

The wks approach is more powerful and recommended for complex configurations.

# wks Files

Which-Key source (`wks`) files are the driving force behind
`wk`. The syntax is novel but provides a flexible means to
manage and express key chords.

## Comments

In `wks` files, comments can be added using the pound
character (`#`). When a pound character is encountered, it
signifies the start  of a comment. The comment extends from
the pound character until the end of the line. It's
important to note that the pound character is treated as a
literal character within descriptions and commands and does
not indicate the start of a comment in those contexts.

## Key Chords

```
key_chord -> ( chord | prefix | chord_array ) ;
```

The key chord is the main building block of a `wks` file.
This can be either a chord, prefix, or a chord array.
The chord is the most basic example of a key chord and
serves as a good entry point for this discussion.

### Chords

A chord is a key chord that results in `wk` performing some
action, like executing a command, when the trigger key is
pressed.

```
chord -> trigger_key description keyword* command ;
```

All chords must have a trigger key, description, and a
command. Zero or more keywords may be given between the
description and command. These will be addressed later. For
now, let's break down the required parts of the chord.

#### Trigger Keys

A trigger key represents the specific keypress or key
combination that triggers a corresponding action or command.
In a `wks` file, it is the written representation of the
physical key(s) pressed by the user on their keyboard.

```
trigger_key -> modifier* ( normal_key | special_key ) ;
```

A trigger key is then zero or more modifiers followed by a
normal key or a special key.

#### Normal Keys

A normal key is any printable, non-whitespace, utf8
character.

```
normal_key -> ( '\\' [\\\[\]{}#":^+()] | [^\s\[\]{}#":^+()] ) ;
```

The characters `\`, `[`, `]`, `{`, `}`, `#`, `"`, `:`, `^`,
`+`, `(`, and `)` have special meanings in `wks` files. To
use any of these as a normal key, simply precede them with
a backslash (`\`).

All other non-whitespace, printable utf8 characters prior to
a description will be interpreted as a normal key. Those
that are whitespace or non-printable fall into the special
key category.

#### Special Keys

Special keys like `tab`, `escape`, `spacebar`, and `F1` can
still be used as trigger keys in `wks` files via their
special forms.

| Special Key    | `wks` form |
|----------------|------------|
| Left arrow     | `Left`     |
| Right arrow    | `Right`    |
| Up arrow       | `Up`       |
| Down arrow     | `Down`     |
| Tab            | `TAB`      |
| Space          | `SPC`      |
| Enter/Return   | `RET`      |
| Delete         | `DEL`      |
| Backspace      | `BS`       |
| Esc            | `ESC`      |
| Home           | `Home`     |
| Page up        | `PgUp`     |
| Page down      | `PgDown`   |
| End            | `End`      |
| Begin          | `Begin`    |
| F[1-35]        | `F[1-35]`  |
| Volume Down    | `VolDown`  |
| Mute Vol       | `VolMute`  |
| Volume Up      | `VolUp`    |
| Play Audio     | `Play`     |
| Stop Audio     | `Stop`     |
| Audio Previous | `Prev`     |
| Audio Next     | `Next`     |

In `wks` files, whitespace is generally not significant
around individual parts of the syntax, with one notable
exception: special keys. When using special keys, it is
required to include whitespace between the end of the
special key and the start of the next item in the `wks`
file.

If you have any additional special keys that you would like
`wks` files to support, please open an issue or a pull
request.

#### Modifiers

As mentioned above, zero or more modifiers can be given in a
trigger key. Modifiers can be used in `wks` files via their
special forms.

| Modifier    | `wks` form |
|-------------|------------|
| Control     | `C-`       |
| Alt         | `M-`       |
| Hyper/Super | `H-`       |
| Shift       | `S-`       |

Modifiers act as one would expect. To match the keypress
`Control+c` use the form `C-c` in your `wks` file.

Among the modifiers, the Shift modifier (`S-`) has a unique
behavior when used with standard utf8 key characters. Due to
the way normal keys are interpreted, the `S-` modifier is
not always necessary. To determine whether `S-` is required,
it is recommended to test the character in a `wks` file by
typing it with and without the Shift key pressed.

If the character is non-whitespace, printable, and the
shifted and unshifted versions produce different output,
then the `S-` modifier is not needed. For instance, pressing
the `a` key with the Shift key held down produces an
uppercase `A`. This test demonstrates that the key's output
changes based on the Shift key state.

In such cases, using `S-a` in a `wks` file would not work as
expected because the key will never match when the user
presses `Shift+a`.

I am open to changing it so that `S-a` and `A` match the
same `Shift+a` keypress, but I have yet to find a fitting
solution. The ones I can think of either involve depending
on some utf8 library, writing the code by hand, or
permitting this syntax for ASCII but not other character
sets. Each has its own drawback, and I find the current
solution to be intuitive in practice.

#### Descriptions

Descriptions provide a hint about the purpose of the chord
or prefix.

```
description -> '"' ( '\\"' | [^"] | interpolation )* '"' ;
```

A description starts with a double quote (`"`), followed by
zero or more escaped double quotes, non-double quote
characters, or an interpolation and ends with a double
quote. Aside from interpolations, a description looks like
your typical string in many programming languages.

#### Commands

Commands are the actions executed upon completing a key
chord sequence.

```
command -> '%' delimiter ( . | interpolation )* delimiter ;
```

A command begins with the percent character (`%`) followed
by a delimiter. After the delimiter zero or more
characters, or interpolations may be given. A command is
ended with the same delimiter that followed the percent
character.

Because the delimiter is user defined, there should be no
misinterpretation of anything between the delimiters. This
means any command given at the command-line should be right
at home in between the delimiters.

#### Delimiters

A delimiter acts as a start and stop marker for a command in
a `wks` file.

```
delimiter   -> ( open_delim | close_delim | ([^[{(])\1 ) ;
open_delim  -> ( '{{' | '((' | '[[' ) ;
close_delim -> ( '}}' | '))' | ']]' ) ;
```

The delimiter from one command to the next may be completely
different. This puts the burden on the user to ensure their
delimiter is compatible with the content of the command. The
only restriction is that the delimiter is one **ASCII**
character repeated exactly twice. **Note** this excludes any
null bytes (`\0`) as these are used to determine the end of
a `wks` file.

Here are some examples of different delimiters for the same
command.

```
# The traditional example
%{{echo "hello, world"}}

# Valid alternatives
%||echo "hello, world"||
%((echo "hello, world"))
%\\echo "hello, world"\\
%%%echo "hello, world"%%
%zzecho "hello, world"zz
```

Inspired by [sed](https://www.gnu.org/software/sed/), this
should keep `wks` syntax compatible with shell commands,
almost indefinitely.  It also makes it possible to nest a
`wks` script within a `wks` command if you want to get
really weird.

#### Basic Chord Example

Having learned a large portion of the syntax so far, we can
create our first chord using `wks` syntax.

```
a "Chord" %{{echo "Hello, world!"}}
```

A lot of explanation to do something simple, but this will
set us up for success in the long run. Next is the humble
prefix.

### Prefixes

A prefix is a special type of key chord that acts as a
container for other key chords. It represents an incomplete
key combination that does not trigger a command on its own.

```
prefix -> trigger_key description keyword* '{' ( key_chord )+ '}' ;
```

A prefix has many of the same components as a chord. It
begins with a trigger key, followed by a description, zero
or more keywords and then a block of one or more key chords
surrounded by an opening and closing brace (`{`, and `}`).

**Note** that a key chord may be a prefix, a chord, or a
chord array, meaning many prefixes can be nested one  inside
another.

Here is a simple example of a prefix:

```
m "+Music"
{
    n "Next" %{{mpc next}}
    p "Prev" %{{mpc prev}}
}
```

### Chord Arrays

Chords and prefixes are standard fare in the realm of key
chords, so what the heck is a chord array? Well, mostly
syntactic sugar so you do not have to repeat yourself when
it comes to chords that are very similar but only differ in
slightly different ways.

```
chord_array -> ( implicit_array | explicit_array ) ;
```

A chord array comes in two flavors, /implicit/ and
/explicit/.

### Implicit Arrays

An /implicit array/ is the simplest of the two flavors. It
utilizes the `implicitArrayKeys` defined in
[config.def.h](config/config.def.h) to
generate chords from these trigger keys.

```
implicit_array -> modifier* '...' description keyword* command ;
```

An implicit array is then zero or more modifiers, an
ellipsis (`...`), a description, zero or more keywords, and
a command. This is practially a chord in terms of its
form, but in behavior an implicit array generates any
number of chords from this simple syntax.

As an example, say your implicit array keys are set to `h`,
`j`, `k`, and `l`, and you have this `wks` file:

```
... "Switch workspace %(index+1)" %{{xdotool set_desktop %(index)}}
```

This is the equivilant `wks` file without the use of an
implicit array:

```
h "Switch workspace 1" %{{xdotool set_desktop 0}}
j "Switch workspace 2" %{{xdotool set_desktop 1}}
k "Switch workspace 3" %{{xdotool set_desktop 2}}
l "Switch workspace 4" %{{xdotool set_desktop 3}}
```

The chords are generated in the same order as `implicitArrayKeys`
(h, j, k, l in this example).


### Explicit Arrays

An /explicit array/ is most useful when the desired chords
are less homogeneous.

```
explicit_array -> '[' ( trigger_key | chord_expression )+ ']' description keyword* command ;
```

To use an explicit array begin with an open bracket (`[`)
followed by one or more trigger keys or chord expressions.
The array portion ends with a closing bracket (`]`) followed
by the standard chord components, a description, zero or
more keywords, and a command.

I think an example will make things clear:

```
# Chord array version
[asdfghjkl] "Switch workspace %(index+1)" %{{xdotool set_desktop %(index)}}

# Individual chords and no interpolation
a "Switch workspace 1" %{{xdotool set_desktop 0}}
s "Switch workspace 2" %{{xdotool set_desktop 1}}
d "Switch workspace 3" %{{xdotool set_desktop 2}}
f "Switch workspace 4" %{{xdotool set_desktop 3}}
g "Switch workspace 5" %{{xdotool set_desktop 4}}
h "Switch workspace 6" %{{xdotool set_desktop 5}}
j "Switch workspace 7" %{{xdotool set_desktop 6}}
k "Switch workspace 8" %{{xdotool set_desktop 7}}
l "Switch workspace 9" %{{xdotool set_desktop 8}}
```

In this case, explicit arrays are only slightly different
than an implicit array. However, explicit arrays support
chord expressions which make them far more flexible.

#### Chord Expressions

Chord arrays can be very simple with each chord being only
slightly different from one another. However, it may make
sense to include chords that mostly fit into the chord array
with some more distinct differences. For this situation,
chord expressions may be the answer.

```
chord_expression -> '(' trigger_key description keyword* command? ')' ;
```

A chord expression is only valid within a chord array, and
it is essentially a chord wrapped in parentheses with some
added flexibility. Normally, a chord requires at least a
trigger key, a description, and a command. A chord
expression, on the other hand, requires only a key and a
description. Any other information will be filled in by the
surrounding chord array.

Here is an example of a chord expression within a chord array:

```
# With chord arrays and chord expressions
[
    (b "Brave")
    (c "Mullvad Chrome" %{{mullvad-exclude chrome ~/startpage.html}})
    x
] "XDG-OPEN" %{{%(desc,,) ~/startpage.html}}

# With chords and no interpolation
b "Brave" %{{brave ~/startpage.html}}
c "Mullvad Chrome" %{{mullvad-exclude chrome ~/startpage.html}}
x "XDG-OPEN" %{{xdg-open ~/startpage.html}}
```

Admittedly, chord expressions may not be that useful but
they were easy to implement so they are here for those who
want to use them.

### Interpolations

I have used interpolations in the last few examples without
any explanation. Let's fix that.

```
interpolation -> '%(' ( chord_metadata | user_variable ) ')' ;
```

The basic syntax for an interpolation begins with a `%(`
delimiter followed by either a chord metadata identifier or
a user variable name, and closing parenthesis (`)`).

There are two types of interpolations with different scopes:

**Chord Metadata Interpolations** - Built-in identifiers
that provide access to metadata about the current chord.
These are only valid in descriptions and commands.

**User Variable Interpolations** - User-defined variables
created with the `:var` preprocessor macro. These can be
used in descriptions, commands, and preprocessor macro
arguments.

#### Chord Metadata

The following built-in identifiers provide access to chord
metadata and are only valid in descriptions and commands:

```
chord_metadata -> ( 'key'
                  | 'index'
                  | 'index+1'
                  | 'desc'
                  | 'desc^'
                  | 'desc^^'
                  | 'desc,'
                  | 'desc,,'
                  | 'wrap_cmd' ) ;
```

| Identifier | Metadata                                                                             |
|------------|--------------------------------------------------------------------------------------|
| `key`      | The key portion of the chord.                                                        |
| `index`    | The base 0 index of the chord in the current scope (prefixes begin new scopes).      |
| `index+1`  | The base 1 index of the chord in the current scope (prefixes begin new scopes).      |
| `desc`     | The description of the current chord. May not be given within the description.       |
| `desc^`    | The description of the current chord with the first character capitalized.           |
| `desc^^`   | The description of the current chord with all characters capitalized.                |
| `desc,`    | The description of the current chord with the first character downcased.             |
| `desc,,`   | The description of the current chord with all characters downcased.                  |
| `wrap_cmd` | The globally defined `wrap_cmd`. Set through config.h, `--wrap-cmd`, or `:wrap-cmd`. |

#### User Variables

User-defined variables created with the [`:var`](#the-var-macro)
preprocessor macro can be accessed through interpolation:

```
user_variable -> [^)]+ ;
```

Unlike chord metadata, user variables can be used in three
contexts:
- Descriptions
- Commands
- Preprocessor macro arguments (`:font`, `:title`, `:title-font`, `:fg-*`, `:bg`, `:bd`, `:shell`, `:wrap-cmd`, `:include`, and in `:var` itself)

This enables powerful meta-programming capabilities such as
meta-variables (variables with computed names) and dynamic
configuration values. See [The Var Macro](#the-var-macro)
for examples.

### Keywords

So far keywords have been glossed over, but they are very
handy.

```
keyword -> ( hook | flag ) ;
```

A keyword is either a hook or a flag. Both have equal
precedence, meaning they can be mixed up wherever they are
permitted.

#### Hooks

Hooks provide means of adding additional commands to a chord
or prefix.

```
hook -> '^' ( 'before'
            | 'after'
            | 'sync-before'
            | 'sync-after' ) command ;
```

A hook begins with the caret character (`^`), followed by
the type of hook, and finally the command the hook will run.

The hook type has to do with the order the command will be
run. The `before` hooks run before the chord's command, and
the `after` hooks run after the chord's command.

The `sync-*` hooks relate to how `wk` runs the commands. By
default, all commands are run asynchronously to prevent a
command from blocking `wk`. However, if the hook must
complete before `wk` can proceed you can use the `sync-*`
variant to enforce this behavior.

**Note** that a blocking command may prevent `wk` from ever
resuming execution. In the event that this happens, users
may need to restart their system entirely to regain control
of their keyboard.

Users can certainly chain commands together the same way one
would chain commands in a regular shell, but hooks help to
reduce repetition. They also make more sense in the context
of prefixes.

```
# With hooked prefix
e "+Emacs" ^before %{{xdotool set_desktop 1}}
{
    o "Open" %{{emacsclient -c -a ""}}
    r "Roam" %{{emacsclient -c -a "" ~/20240101080032-startpage.org}}
}

# Without hooks
e "+Emacs"
{
    o "Open" %{{xdotool set_desktop 1 ; emacsclient -c -a ""}}
    r "Roam" %{{xdotool set_desktop 1 ; emacsclient -c -a "" ~/20240101080032-startpage.org}}
}
```

As you can see, this helps to cut down on repetition, but it
also helps enforce a workflow rule without the need to setup
desktop environment rules and such.

This example also hints at the idea of inheritance as the
hook was given to a prefix and not to individual chords.
This topic is covered after introducing flags as these also
factor into the discussion.

#### Meta Commands

Meta commands are special directives that control the menu
itself rather than executing shell commands. They are
mutually exclusive with hooks and regular commands.

```
meta_command -> '@' ( 'goto' description ) ;
```

##### The @goto Meta Command

The `@goto` meta command navigates to a different location
in the key chord hierarchy without closing and restarting
`wk`. This is useful for creating "hydra" menus where users
can perform related actions and then navigate elsewhere.

```
a "Applications" { ... }
w "Window"
{
    m "Move" +keep
    {
        ... "Move to %(index+1)" %{{move-window %(index)}}
        BS "Go back" @goto "w"
        S-BS "Go home" @goto ""
    }
}
```

The path argument follows the same syntax as `--press`:
- `@goto ""` - Navigate to root menu
- `@goto "w"` - Navigate to the "w" prefix
- `@goto "w m"` - Navigate through "w" then "m"

If the path leads to a chord with a command (not a prefix),
that command is executed.

**Note:** `@goto` cannot be combined with hooks (`^before`,
`^after`) or regular commands (`%{{}}`). Circular goto
chains (e.g., `a @goto "b"` and `b @goto "a"`) are detected
and result in an error.

#### Flags

Flags are similar to command-line flags in that they change
the behavior of `wk`.

```
flag -> '+' ( 'keep'
            | 'close'
            | 'inherit'
            | 'ignore'
            | 'unhook'
            | 'deflag'
            | 'no-before'
            | 'no-after'
            | 'write'
            | 'execute'
            | 'sync-command'
            | 'unwrap'
            | 'wrap' '"' ( '\\"' | [^"] | interpolation )* '"'
            | 'title' '"' ( '\\"' | [^"] | interpolation )* '"') ;
```

Flags begin with a plus character (`+`), followed by the
flag itself. Here is how each flag changes the behavior of
`wk`:

| Flag           | Behavior                                                                                                                      |
|----------------|-------------------------------------------------------------------------------------------------------------------------------|
| `keep`         | Instead of closing after `wk` finds a matching chord, it keeps the `wk` menu open.                                            |
| `close`        | Forces the `wk` window to close. Useful when `+keep` was given to a surrounding prefix.                                       |
| `inherit`      | Causes the prefix to inherit flags and hooks from its parent. Has no effect when given to a chord.                            |
| `ignore`       | Ignore all hooks and flags from the surrounding prefix. Has no effect when given to a prefix.                                 |
| `unhook`       | Ignore all hooks from the surrounding prefix.                                                                                 |
| `deflag`       | Ignore all flags from the surrounding prefix.                                                                                 |
| `no-before`    | Ignore `before` and `sync-before` hooks from the surrounding prefix.                                                          |
| `no-after`     | Ignore `after` and `sync-after` hooks from the surrounding prefix.                                                            |
| `write`        | Write commands to stdout rather than executing them.                                                                          |
| `execute`      | Execute the command rather than writing them to stdout. Useful when `+write` was given to a surrounding prefix.               |
| `sync-command` | Execute the command in a blocking fashion. See the note in [hooks](#hooks) regarding potential issues with blocking commands. |
| `unwrap`       | Prevent wrapping this chord, even if a global wrapper is set or inherited from a parent prefix.                               |
| `wrap`         | Wrap chord commands with the given string (supports interpolation). Overrides the global `wrap-cmd` setting.                  |
| `title`        | Set a title for the chord or prefix that is displayed above the menu (supports interpolation). Overrides the global `--title` setting. |

Each flag has a time and a place but I find `+keep`, and
`+write` to be the most useful out of the bunch.

The `+keep` flag can turn `wk` into a hydra of sorts. I use
this to control music playback on my system like this:

```
m "+Music" +keep
{
    c "Clear mpc" %{{mpc clear}}
    d "Display Song" %{{songinfo}}
    h "Seek -5" %{{mpc seek "-5"}}
    l "Seek +5" %{{mpc seek "+5"}}
    n "Next song" %{{mpc next}}
    p "Prev song" %{{mpc prev}}
    o "Open mpc" +close %{{st -e ncmpcpp}}
    y "Playlist" +close %{{st -e ncmpcpp --screen playlist}}
}
```

The `+write` flag is useful for scripting purposes. In the
same way that `dmenu` and co print selections to stdout,
this turns `wk` into a prompt for users to choose from some
list of options with less typing.

#### Command Wrapping

Command wrapping allows you to prefix all commands (or
specific groups of commands) with a common string. This is
useful for:

- Running commands in specific environments (containers,
  uwsm, etc.)
- Adding environment variables
- Prefixing with terminal emulators

**Global wrapper** (applies to all chords):

```
:wrap-cmd "uwsm-app --"
f "Firefox" %{{firefox}}
# Executes: /bin/sh -c "uwsm-app -- firefox"
```

The above example is equivalent to:

```
f "Firefox" %{{uwsm-app -- firefox}}
```

**Local wrapper** (applies to specific prefix or chord):

```
:wrap-cmd "uwsm-app --"
b "+Browse" +wrap "%(wrap_cmd) firefox"
{
    g "GNU" %{{gnu.org}}
    [
        (y "YouTube")
        (s "Soundcloud")
    ] "null" %{{%(desc,,).com}}
}
f "+Foot" +wrap "foot -e"
{
    n "ncmpcpp" %{{ncmpcpp}}
    # Executes: /bin/sh -c "foot -e ncmpcpp"
}
```

**Wrapper precedence** (most to least specific):

1. `+unwrap` on chord (no wrapper)
2. `+wrap "custom"` on chord
3. `+wrap "custom"` inherited from parent prefix
4. Global `:wrap-cmd` / `--wrap-cmd` / config.h
5. No wrapper

**Note:** Wrapping does not apply to hooks.

#### Inheritance

Inheritance relates to hooks and flags given to prefixes.
The idea is fairly simple. A hook or flag given to a prefix
is inherited by any chord within the prefix. Nested prefixes
do not inherit the hooks and flags given to their parent.

```
a "+Prefix" +write
{
    w "Write it!" %{{I get written!}}
    n "+Nested Prefix"
    {
        r "Run it!" %{{echo "I get run!"}}
    }
}
```

In the above example, the key chord `a w` causes `I get
written!` to be printed to stdout. The key chord `a n r`
runs the command `echo "I get run!"`.

To force a nested prefix to inherit from its parent the
`+inherit` flag must be given. Additionally, if the prefix
only wishes to inherit certain hooks or flags additional
flags may be given to ignore unwanted behavior.

#### Sorting

Key chords are sorted by default when processing a `wks`
file. Index interpolations are resolved *before* sorting,
so `%(index)` reflects parse order, not final sorted
position. Sorting only changes display order, not index
values.

```
# Base file
[neio] "Switch %(index+1)" %{{xdotool set_desktop %(index)}}
b "Second?" +write %{{%(index)}}
a "First?" +write %{{%(index)}}

# Result (sorted by default, but indices reflect parse order)
a "First?" +write %{{5}}
b "Second?" +write %{{4}}
e "Switch 2" %{{xdotool set_desktop 1}}
i "Switch 3" %{{xdotool set_desktop 2}}
n "Switch 1" %{{xdotool set_desktop 0}}
o "Switch 4" %{{xdotool set_desktop 3}}
```

The array `[neio]` expands to indices 0-3 in parse order
(n=0, e=1, i=2, o=3), then `b` gets index 4 and `a` gets
index 5. After sorting, the display order changes but
indices remain unchanged.

To disable sorting, use the `--unsorted` CLI flag or the
`:unsorted` preprocessor macro.

## Preprocessor Macros

There are a number of preprocessor macros that can be used
in `wks` files. These have a number of uses from making
`wks` files more modular to controlling the look and feel of
`wk`.

```
preprocessor_macro -> ':' ( string_macro
                          | switch_macro
                          | integer_macro
                          | unsigned_macro
                          | number_macro ) ;
```

A preprocessor macro begins with the colon character (`:`)
followed by a specific macro form.

The majority of macros correspond to the command-line
arguments that `wk` supports. When given, these override
anything given at the command-line. They are here to provide
a baked-in alternative to the command-line versions making
it easy to simply run the `wks` file and get the desired
look and feel without having to give the same arguments each
time. It can also help distinguish the purpose of the key
chords if it is intended to be used as part of a script by
making the `wk` popup window different from the builtin
settings.

### String Macros

String macros require a string argument.

```
string_macro -> ( 'include'
                | 'fg-color'
                | 'bg-color'
                | 'bd-color'
                | 'fg-key'
                | 'fg-delimiter'
                | 'fg-prefix'
                | 'fg-chord'
                | 'fg-title'
                | 'shell'
                | 'font'
                | 'title'
                | 'title-font'
                | 'delimiter'
                | 'wrap-cmd'
                | 'var' '"' ( '\\"' | [^"] | user_variable )* '"' ) '"' ( '\\"' | [^"] | user_variable )* '"' ;
```

Many of the macros here work the same as their command-line
counterparts. Simply use `:MACRO "ARGUMENT"` to make use of
any string macro, (e.g. `:shell "/usr/bin/env zsh"`).

**Note** All string macro arguments support user variable
interpolation using the `%(variable_name)` syntax. This
allows you to use variables defined with `:var` in any
string macro argument, including `:include`, `:font`,
`:title-font`, `:title`, `:fg-color`, `:fg-key`,
`:fg-delimiter`, `:fg-prefix`, `:fg-chord`, `:fg-title`,
`:bg-color`, `:bd-color`, `:shell`, `:delimiter`, and
`:wrap-cmd`. See [The Var Macro](#the-var-macro) for examples
of using variables in preprocessor directives.

#### The Include Macro

Out of the string macros, the `:include` macro is not present
as a command-line argument to `wk`. This is because this
macro has more to do with `wks` files than the look and feel
of `wk`.

The `:include` macro works similarly to the `#include` macro
found in C/C++. It allows users to bring other `wks` files
into a single file.

**Note**, self includes and recursive includes are not
permitted and will cause an error.

**Note**, the same file may be included multiple times. This
is not an error, and may even be desirable for some users.

Here is an example of the `include` macro:

```
# File main.wks
---------------
# Browser prefix
b "+Browser" { :include "browser_key_chords.wks" }
# Emacs prefix
e "+Emacs" ^before %{{xdotool set_desktop 1}} { :include "emacs_key_chords.wks" }
# Music prefix
m "+Music" +keep { :include "music_key_chords.wks" }

# File browser_key_chords.wks
-----------------------------
[
    (b "Brave")
    (c "Chrome")
    (f "Firefox")
] "null" %{{%(desc,,)}}

# Mullvad-exclude prefix
m "+Mullvad Exclude"
{
    [
        (b "Brave")
        (c "Chrome")
        (f "Firefox")
    ] "null" %{{mullvad-exclude %(desc_)}}
}

# File emacs_key_chords.wks
---------------------------
b "Open blank" %{{emacsclient -c -a ""}}
p "+Projects"
{
    w "wk" %{{emacs "~/Projects/wk"}}
}

# File music_key_chords.wks
---------------------------
c "Clear mpc" %{{mpc clear}}
d "Display song" %{{songinfo}}
h "Seek -5s" %{{mpc seek "-5"}}
l "Seek +5s" %{{mpc seek "+5"}}
n "Next song" %{{mpc next}}
p "Prev song" %{{mpc prev}}
o "Open mpc" +close %{{st -e ncmpcpp}}
```

This allows users to create key chords in a more modular
manner. This can be beneficial when you may want to reuse a
`wks` file in a different context than your main key chords.

**Note**, while the `#include` macro in C/C++ has
restrictions  on where it can go in a file, the`:include`
macro in a `wks` file may go literally anywhere. In the
above example, this was given in the middle of a prefix
without error.

You can even do silly things like this:

```
# File part_one.wks
-------------------
A "silly :include "part_two.wks"

# File part_two.wks
-------------------
example" %{{echo "You wouldn't do this right??"}}

# Resulting wks file
--------------------
A "silly example" %{{echo "You wouldn't do this right??"}}
```

As for file resolution, it's pretty simple. A relative path
is assumed to be in the same directory as the file being
executed,  and absolute paths are just that, absolute.

#### The Var Macro

The `:var` macro allows you to define a variable with some
value. It takes two arguments, the first is the `key` or
`variable name`, and the second is the `value` which can be
an empty string, i.e., unset a previously defined `var`.

**Note** Both arguments support variable interpolation,
enabling meta-variables (variables with computed names) and
variables that reference other variables.

The `key` can contain any character except a closing
parenthesis ('`)`'), otherwise it would be inaccessible
through interpolation. The `key` cannot shadow builtin chord
metadata identifiers.

##### Basic Variables

The simplest use case is defining a variable and using it in
chord descriptions or commands:

```
:var "WORKSPACE_CMD" "hyprctl dispatch workspace"
[asdfghjkl] "Workspace %(index+1)" %{{%(WORKSPACE_CMD) %(index)}}
```

##### Meta-Variables

Since variable names support interpolation, you can create
variables with computed names:

```
# Variable name from another variable
:var "key_name" "GREETING"
:var "%(key_name)" "Hello, World!"
a "Say hello" %{{echo %(GREETING)}}

# Multi-part variable names
:var "prefix" "MY"
:var "suffix" "VAR"
:var "%(prefix)_%(suffix)" "success"
b "Test" %{{echo %(MY_VAR)}}
```

**Note** Variables used in the name must be defined before
the `:var` directive that uses them.

##### Variables Referencing Variables

Variable values can reference other variables:

```
:var "original" "first"
:var "derived" "%(original)"
# derived now contains "first"

:var "base" "foo"
:var "extended" "%(base)_bar"
# extended now contains "foo_bar"
```

Variables are resolved at definition time, so the referenced
variables must be defined first.

##### Variables in Preprocessor Directives

All preprocessor macros that take string arguments support
variable interpolation:

```
# Color scheme composition
:var "r" "ff"
:var "g" "00"
:var "b" "00"
:fg "#%(r)%(g)%(b)"

# Font configuration
:var "font_name" "Monospace"
:var "font_size" "12"
:font "%(font_name), %(font_size)"

# Shell configuration
:var "shell_path" "/bin/bash"
:shell "%(shell_path)"

# Wrap command
:var "wrapper" "uwsm-app --"
:wrap-cmd "%(wrapper)"
```

##### Environment-Agnostic Configurations

Variables enable writing configuration files that work
across different environments:

```
# File main.wks
w "+Workspace"
{
    [asdfghjkl] "Workspace %(index+1)" %{{%(WORKSPACE_CMD) %(index)}}
}

# File hyprland.wks
:var "WORKSPACE_CMD" "hyprctl dispatch workspace"
:include "main.wks"

# File dwm.wks
:var "WORKSPACE_CMD" "xdotool set_desktop"
:include "main.wks"
```

With clever use of variables, you can construct general `wk`
menus that work across different window managers or
environments without modification.

##### Variable Resolution Order

Variables are resolved at definition time in a single pass:
1. Variable names are resolved first (meta-variables)
2. Variable values are then resolved
3. Undefined variables in either context cause an error

This prevents circular references but requires that
variables be defined before they are used.

### Switch Macros

Switch macros are the simplest of the bunch. They are
essentially an on switch for the corresponding menu
settings.

```
switch_macro -> ( 'debug'
                | 'unsorted'
                | 'top'
                | 'bottom' );
```

All the switch macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.scd) page
for more info.

### Integer Macros

The integer macros require a positive or negative integer
argument to the macro.

```
integer_macro -> ( 'menu-width'
                 | 'menu-gap'
                 | 'table-padding' ) '-'? [0-9]+ ;
```

All the integer macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.scd) page
for more info.

### Unsigned Macros

The unsigned macros require a positive integer argument to
the macro.

```
unsigned_macro -> ( 'max-columns'
                  | 'border-width'
                  | 'width-padding'
                  | 'height-padding'
                  | 'delay'
                  | 'keep-delay' ) [0-9]+ ;
```

All the unsigned macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.scd) page
for more info.

### Number Macros

The number macros require a positive number argument to
the macro.

```
number_macro -> ( 'border-radius' ) '-'? [0-9]+ ( '.' [0-9]* )? ;
```

All the number macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.scd) page
for more info.

## Full documentation

The above should serve as a solid introduction to `wks` file
syntax. The [man](man/wks.5.scd) page for `wks` files
contains the same information. When `wk` is installed,
simply run `man 5 wks` to get refrence examples and a full
break down of `wks` syntax.

Additionally, there are several example files included in
the [`examples`](examples) section for testing and
understanding.

## `wks-mode` Emacs Package

There is also a [wks-mode](https://github.com/3L0C/wks-mode)
package for Emacs provides syntax highlighting, and proper
indentation in `wks` files. I'm no elisp wizard, if you have
any way to make that package better, please reach out.

# Acknowledgments

This project would not be where it is without
[dmenu](https://tools.suckless.org/dmenu/), and
[bemenu](https://github.com/Cloudef/bemenu).  I first tried
to hack `dmenu` into an which-key like abomination, but
failing to do that I looked to create my own solution.
However, I am not a programmer by trade, and my knowledge of
X11 and Wayland is very limited. It is thanks to those
projects that `wk` runs on either environment. `bemenu`
especially was a life saver. The code for the Wayland
runtime has been lightly addapted for use with `wk`. All
credit goes to the people who work on that project for the
code there.

# Contributing

Contributions are welcome! If you find any issues or have
suggestions for improvements, please open an issue or submit
a pull request.
