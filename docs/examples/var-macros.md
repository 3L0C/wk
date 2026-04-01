# Using Variables

Do you want _one config to rule them all_? Then consider
using variables:

````{tab} Varied

```{code-block} wks
:caption: `~/x11.wks`
:emphasize-lines: 1

:var "PASTE_COMMAND" "xclip -o -selection clipboard"
```

```{code-block} wks
:caption: `~/wayland.wks`
:emphasize-lines: 1

:var "PASTE_COMMAND" "wl-paste"
```

```{code-block} wks
:caption: `~/main.wks`

:include "wayland.wks"
u "Url Handler"
{
    f "Firefox" %{{firefox "$(%(PASTE_COMMAND))"}}
    m "mpv"     %{{mpv     "$(%(PASTE_COMMAND))"}}
}
```

````

````{tab} Invariant

```{code-block} wks
:caption: `~/main.wks`

u "Url Handler"
{
    f "Firefox" %{{firefox "$(wl-paste)"}}
    m "mpv"     %{{mpv     "$(wl-paste)"}}
}
```

````

In this example, we use a variable to represent an abstract
`PASTE_COMMAND`. We define this variable in an
environment-specific wks file. Now, it's easy to move
between environments by simply changing the included wks
file.

```{seealso}
Read the complete documentation on the `:var` macro [here](../reference/wks.md#var-macro).
```
