# Wrapping Commands

Wrapped commands can be useful in certain environments:

````{tab} Wrapped

```{code-block} wks
:emphasize-lines: 1,7

:wrap-cmd "uwsm-app --"
a "Apps"
{
    b "Brave" %{{brave}}
    k "Kitty" %{{kitty}}
}
m "Music" +wrap "%(wrap_cmd) kitty -e"
{
    n "ncmpcpp" %{{ncmpcpp}}
}
```

````

````{tab} Unwrapped

```wks
a "Apps"
{
    b "Brave" %{{uwsm-app -- brave}}
    k "Kitty" %{{uwsm-app -- kitty}}
}
m "Music"
{
    n "ncmpcpp" %{{uwsm-app -- kitty -e ncmpcpp}}
}
```

````

Some Wayland window managers, like
[Hyprland](https://wiki.hypr.land/Useful-Utilities/Systemd-start/),
benefit from special tools like
[uwsm](https://wiki.archlinux.org/title/Universal_Wayland_Session_Manager).
In other cases, it may be useful to apply a common prefix to
some key chords via wrapping. This is something that
[hooks](../reference/wks.md#hooks) cannot do as they run
standalone commands.

You can wrap commands globally by setting `:wrap-cmd
"global-wrap"`, or locally with `+wrap "local-wrapper"`. The
global command wrapper can be accessed via
[interpolation](../reference/wks.md#interpolations) with
`%(wrap_cmd)`.

```{seealso}
Read the complete documentation on wrapping commands [here](../reference/wks.md#command-wrapping).
```
