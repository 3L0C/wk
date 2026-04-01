# Implicit Array Keys

You may find it useful to predefine some trigger keys to
avoid repeating yourself. Here is an example:

````{tab} Implicit
```wks
:implicit-array-keys "asdfjk;"
... "Workspace %(index+1)" %{{switch-to-workspace %(index)}}
```
````

````{tab} Explicit
```wks
[asdfjk;] "Workspace %(index+1)" %{{switch-to-workspace %(index)}}
```
````

````{tab} Verbose
```wks
a "Workspace 1" %{{switch-to-workspace 0}}
s "Workspace 2" %{{switch-to-workspace 1}}
d "Workspace 3" %{{switch-to-workspace 2}}
f "Workspace 4" %{{switch-to-workspace 3}}
j "Workspace 5" %{{switch-to-workspace 4}}
k "Workspace 6" %{{switch-to-workspace 5}}
; "Workspace 7" %{{switch-to-workspace 6}}
```
````

The implicit array keys really shine if you often use your
home row, numpad, etc. to select from a list of options. It
also makes it easy to change your preferred keys as you only
need to modify one line instead of many.

```{seealso}
Read the complete documentation on implicit array keys [here](../reference/wks.md#implicit-arrays).
```
