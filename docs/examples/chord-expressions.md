# Chord Expressions

**Chord expressions** are a twist on **chord arrays**. Here
is an example:

````{tab} Expressive
```wks
[
    (y "YouTube")
    (g "GitHub")
    (t "Twitch" %{{twitch.tv}})
] "Default" %{{firefox %(desc,,).com}}
```
````

````{tab} Expressionless
```wks
y "YouTube" %{{firefox youtube.com}}
g "GitHub"  %{{firefox github.com}}
t "Twitch"  %{{firefox twitch.tv}}
```
````

```{note}
Check out the supported [interpolations](../reference/wks.md#interpolations) if you are unfamiliar with the meaning of `%(desc,,)`.
```

Chord arrays are great for defining key chords that differ
ever so slightly. For those which differ a bit more, you can
use chord expressions. It may seem simple enough to use the
expressionless form here, but consider the work you would
need to do to add 10 more websites to this list. Now
consider switching from firefox to brave. Chord expressions
become more valuable as your key chords grow in size, and
you find yourself repeating common patterns.

```{seealso}
Read the complete documentation on chord expressions [here](../reference/wks.md#chord-expressions).
```
