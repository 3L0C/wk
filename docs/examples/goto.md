# The Goto Meta Command

Meta commands can be fun to play around with. Here is a
visual example:

<video autoplay loop muted playsinline controls width="100%">
    <source src="../_static/demo-goto.webm" type="video/webm">
</video>

```{code-block} wks
:emphasize-lines: 5

a "Apps" +keep +title
{
    b "Brave" %{{brave}}
    s "Spotify" %{{spotify}}
    e "Emacs" @goto "e"
}
e "Emacs" +title
{
    o "Open" %{{emacs}}
    r "Roam" %{{emacs ~/Documents/roam}}
}
```

In this example, I press `SUPER+SPACE` to open wk, then I
press `a b e o`. With this one wk session, I open Brave and
Emacs even though they are not part of the same key
sequence. Without `@goto`, I would have triggered wk,
pressed `a b` to open Brave, then `ESCAPE` to close wk,
trigger wk again, and finally pressed `e o` to open Emacs.

The meta command here is `@goto`. It's a meta command
because it controls the menu itself, and it takes the place
of a command. `@goto` takes a single quoted argument: a list
of trigger keys to "press".

```{note}
The use of `+keep` is essential in the above example. Without that the menu would have closed after I triggered Brave.
```

```{seealso}
Read the complete documentation on `@goto` [here](../reference/wks.md#goto).
```
