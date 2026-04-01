# Getting Started

There are two steps required to start using wk:

1. Define your keybinds in a which key source (wks) file
2. Create a keybind in your Desktop Environment/Window
   Manager to launch wk

## Creating your first wks file

A wks file defines all the keybinds and settings for wk.
Here is a basic example:

```wks
# ~/main.wks
b "Brave" %{{brave}}
e "Emacs" %{{emacs}}
s "Spotify" %{{spotify}}
```

Assuming you saved the above example to `~/main.wks`, you
could test the file from your terminal by running:

```{prompt} bash
wk --key-chords ~/main.wks
```

To access these keybinds outside the terminal, you can
create a keybind to run the above command when you press
`SUPER+SPACE` in your desktop environment/window manager.
You can think of that keybind as the **leader key** in a
keybind sequence, or **key chord**. So, to access your wk
keybinds, you would press your **leader key** followed by
any of the wk keybinds you defined in `~/main.wks`. In this
example, you would have access to the following **key
chords**:

| Key Chord       | Result                         |
|-----------------|--------------------------------|
| `SUPER+SPACE b` | Opens the Brave browser        |
| `SUPER+SPACE e` | Opens the Emacs text editor    |
| `SUPER+SPACE s` | Opens the Spotify music client |

```{note}
These programs need to be available in your system [`$PATH`](https://en.wikipedia.org/wiki/PATH_(variable)) in order for wk to open them.
```

### A more useful example

The example above works, but you could achieve the same
thing with plain keybinds. Where wk shines is **nested key
chords** - organizing related actions under a common prefix:

```wks
# ~/main.wks
a "Apps"
{
    b "Brave" %{{brave}}
    s "Spotify" %{{spotify}}
}
e "Emacs"
{
    e "Emacs" %{{emacs}}
    r "Roam"
    {
        h "Home" %{{emacs ~/Documents/roam/20230620214854-home.org}}
        j "Journal" %{{emacs ~/Documents/roam/daily}}
    }
    w "wk" %{{emacs ~/Projects/wk}}
}
```

Now `SUPER+SPACE e r h` opens a specific file in emacs - a
key chord most desktop environments simply cannot express.
With wk, you sacrifice a single system keybind and gain
arbitrarily deep, discoverable key chords. And since your
entire workflow lives in a wks file, it's portable: install
wk, copy the wks file, bind a leader key, and you're set.

## The building blocks of a wks file

Let's break down the first example:

```wks
# ~/main.wks
b "Brave" %{{brave}}
```

**Comments** (`#`)
: Everything after `#` is ignored. Use them to annotate your
  wks files.

**Trigger key** (`b`)
: The key you press to activate the chord - a single
  character. See [trigger
  keys](../reference/wks.md#trigger-keys) for modifiers and
  special keys.

**Description** (`"Brave"`)
: The hint text shown in the wk menu (e.g., `b -> Brave`).
  Can be empty (`""`).

**Command** (`%{{brave}}`)
: The command wk executes, wrapped in `%{{` and `}}`. If it
  runs in your terminal, it works in a wks file. See the
  [delimiter](../reference/wks.md#delimiters) documentation
  if your command contains `}}`.

Together, a trigger key, description, and command form a
complete **chord**.

### Prefixes

The above example contains no **prefixes**, but they are a
core part of any wks file. Here is a modified example with a
prefix:

```wks
# ~/main.wks
a "Apps"
{
    b "Brave" %{{brave}}
}
```

Here, the "Apps" **key chord** is a prefix which contains
the "Brave" **chord**. A prefix may also contain other
prefixes, like the [second example](#a-more-useful-example)
above.

## Changing the look of the menu

You can alter the menu's look and feel through the wks file.
By default, your menu will look like this:

![default-menu](../_static/default-menu.png)

Add the following code:

```{code-block} wks
:emphasize-lines: 5-10

# ~/main.wks
# Laserwave theme:
#   Author https://github.com/Jaredk3nt/laserwave
#   doomemacs/themes port by github user @hyakt
:bg "#27212E"
:fg-key "#74DFC4"
:fg-delimiter "#4E415C"
:fg-chord "#40B4C4"
:bd "#EB64B9"
:border-radius 20
b "Brave" %{{brave}}
e "Emacs" %{{emacs}}
s "Spotify" %{{spotify}}
```

and everything changes:

![laserwave-menu](../_static/laserwave-menu.png)

### Preprocessor macros

We added [preprocessor
macros](../reference/wks.md#preprocessor-macros) above. You
can use these to alter the look and feel of the menu. The
ones we added above altered the menu colors and border
radius, but there are other macros. For example:

- Use `:top` to display the menu at the top of your screen
  instead of the bottom

- Use `:delay 500` to delay the popup menu by **half a second**
  instead of the default **one second** delay

```{note}
Some macros take an argument while others are simple switches to turn some behavior on or off. Please consult the [full documentation](../reference/wks.md#preprocessor-macros) for the specifics of each and for a complete list of all macros supported.
```

## Modifying key chords with flags

A **flag** modifies the behavior of a **prefix** or
**chord**, making it possible to do more than just executing
commands. Here are two flags you might find useful.

### The `+keep` flag

The `+keep` flag causes a completed chord to keep the menu
open instead of closing like normal. Here is an example of
the `+keep` flag:

<video autoplay loop muted playsinline controls width="100%">
  <source src="../_static/demo-keep-flag.webm" type="video/webm">
</video>

And here's the wks code:

```wks
m "Move window" +keep
{
    a "Workspace 1" %{{send-to-workspace 0}}
    s "Workspace 2" %{{send-to-workspace 1}}
    d "Workspace 3" %{{send-to-workspace 2}}
}
```

As you can see, the windows move to a different workspace
but the menu stays open. All you have to press is
`SUPER+SPACE m a a a`. Without the `+keep` flag, you would
need to press `SUPER+SPACE m a` for every window you want to
move. To close the menu, you can press any key that is not
bound in the wks file, like `ESCAPE` or `q`.

### The `+write` flag

`+write` is another useful flag, especially when using wk in
a shell script. Here is a simple example:

<video autoplay loop muted playsinline controls width="100%">
  <source src="../_static/demo-script.webm" type="video/webm">
</video>

Here is the script:

```bash
#!/usr/bin/env bash
# Prompt to play my favorite song :)

wks_source='
y "Yes" +write %{{yes}}
n "No" +write %{{no}}
'

response="$(echo "$wks_source" | wk --script)"

case "$response" in
    yes) mpv "https://www.youtube.com/watch?v=dQw4w9WgXcQ" ;;
    *) ;;
esac
```

```{note}
You'll need [mpv](https://mpv.io/) and [yt-dlp](https://github.com/yt-dlp/yt-dlp) installed for the script to work
```

Instead of wk executing the **command** in a chord, it
prints the command text to stdout. This can be used in a
shell script to prompt a user to select from some list of
options. Where a script might use `dmenu` or `rofi` to
prompt for input, wk with `+write` can serve the same role.

You can see more examples of scripting with wk
[here](../examples/scripting).

## Adding some sugar to your wks files

wks offers some syntactic sugar to help you write less and
do more. You may have noticed that the `+keep` example was a
bit repetitive. Using **chord arrays** and
**interpolations**, we can succinctly capture the same
behavior:

````{tab} Sweetened

```wks
m "Move window" +keep
{
    [a s d] "Workspace %(index+1)" %{{send-to-workspace %(index)}}
}
```

````

````{tab} Plain

```wks
m "Move window" +keep
{
    a "Workspace 1" %{{send-to-workspace 0}}
    s "Workspace 2" %{{send-to-workspace 1}}
    d "Workspace 3" %{{send-to-workspace 2}}
}
```

````

There are two pieces of new syntax: `[a s d]` and
`%(keyword)`.

**Chord arrays** (`[a s d]`)
: A chord array is used to generate one chord per trigger
  key. Each trigger key shares the description and command
  template given to the chord array.

**Interpolations** (`%(keyword)`)
: An interpolation allows you to access some metadata about
  a chord within a description or command.

Because a chord array generates chords from a shared
template, you will almost always use chord arrays and
interpolations together. In this example, `%(index)` gives
the zero-based position of each chord in the array.
`%(index+1)` offsets it by one so the descriptions start
at 1.

```{seealso}
Read more about [chord arrays](../reference/wks.md#chord-arrays) and [interpolations](../reference/wks.md#interpolations) in the documentation.
```

## Further reading

We have only scratched the surface of wks features. If you
prefer to learn by example continue reading. If you want to
learn about all the tools at your disposal check out the
complete [cli](../reference/cli) and [wks](../reference/wks)
documentation.

```{seealso}
If you built wk from source, you can build your wks configuration into the binary by following [these steps](installation.md#building-with-a-wks-config).
```
