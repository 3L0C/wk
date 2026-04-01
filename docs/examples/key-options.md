# Key Options

Key options help when constructing a [modular
config](modular-configs):

````{tab} Guarded

```{code-block} wks
:caption: `~/chat.wks`
:emphasize-lines: 1

<s i g n a l> "Signal"
{
    # some bindings
}
```

```{code-block} wks
:caption: `~/music.wks`
:emphasize-lines: 1

<s p o t i f y> "Spotify"
{
    # some bindings
}
```

```{code-block} wks
:caption: `~/main.wks`

a "Apps"
{
    :include "chat.wks"
    :include "music.wks"
}
```

````

````{tab} Naive

```{code-block} wks
:caption: `~/chat.wk`

s "Signal"
{
    # some bindings
}
```

```{code-block} wks
:caption: `~/music.wks`

s "Spotify"
{
    # some bindings
}
```

```{code-block} wks
:caption: `~/main.wks`

a "Apps"
{
    :include "chat.wks"
    :include "music.wks"
}
```

````

In the guarded example, "Signal" is bound to `a s` while
"Spotify" is bound to `a p`. In the naive example, "Spotify"
overrides "Signal" because they both try to bind to the same
key. Last wins during such collisions, and "Spotify" is
bound while "Signal" is silently dropped.

Key options avoid this. Instead of binding to a single,
constant trigger key, a set of options is given between the
brackets (`<a b c>`). In the above example, "Signal" comes
first, and the first key option, `s`, is chosen because it
is not taken. When "Spotify" is parsed, we see that `s` is
already taken, so the next option `p` is chosen from the
list. If all options are already defined, then wk will print
an error stating that all key options are already bound
instead of silently droping the key chord.

```{seealso}
Read the complete documentation on key options [here](../reference/wks.md#key-options).
```
