wk
========

`wk` - which-key via X11 and Wayland. 
Displays available key chords in a popup window. 
Inspired by 
[emacs-which-key](https://github.com/justbur/emacs-which-key), 
[dmenu](https://tools.suckless.org/dmenu/), and 
[bemenu](https://github.com/Cloudef/bemenu). 

![wk.png](./wk-which-key.png)

# Introduction

`wk` offers users a portable, scriptable, and highly
customizable interface for their key chord mappings through
a number of sources. Key chords can be built into the binary
via the [key_chords.def.h](config/key_chords.def.h) header,
read from a [wks file](#wks-Files), or read from stdin with
the same `wks` syntax.

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

All runtime dependencies below are searched with
`pkg-config`. 

| Backend | Dependencies                                |
|---------|---------------------------------------------|
| Common  | cairo, pango, pangocairo                    |
| X11     | x11, xinerama                               |
| Wayland | wayland-client, wayland-protocols xkbcommon |

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

# Everything else
wk --help 
usage: wk [options]

options:
    -h, --help                 Display help message and exit.
    -v, --version              Display version number and exit.
    -d, --debug                Print debug information.
    -t, --top                  Position window at top of screen.
    -b, --bottom               Position window at bottom of screen.
    -s, --script               Read script from stdin to use as key chords.
    -S, --sort                 Sort key chords read from --key-chords, --script, or --transpile.
    -m, --max-columns INT      Set maximum columns to INT.
    -p, --press KEY(s)         Press KEY(s) before dispalying window.
    -T, --transpile FILE       Transpile FILE to valid 'key_chords.h' syntax and print to stdout.
    -k, --key-chords FILE      Use FILE for key chords rather than those precompiled.
    -w, --window-width INT     Set window width to INT.
    -g, --window-gap INT       Set window gap between top/bottom of the screen to INT.
                               Set to '-1' for a gap equal to 1/10th of the screen height.
    --border-width INT         Set border width to INT.
    --border-radius NUM        Set border-radius to NUM.
    --wpadding INT             Set left and right padding around hint text to INT.
    --hpadding INT             Set top and bottom padding around hint text to INT.
    --fg COLOR                 Set window foreground to COLOR (e.g., '#F1CD39').
    --bg COLOR                 Set window background to COLOR (e.g., '#F1CD39').
    --bd COLOR                 Set window border to COLOR (e.g., '#F1CD39').
    --shell STRING             Set shell to STRING (e.g., '/bin/sh').
    --font STRING              Set font to STRING. Should be a valid Pango font description
                               (e.g., 'monospace, M+ 1c, ..., 16').

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
manage and express key chords. Below is an introduction to
the `wks` syntax to get users up and running. For a deep
dive, see the [man](man/wks.5.org) page.

## Comments

In `wks` files, comments can be added using the `#`
character. When a `#` is encountered, it signifies the start
of a comment. The comment extends from the `#` character
until the end of the line. It's important to note that the
`#` character is treated as a literal character within
descriptions and commands and does not indicate the start of
a comment in those contexts.

## Key Chords

```
key_chord -> ( chord | prefix | chord_array ) ;
```

The `key_chord` is the main building block of a `wks` file.
This can be either a `chord`, `prefix`, or a `chord_array`.
The chord is the most basic example of a key chord and
serves as a good entry point for this discussion.

### Chords 

A chord is a key chord that results in `wk` performing some
action, like executing a command, when the trigger key is
pressed. The formal grammar looks like this:

```
chord -> key description keyword* command ;
```

All chords must have a `key`, `description`, and a
`command`. Zero or more `keyword`s may be given. These will
be addressed later. For now, let's break down the required
parts of the chord.

#### Keys

A key, or trigger key, represents the specific keypress or
key combination that triggers a corresponding action or
command. In a `wks` file, it is the written representation
of the physical key(s) pressed by the user on their
keyboard. The grammar looks like this:

```
key -> modifier* ( '\\'[\\\[\]{}#":^+()] 
                 | [^\s\[\]{}#":^+()] 
                 | special_key ) ;
```

A key is then zero or more `modifiers` followed by an
escaped character with special meaning, any non-whitespace,
printable utf8 character, or a `special_key`. 

The characters `\`, `[`, `]`, `{`, `}`, `#`, `"`, `:`, `^`,
`+`, `(`, and `)` have special meanings in `wks` files. To
use any of these as a key, simply precede them with a
backslash `\`.

All other non-whitespace, printable utf8 characters prior to
a description will be interpreted as a key. Those that are
whitespace or non-printable fall into the `special_key`
category.

#### Special Keys

Special keys like `tab`, `escape`, `spacebar`, and `F1` can
still be used as trigger keys in `wks` files with the
following special forms:

| Special Key    | Representation in `wks` |
|----------------|-------------------------|
| Left arrow     | `Left`                  |
| Right arrow    | `Right`                 |
| Up arrow       | `Up`                    |
| Down arrow     | `Down`                  |
| Tab            | `TAB`                   |
| Space          | `SPC`                   |
| Enter/Return   | `RET`                   |
| Delete         | `DEL`                   |
| Esc            | `ESC`                   |
| Home           | `Home`                  |
| Page up        | `PgUp`                  |
| Page down      | `PgDown`                |
| End            | `End`                   |
| Begin          | `Begin`                 |
| F[1-35]        | `F[1-35]`               |
| Volume Down    | `VolDown`               |
| Mute Vol       | `VolMute`               |
| Volume Up      | `VolUp`                 |
| Play Audio     | `Play`                  |
| Stop Audio     | `Stop`                  |
| Audio Previous | `Prev`                  |
| Audio Next     | `Next`                  |

In `wks` files, whitespace is generally not significant
around individual parts of the syntax, with one notable
exception: special keys. When using special keys, it is
recommended to include whitespace between the end of the
special key and the start of the next item in the `wks`
file.

If you have any additional special keys that you would like
`wks` files to support, please open an issue or a pull
request.

#### Modifiers

As mentioned above, zero or more modifiers can be given in a
key. The following modifiers are recognized with the
corresponding forms in a `wks` file: 

| Modifier    | Representation in `wks` |
|-------------|-------------------------|
| Control     | `C-`                    |
| Alt         | `M-`                    |
| Hyper/Super | `H-`                    |
| Shift       | `S-`                    |

Modifiers act as one would expect. To match the keypress
`Control+c` use the form `C-c` in your `wks` file.

Among the modifiers, the Shift modifier (`S-`) has a unique
behavior when used with non-special key characters. Due to
the way keys are interpreted, the `S-` modifier is not
always necessary for these characters. To determine whether
`S-` is required, it is recommended to test the character in
a `wks` file by typing it with and without the Shift key
pressed.

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
command -> '%{{' ( . | interpolation )* '}}' ;
```

A command begins with the `%{{` delimiter and ends with the
`}}` delimiter. Everything in between is taken as part of
the command for the key chord. There is no need to do
anything special here, just provide your shell command as
you would at the command line. 

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
prefix -> key description keyword* '{' ( key_chord )+ '}' ;
```

A prefix has many of the same components as a chord. It
begins with a key, followed by a description, zero or more
keywords and then a block of one or more key chords
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
chord_array -> '[' ( key | chord_expression )+ ']' description keyword* command ;
```

To use a chord array begin with an open bracket (`[`)
followed by one or more keys or chord expressions. The array
portion ends with a closing bracket (`]`) followed by the
standard chord components, a description, zero or more
keywords, and a command. 

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
chord_expression -> '(' key description keyword* command? ')' ;
```

A chord expression is only valid within a chord array, and
it is essentially a chord wrapped in parentheses with some
added flexibility. Normally, a chord requires at least a
key, a description, and a command. A chord expression, on
the other hand, requires only a key and a description. Any
other information will be filled in by the surrounding chord
array. 

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
(`)`). **Note** that interpolations can only be used in
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
    y "Playlist" +close %{{st -e ncmpcpp playlist}}
}
```

The `+write` flag is useful for scripting purposes. In the
same way that `dmenu` and co print selections to stdout,
this turns `wk` into a prompt for users to choose from some
list of options with less typing.

#### Inheritance 

Inheritance relating to hooks and flags given to prefixes is
fairly simple. A hook or flag given to a prefix is inherited
by any chord within the prefix. Nested prefixes do not
inherit the hooks and flags given to their parent. 

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
written!` to be printed to stdout. The key chord `a n e`
runs the command `echo "I get run!"`.

To force a nested prefix to inherit from its parent the
`+inherit` flag must be given. Additionally, if the prefix
only wishes to inherit certain hooks or flags additional
flags may be given to ignore unwanted behavior.

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

Out of the string macros, the `include` macro is not present
as a command-line argument to `wk`. This is because this
macro has more to do with `wks` files than the look and feel
of `wk`.

The `include` macro works similarly to the `#include` macro
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

## Full documentation

The above is useful as quick and dirty introduction to the
`wks` syntax. For complete details, see [man](man/wks.5.org)
page here in this repo, or through `man 5 wks` if you have
installed `wk`. 

Additionally, there are several example files included in
the examples section for testing and understanding. 

There is also a [wks-mode](https://github.com/3L0C/wks-mode)
package for emacs that is a work in progress but currently
provides syntax highlighting, and proper indentation in
`wks` files.  I'm no elisp wizard, if you have any way to
make that package better, please reach out.

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
