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

``` 
# Make wk for X11 and Wayland
make 

# Make wk for X11
make x11

# Make wk for wayland
make wayland

# Install 
make clean && make && sudo make install
```

# Dependencies

- C compiler 
- sed to cleanup man files

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
    -S, --sort                 Sort key chords read from --key-chords, --script,
                               or --transpile.
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
    --border-width INT         Set border width to INT (default 4).
    --border-radius NUM        Set border radius to NUM degrees. 0 means no curve
                               (default 0).
    --wpadding INT             Set left and right padding around hint text to
                               INT (default 6).
    --hpadding INT             Set top and bottom padding around hint text to
                               INT (default 2).
    --fg COLOR                 Set all menu foreground text to COLOR where color
                               is some hex string i.e. '#F1CD39' (default unset).
    --fg-key COLOR             Set foreground key to COLOR (default '#DCD7BA').
    --fg-delimiter COLOR       Set foreground delimiter to COLOR (default '#525259').
    --fg-prefix COLOR          Set foreground prefix to COLOR (default '#AF9FC9').
    --fg-chord COLOR           Set foreground chord to COLOR (default '#DCD7BA').
    --bg COLOR                 Set background to COLOR (default '#181616').
    --bd COLOR                 Set border to COLOR (default '#7FB4CA').
    --shell STRING             Set shell to STRING (default '/bin/sh').
    --font STRING              Set font to STRING. Should be a valid Pango font
                               description (default 'monospace, 14').

run `man 1 wk` for more info on each option.
```

# Configuration 

`wk` can be configured at the command line as shown in the
above help message, or your configuration can be built into
the binary by changing the settings in
[config.def.h](config/config.def.h). 

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
chord_array -> '[' ( trigger_key | chord_expression )+ ']' description keyword* command ;
```

To use a chord array begin with an open bracket (`[`)
followed by one or more trigger keys or
chord expressions. The array portion ends with a closing
bracket (`]`) followed by the standard chord components, a
description, zero or more keywords, and a command. 

I think an example will make things clear:

```
# Chord array version
[arstgmnei] "Switch workspace %(index+1)" %{{xdotool set_desktop %(index)}}

# Individual chords and no interpolation
a "Switch workspace 1" %{{xdotool set_desktop 0}}
r "Switch workspace 2" %{{xdotool set_desktop 1}}
s "Switch workspace 3" %{{xdotool set_desktop 2}}
t "Switch workspace 4" %{{xdotool set_desktop 3}}
g "Switch workspace 5" %{{xdotool set_desktop 4}}
m "Switch workspace 6" %{{xdotool set_desktop 5}}
n "Switch workspace 7" %{{xdotool set_desktop 6}}
e "Switch workspace 8" %{{xdotool set_desktop 7}}
i "Switch workspace 9" %{{xdotool set_desktop 8}}
```

As you can see, chord arrays can cut down on the need to
repeat common information across chords. However, this would
not be useful if the resulting chords were exactly the same.
Thankfully, interpolations make it easy for the resulting
chords to differ without interfering with the common
elements. 

Interpolations are covered in full detail later, but the
main idea is they provide a means of inserting metadata
about a chord into descriptions and commands. 

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
interpolation -> '%(' identifier ')' ;
```

The basic syntax for an interpolation begins with a `%(`
delimiter followed by an identifier and closing parenthesis
(`)`). 

**Note** that interpolations can only be used in
descriptions and commands.

The basic idea of interpolation is to provide users with
easy access to metadata about a chord. The following
identifiers are recognized in an interpolation along with
their corresponding metadata.

| Identifier | Metadata                                                                        |
|------------|---------------------------------------------------------------------------------|
| `key`      | The key portion of the chord.                                                   |
| `index`    | The base 0 index of the chord in the current scope (prefixes begin new scopes). |
| `index+1`  | The base 1 index of the chord in the current scope (prefixes begin new scopes). |
| `desc`     | The description of the current chord. May not be given within the description.  |
| `desc^`    | The description of the current chord with the first character capitalized.      |
| `desc^^`   | The description of the current chord with all characters capitalized.           |
| `desc,`    | The description of the current chord with the first character downcased.        |
| `desc,,`   | The description of the current chord with all characters downcased.             |

There are only a few identifiers that can be interpolated,
but even this small set makes `wk` more scriptable.

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

#### Flags

Flags are similar to command-line flags in that they change
the behavior of `wk`. 

```
flag -> '+' ( 'keep'
            | 'close'
            | 'inherit'
            | 'ignore'
            | 'ignore-sort'
            | 'unhook'
            | 'deflag'
            | 'no-before'
            | 'no-after'
            | 'write'
            | 'execute'
            | 'sync-command' ) ;
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
| `ignore-sort`  | Chord is ignored during sorting leaving it in it in the same position it was parsed in.                                       |
| `unhook`       | Ignore all hooks from the surrounding prefix.                                                                                 |
| `deflag`       | Ignore all flags from the surrounding prefix.                                                                                 |
| `no-before`    | Ignore `before` and `sync-before` hooks from the surrounding prefix.                                                          |
| `no-after`     | Ignore `after` and `sync-after` hooks from the surrounding prefix.                                                            |
| `write`        | Write commands to stdout rather than executing them.                                                                          |
| `execute`      | Execute the command rather than writing them to stdout. Useful when `+write` was given to a surrounding prefix.               |
| `sync-command` | Execute the command in a blocking fashion. See the note in [hooks](#hooks) regarding potential issues with blocking commands. |

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

Key chords will be sorted when processing a `wks` file if
the `--sort` flag is passed to `wk`. This has knock-on
effects with index interpolations (often for chord arrays).
A `wks` file like this will produce different results sorted
vs unsorted (the default).


```
# Base file
[neio] "Switch %(index+1)" %{{xdotool set_desktop %(index)}}
b "Second?" +write %{{%(index)}}
a "First?" +write %{{%(index)}}

# Unsorted result
n "Switch 1" %{{xdotool set_desktop 0}}
e "Switch 2" %{{xdotool set_desktop 1}}
i "Switch 3" %{{xdotool set_desktop 2}}
o "Switch 4" %{{xdotool set_desktop 3}}
b "Second?" +write %{{4}}
a "First?" +write %{{5}}

# Sorted result
a "First?" +write %{{0}}
b "Second?" +write %{{1}}
e "Switch 3" %{{xdotool set_desktop 2}}
i "Switch 4" %{{xdotool set_desktop 3}}
n "Switch 5" %{{xdotool set_desktop 4}}
o "Switch 6" %{{xdotool set_desktop 5}}
```

To avoid this you can add the `+ignore-sort` flag to any key
chord to ensure the value of the index interpolations.

```
# Base file
[neio] "Switch %(index+1)" +ignore-sort %{{xdotool set_desktop %(index)}}
b "Second?" +write %{{%(index)}}
a "First?" +write %{{%(index)}}

# Sorted with `+ignore-sort` result
n "Switch 3" %{{xdotool set_desktop 2}}
e "Switch 1" %{{xdotool set_desktop 0}}
i "Switch 2" %{{xdotool set_desktop 1}}
o "Switch 4" %{{xdotool set_desktop 3}}
a "First?" +write %{{4}}
b "Second?" +write %{{5}}
```

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
                | 'shell'
                | 'font' ) '"' ( '\\"' | [^"] )* '"' ;
```

Many of the macros here work the same as their command-line
counterparts. Simply use `:MACRO "ARGUMENT"` to  make use of
any string macro, (e.g. `:shell "/usr/bin/env zsh"`).

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

### Switch Macros 

Switch macros are the simplest of the bunch. They are
essentially an on switch for the corresponding menu
settings.

```
switch_macro -> ( 'debug'
                | 'sort'
                | 'top'
                | 'bottom' );
```

All the switch macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.org) page
for more info. 

### Integer Macros 

The integer macros require a positive or negative integer
argument to the macro.

```
integer_macro -> ( 'menu-width'
                 | 'menu-gap' ) '-'? [0-9]+ ;
```

All the integer macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.org) page
for more info. 

### Unsigned Macros 

The unsigned macros require a positive integer argument to
the macro. 

```
unsigned_macro -> ( 'max-columns'
                  | 'border-width'
                  | 'width-padding'
                  | 'height-padding' 
                  | 'delay' ) [0-9]+ ;
```

All the unsigned macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.org) page
for more info. 

### Number Macros 

The number macros require a positive number argument to
the macro. 

```
number_macro -> ( 'border-radius' ) '-'? [0-9]+ ( '.' [0-9]* )? ;
```

All the number macros correspond to their cli flags for
`wk`. See the help message or the [man](man/wk.1.org) page
for more info. 

## Full documentation

The above should serve as a solid introduction to `wks` file
syntax. The [man](man/wks.5.org) page for `wks` files
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
