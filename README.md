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

```c
/* Delimiter when displaying chords. */
static const char* delimiter = " -> ";
/* Max number of columns to use. */
static const uint32_t maxCols = 4;
/* Window width. Set to '-1' for 1/2 the width of your screen. */
static const int32_t windowWidth = -1;
/* Window gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */
static const int32_t windowGap = -1;
/* X-Padding around key/description text in cells. */
static const uint32_t widthPadding = 6;
/* Y-Padding around key/description text in cells. */
static const uint32_t heightPadding = 2;
/* Position to place the window. '0' = bottom; '1' = top. */
static const uint32_t windowPosition = 0;
/* Window border width */
static const uint32_t borderWidth = 4;
/* Window border radius. 0 means no curve */
static const double borderRadius = 0;
/* Window foreground color */
static const char* foreground = "#DCD7BA";
/* Window background color */
static const char* background = "#181616";
/* Window border color */
static const char* border = "#7FB4CA";
/* Default shell to run chord commands with. */
static const char* shell = "/bin/sh";
/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */
static const char* font = "monospace, 14";
```

## wks Files

The `wks` syntax is what I believe sets `wk` apart from the
small list of alternatives (not to mention most don't show
the key chords in a popup menu). So what is this `wks` file
type that I keep talking about? Let me show you.

```
# This is a comment
    # This is also a comment

# This is a chord.
# The trigger key is 'a'.
# The description is "A chord".
# The command is 'echo "Hello, World"'.
a "A chord" %{{echo "Hello, world!"}}

# This is a prefix.
# The trigger key is 'Control + a'
C-a "A prefix"
{
    # This is a chord that can only be accessed after triggering the parent prefix.
    b "A chord" %{{echo "Hello from inside prefix 'C-a'"}}

    # Prefixes can nest additional prefixes arbitrarily deep
    c "Another prefix"
    {
        d "Done" %{{echo "You've reached the end!"}}
    }
}
```

I know, very cool. But it gets better. I use `dwm` with a
patch that lets me control windows, and run other functions
from the command-line. That on its own is very handy, but
with a `wks` file, I can have something like this to take
advantage of that conveniece.

```
# Window prefix
w "+Window" 
{
    t "Tag" +keep
    {
        [arstgmnei] "View tag %(index+1)" %{{dwmc viewex %(index)}}
    }
    # Layout prefix
    p "+Layout" +keep
    {
        [
            (b "TTT Bstack") 
            (c "|M| Centered")
            (C ">M> Center float")
            (d "[D] Deck")
            (D "[\\] dwindle")
            (f "><> Floating")
            (t "[]= Tile")
        ] "(null)" %{{dwmc setlayoutex %(index)}}
    }
}
```

That introduced a lot of stuff. Lets look at what's going
on.

### Flags 

The first new bit of syntax is seen in `+keep`. This is a
**flag**. There are many flags that effect the behavior of
`wk`. For a thorough explanation please see the
[flags](man/wks.5.org#flag) section of the man page.

### Chord Arrays and Interpolations 

Flags are great, and so is the `chord array` syntax. This
allows users to group similar chords together and cut down
on repetition. Taking a closer look at key chord `w t` there
are two ways the chords in the prefix could have been
written. 

```
# The simple way
[arstgmnei] "View tag %(index+1)" %{{dwmc viewex %(index)}}

# The verbose way
a "View tag 1" %{{dwmc viewex 0}}
r "View tag 2" %{{dwmc viewex 1}}
s "View tag 3" %{{dwmc viewex 2}}
t "View tag 4" %{{dwmc viewex 3}}
g "View tag 5" %{{dwmc viewex 4}}
m "View tag 6" %{{dwmc viewex 5}}
n "View tag 7" %{{dwmc viewex 6}}
e "View tag 8" %{{dwmc viewex 7}}
i "View tag 9" %{{dwmc viewex 8}}
```

Both produce equivelent result in the popup menu. This is
handy when you have commands that vary in minor ways, and it
leverages [interpolations](man/wks.5.org#interpolation).
There are a number of interpolations offered to users, check
'em out.

### Chord Expressions in Chord Arrays

So the weirdest thing about this example is probably the
`chord expressions`. This syntax is only supported with a
chord array, but it lets users fit those stubbornly unique
key chords into a chord array that may or may not be
relatively simple. 

Here, the common thread is the command being run by the
chords with the only variation being the descriptions, and
the argument to the command. However, the chord expression
syntax supports more than just a unique description. Users
can specify flags, hooks, and even a command that is unique
to that chord. Whatever the chord expression is missing,
aside from the description, is filled in with the bits from
the chord array.

### Probably Should Mention Special Keys

There is a lot I could say about this but I really should
mention `special keys`. 

### Full documentation

The above is useful as quick and dirty introduction to the
`wks` syntax. For complete details, see [man](man/wks.5.org)
page here in this repo, or through `man 5 wks` if you have
installed `wk`. 

Additionally, there are several example files included in
the examples section for testing and understanding. 

There is also a [wks-mode.el]() package for emacs that is a
work in progress but currently provides syntax highlighting,
and proper indentation of `wks` files.  I'm no elisp wizard,
if you have any way to make that package better, please
reach out.

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
