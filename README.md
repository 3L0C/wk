wk
========

`wk` - which-key via X11 and Wayland. 
Displays available key chords in a popup window. 
Inspired by 
[emacs-which-key](https://github.com/justbur/emacs-which-key), 
[dmenu](https://tools.suckless.org/dmenu/), and 
[bemenu](https://github.com/Cloudef/bemenu). 

![wk.png](./wk.png)

## Introduction

`wk` offers users a portable, scriptable, and highly
customizable interface for their key chord mappings through
a number sources. Key chords can be built into the binary
via the [key_chords.def.h](config/key_chords.def.h) header, read
from a [wks file](#wks-Files), or read from stdin with the
same `wks` syntax.

## Building 

``` sh
# Make wk for X11 and Wayland
make 

# Make wk for X11
make x11

# Make wk for wayland
make wayland

# Install 
make clean && make && sudo make install
```

## Dependencies

- C compiler 

All runtime dependencies below are searched with
`pkg-config`. 

| Backend | Dependencies                      |
|---------|-----------------------------------|
| Common  | cairo, pango, pangocairo          |
| X11     | x11, xinerama                     |
| Wayland | wayland-client, wayland-protocols |


## About Wayland support

Wayland is only supported by compositors that implement the
[wlr-layer-shell](https://gitlab.freedesktop.org/wlroots/wlr-protocols/tree/master/unstable)
protocol. Typically
[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots/)-based 
compositors. For those not on a `wlroots`,
[Xwayland](https://wayland.freedesktop.org/xserver.html) 
does work based on my testing, but the popup menu seems to
only display on one screen.

## Usage

``` sh
# Display builtin key chords.
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
    -S, --sort                 Sort key chords read from --key-chords, or --script.
    -m, --max-columns INT      Set maximum columns to INT.
    -p, --press KEY(s)         Press KEY(s) before dispalying window.
    -T, --transpile FILE       Transpile FILE to valid 'key_chords.h' syntax and print to stdout.
    -k, --key-chords FILE      Use FILE for key chords rather than those precompiled.
    -w, --window-width INT     Set window width to INT.
    -g, --window-gap INT       Set window gap between top/bottom of screen to INT.
                               Set to '-1' for a gap equal to 1/10th of the screen height.
    --border-width INT         Set border width to INT.
    --border-radius NUM        Set border radius to NUM.
    --wpadding INT             Set left and right padding around hint text to INT.
    --hpadding INT             Set up and down padding around hint text to INT.
    --fg COLOR                 Set window foreground to COLOR (e.g., '#F1CD39').
    --bg COLOR                 Set window background to COLOR (e.g., '#F1CD39').
    --bd COLOR                 Set window border to COLOR (e.g., '#F1CD39').
    --shell STRING             Set shell to STRING (e.g., '/bin/sh').
    --font STRING              Set font to STRING. Should be a valid Pango font description
                               (e.g., 'monospace, M+ 1c, ..., 16').

run `man 1 wk` for more info on each option.
```

Above are some example uses of `wk`. A full list of
options can be had with the `--help` flag, not to mention
the `wks` file syntax that will be covered below.

## Configuration 

`wk` can be configured at the command line as show in the
above help message, or your configuration can be built into
the binary by changing the settings in
[config.def.h](config/config.def.h). 

## wks Files

Which-Key source (`wks`) are the driving force behind `wk`.
The syntax is novel, but provides a flexible means to manage
and organize key chords. Below is a brief introduction to
get users up and running. For a deep dive see the
[man](man/wks.5.org) page.

### The Basics

A key chord is some series of keys that must be pressed in
order to execute some action. In a `wks` file the most basic
key chord includes one key, a description, and a command to
execute.

```
a "A simple key chord" %{{echo "Hello, World!"}}
```

A key chord that does not result in some command being
executed is a prefix. A prefix is one common part of
multiple individual key chords. In a `wks` file a prefix has
a key, a description, and a block of one or more key chords.

```
a "+A prefix" 
{
    b "A chord" %{{echo "It takes two keys to reach me!"}}
    c "+More key chords"
    {
        d "A useful description" %{{echo "It takes three keys to reach me!"}}
    }
}
```

This is all pretty standard when it comes to key chords, but
`wks` files can get a bit different. Let's start with hooks
and flags.

### Hooks

One new concept that `wks` files offer for key chords is the
use of hooks and flags. A hook is a command that can be run
before or after a key chords command. In a `wks` file they
go between the description and the key chord command, like
this:

```
h "Hooks galore!" ^before %{{echo "I run first!"}} ^after %{{echo "I run third!"}}
    %{{echo "I run second!"}}
```

In the above example when the key chord `h` is used, `wk`
will run the before hook, the command, and then the after
hook for the chord. By default these are all run
asynchronously to prevent a hook or command from interfering
with `wk` in the case where the command may not complete.

For a full overview see the [hooks](man/wks.5.org#hook)
section of the man page.

### Flags

While hooks add commands to key chords, flags change the
behavior of `wk`. In a `wks` file they also go between the
description and the command. Hooks and flags have the same
precedence, meaning they can be given before or after one
another without issue, so long as they are all between the
description and command. Here are a few hooks that I use
most often.

```
n "Next" +keep %{{mpc next}}
p "Previous" +keep %{{mpc previous}}
w "Write" +write %{{This doesn't look like a shell command...}} 
```

The two flags above are the `+keep` and `+write` flags. The
`+keep` flag keeps `wk` open after the key chord is
triggered. This is useful when you may want to repeat the
key chord without having to start from the begining. The
second flag is the `+write` flag. Normally, everything
between the `%{{` and `}}` delimiters is run as a shell
command. However, you may want to simply write this
information to the console instead. I find this handy for
scripts as a sort of `dmenu` altrenative. I have some
[examples](#examples) below that feature the `+write` flag.

The full list of flags is covered in the
[man](man/wks.5.org#flag) page.

### Hooks and Flags with Prefixes

Hooks and flags can also be given to prefixes. To give a
hook or flag to a prefix, simply add it after the
description and before the opening brace `{` like this:

```
m "+Music" +keep 
{
    n "Next" %{{mpc next}}
    p "Previous" %{{mpc previous}}
}
```

When a hook or flag is given to a prefix it propagates to
all chords within the block. If the block contains any
prefixes, the hooks and flags will not propagate to these by
default. This is a shallow inheritance that can be
overridden with the `+inherit` flag for prefixes that wish
to propagate their parent's hooks and flags to the next
block. 

See [inheritance](man/wks.5.org#inheritance) for more
information. 

### Comments and Special Symbols

Comments are normal part of most configuration files. `wks`
supports comments with the `#` character. After the `#` the
rest of the line will be treated as a comment until the end
of the line. The only caveat to this is that `#` symbols
inside descriptions and commands are not interpreted as
comments. Everywhere else they are interpreted as the start
of a comment. To use the `#` symbol as a trigger key, simply
add a `\` before the `#`. The same can be done for any of
the special characters: `[]{}#":^+()`. 

### Key Syntax 

Keys, or trigger keys, are a pretty important part of key
chords. In a `wks` file any utf8 character can be
interpreted as a key. The only exceptions are the special
characters: `[]{}#":^+()`. To use these as a key simply add
a `\` infront of the character. 

#### Special keys

While many keys are simple keys you would normally type, not
all are like this. Thankfully, `wks` supports the following
special keys in the corresponding forms:

```
# Specail key = wks form
Left arrow = Left
Right arrow = Right
Up arrow = Up
Down arrow = Down

Tab key = TAB
Space bar = SPC
Enter/return key = RET
Delete key = DEL
Esc key = ESC

Home key = Home
Page up key = PgUp
Page down key = PgDown
End key = End
Begin key = Begin
```

I plan to add additional special keys like the `F` keys as
well as volume and brigthness control, but that is still a
work in progress.

#### Modifiers 

Many people use modifiers with trigger keys. These are
supported in `wks` files via `C-`, `A-`, `H-`, and `S-`
which correspond to `Control`, `Alt`, `Hyper` or `Super`,
and `Shift`. Multiple modifiers can be given to a trigger
key like so:

```
C-A-H-A "That's a lot of keys" %{{echo "Was it worth it?"}}
```

The above key chord is triggered when a user presses
`Control + Alt + Hyper + Shift + a`. The notable point about
modifiers is that the `S-` modifier is only considered by
`wk` when given to a special key. Aside from those keys
users should use the shifted version of their trigger key as
shown here. I'm open to changing this behavior in the future.

### Chord Arrays and Interpolations 

#### Chord Arrays

Some chords only vary slightly one from another. To make
these chords easier to express, `wks` files support chord
arrays.

```
[arstgmnei] "Tag %(index+1)" %{{dwmc %(index)}}
```

The above is equivelent to the following:

```
a "Tag 1" %{{dwmc 0}}
r "Tag 2" %{{dwmc 1}}
s "Tag 3" %{{dwmc 2}}
t "Tag 4" %{{dwmc 3}}
g "Tag 5" %{{dwmc 4}}
m "Tag 6" %{{dwmc 5}}
n "Tag 7" %{{dwmc 6}}
e "Tag 8" %{{dwmc 7}}
i "Tag 9" %{{dwmc 8}}
```

#### Interpolations 

Chord arrays often go hand in hand with another `wks`
feature, interpolations. Interpolations may be given in
descriptions and commands to make use of meta information
about the chord itself. An interpolation begins with `%(`
and ends with `)`. Only recognized identifiers should go
inbetween these delimiters. For a full list of supported
identifiers please see the [man](man/wks.5.org#IDENTIFIER)
page.

The above example demonstrated the `%(index)` and
`%(index+1)` interpolations. These correspond to the 0 and 1
based index of each chord respectively. In this example the
only chords are the ones shown, but every chord in a block
has an index. 

You may have noticed that the equivelent example is not
sorted. The default behavior for `wk` is to not sort the key
chords in `wks` files in order to not conflict with user
expectations about the value of `%(index[+1])`
interpolations. Had sorting been used the above example
would have been equivelent to this: 

```
a "Tag 1" %{{dwmc 0}}
e "Tag 2" %{{dwmc 1}}
g "Tag 3" %{{dwmc 2}}
i "Tag 4" %{{dwmc 3}}
m "Tag 5" %{{dwmc 4}}
n "Tag 6" %{{dwmc 5}}
r "Tag 7" %{{dwmc 6}}
s "Tag 8" %{{dwmc 7}}
t "Tag 9" %{{dwmc 8}}
```

For a better understanding see the
[sorting](man/wks.5.org#sorting) section of the man page.

### Full documentation

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

## Acknowledgments 

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

## Contributing

Contributions are welcome! If you find any issues or have
suggestions for improvements, please open an issue or submit
a pull request.
