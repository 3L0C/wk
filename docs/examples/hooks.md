# Adding Hooks

Hooks help enforce some behavior while minimizing repetition.
Here is an example:

````{tab} Hooked

```{code-block} wks
:emphasize-lines: 1,5

e "Emacs" ^before %{{switch-to-workspace 1}}
{
    o "Open" %{{emacs}}
    r "Roam" %{{emacs ~/Documents/roam/20240101080032-startpage.org}}
    s "Scratch" +unhook %{{emacs --eval "(scratch-buffer)"}}
}
```

````

````{tab} Unhooked

```wks
e "Emacs"
{
    o "Open" %{{switch-to-workspace 1; emacs}}
    r "Roam" %{{switch-to-workspace 1; emacs ~/Documents/roam/20240101080032-startpage.org}}
    s "Scratch" %{{emacs --eval "(scratch-buffer)"}}
}
```

````

In this example, we use hooks to ensure the "Emacs" key
chords take place on a specific workspace. Some key chords
are workspace agnostic. For these you can use the `+unhook`
flag, and the key chord will work as normal.

Notice that this policy is enforced by wk and not by a rule
configured in your desktop environment or window manager. No
messing with window titles, class names, or other
non-obvious attributes. Let wk switch to the appropriate
workspace when it should.

```{seealso}
Read the complete documentation on hooks [here](../reference/wks.md#hooks).
```
