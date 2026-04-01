# Modular Configs

If your config starts to grow too big for a single file, you
can break it up into smaller modules:

````{tab} Modular

```{code-block} wks
:caption: `~/apps.wks`
:emphasize-lines: 2

b "Brave" %{{brave}}
e "Emacs" { :include "emacs.wks" }
s "Spotify" %{{spotify}}
```

```{code-block} wks
:caption: `~/emacs.wks`

o "Open" %{{emacs}}
r "Roam" %{{emacs ~/Documents/roam}}
```

```{code-block} wks
:caption: `~/settings.wks`

:font "monospace, 12"
:delay 0
:top
```

```{code-block} wks
:caption: `~/main.wks`
:emphasize-lines: 1-3

:include "settings.wks"
a "Apps"  { :include "apps.wks" }
e "Emacs" { :include "emacs.wks" }
```

````

````{tab} Monolith

```{code-block} wks
:caption: `~/main.wks`

:font "monospace, 12"
:delay 0
:top
a "Apps"
{
    b "Brave" %{{brave}}
    e "Emacs"
    {
        o "Open" %{{emacs}}
        r "Roam" %{{emacs ~/Documents/roam}}
    }
    s "Spotify" %{{spotify}}
}
e "Emacs"
{
    o "Open" %{{emacs}}
    r "Roam" %{{emacs ~/Documents/roam}}
}
```

````

In the modular example, you only need to run `wk
--key-chords ~/main.wks` and all your included files will
get pulled in.

```{seealso}
Read the complete documentation on the `:include` macro [here](../reference/wks.md#include-macro).
```
