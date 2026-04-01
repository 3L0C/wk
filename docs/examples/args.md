# Dynamic Arguments

The `+args` flag enables parameterized chord definitions.
Here is an example:

````{tab} Dynamic
```wks
[
    (a "Alternative" +args  "~/.config/music-player/playlist/alternative.m3u")
    (e "Electronic"  +args  "~/.config/music-player/playlist/electronic.m3u")
    (p "Pop Punk"    +args  "--shuffle ~/.config/music-player/playlist/pop-punk.m3u")
    m
] "Music Player" %{{music-player %($0)}}
```
````

````{tab} Static
```wks
a "Alternative"  %{{music-player ~/.config/music-player/playlist/alternative.m3u}}
e "Electronic"   %{{music-player ~/.config/music-player/playlist/electronic.m3u}}
p "Pop Punk"     %{{music-player --shuffle ~/.config/music-player/playlist/pop-punk.m3u}}
m "Music Player" %{{music-player}}
```
````

Positional interpolations - `%($0)`, `%($1)`, etc. - are
defined with the `+args` flag. In the above example `%($0)`
is not defined for the "Music Player" chord. Any undefined
argument is equivalent to the empty string, making it
generally harmless to use any undefined arguments.

```{attention}
The `+args` flag has a lot of nuance - please consult the [documentation](../reference/wks.md#argument-positions) if you plan on getting creative.
```
