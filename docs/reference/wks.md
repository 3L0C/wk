# Which Key Source Files

wks is the scripting language used by **wk**(1) to define key chords.
This page is the complete reference. If you're new to wks, start with
the [getting started](../quick-start/getting-started) guide and the
[examples](../examples/index).

## Grammar

```text
key_chord          -> ( chord | prefix | chord_array | meta_command ) ;

chord              -> trigger_key description keyword* ( command | meta_command ) ;

prefix             -> trigger_key description keyword* '{' ( key_chord )+ '}' ;

chord_array        -> ( implicit_array | explicit_array ) ;

implicit_array     -> modifier* '...' description keyword* ( command | meta_command ) ;

explicit_array     -> '[' ( trigger_key | chord_expression )+ ']' description keyword* ( command | meta_command ) ;

chord_expression   -> '(' trigger_key description keyword* ( command | meta_command )? ')' ;

trigger_key        -> modifier* ( normal_key | special_key | key_options ) ;

normal_key         -> ( '\\' [\\\[\]{}#":^+()] | [^\s\[\]{}#":^+()] ) ;

special_key        -> ( 'Left'    | 'Right'   | 'Up'     | 'Down' | 'BS'
                      | 'TAB'     | 'SPC'     | 'RET'    | 'DEL'  | 'ESC'
                      | 'Home'    | 'PgUp'    | 'PgDown' | 'End'  | 'Begin'
                      | 'VolDown' | 'VolMute' | 'VolUp'  | 'Play' | 'Stop'
                      | 'Prev'    | 'Next'    | 'F'[1-35] ) \s+ ;

modifier           -> ( 'C' | 'H' | 'M' | 'S' ) '-' ;

key_options        -> '<' ( modifier* ( normal_key | special_key | '...' ) )+ '>' ;

description        -> '"' ( '\\"' | [^"] | interpolation )* '"' ;

command            -> '%' delimiter ( . | interpolation )* delimiter ;

delimiter          -> ( open_delim | close_delim | ([^[{(])\1 ) ;

open_delim         -> ( '{{' | '((' | '[[' ) ;

close_delim        -> ( '}}' | '))' | ']]' ) ;

interpolation      -> '%(' ( chord_metadata | arg_position | user_variable ) ')' ;

chord_metadata     -> ( 'key'
                      | 'index'
                      | 'index+1'
                      | 'desc'
                      | 'desc^'
                      | 'desc^^'
                      | 'desc,'
                      | 'desc,,'
                      | 'wrap_cmd' );

arg_position       -> '$' [0-9]+ ;

user_variable      -> [^)]+ ;

keyword            -> ( hook | flag ) ;

meta_command       -> '@' ( 'goto' description
                          | 'group' description '{' ( key_chord )+ '}' ) ;

hook               -> '^' ( 'before'
                          | 'after'
                          | 'sync-before'
                          | 'sync-after' ) command ;

flag               -> '+' ( 'keep'
                          | 'close'
                          | 'inherit'
                          | 'ignore'
                          | 'unhook'
                          | 'deflag'
                          | 'no-before'
                          | 'no-after'
                          | 'write'
                          | 'execute'
                          | 'sync-command'
                          | 'unwrap'
                          | 'release'
                          | 'hold'
                          | 'wrap' '"' ( '\\"' | [^"] | interpolation )* '"'
                          | 'title' ( '"' ( '\\"' | [^"] | interpolation )* '"' )?
                          | 'args' ( '"' ( '\\"' | [^"] | interpolation )* '"' )+ ) ;

preprocessor_macro -> ':' ( string_macro
                          | switch_macro
                          | integer_macro
                          | unsigned_macro
                          | number_macro ) ;

string_macro       -> ( 'include'
                      | 'fg'
                      | 'fg-key'
                      | 'fg-delimiter'
                      | 'fg-prefix'
                      | 'fg-chord'
                      | 'fg-title'
                      | 'fg-goto'
                      | 'fg-header'
                      | 'bg'
                      | 'bd'
                      | 'shell'
                      | 'font'
                      | 'title'
                      | 'title-font'
                      | 'delimiter'
                      | 'wrap-cmd'
                      | 'header-align'
                      | 'header-font'
                      | 'var' '"' ( '\\"' | [^"] | user_variable )* '"' ) '"' ( '\\"' | [^"] | user_variable )* '"' ;

switch_macro       -> ( 'debug'
                      | 'unsorted'
                      | 'top'
                      | 'bottom'
                      | 'release' );

integer_macro      -> ( 'menu-width'
                      | 'menu-gap'
                      | 'table-padding' ) '-'? [0-9]+ ;

unsigned_macro     -> ( 'max-columns'
                      | 'border-width'
                      | 'width-padding'
                      | 'height-padding'
                      | 'delay'
                      | 'keep-delay' ) [0-9]+ ;

number_macro       -> ( 'border-radius' ) '-'? [0-9]+ ( '.' [0-9]* )? ;
```

Comments begin with `#` and extend to the end of the line. The `#`
character is literal inside descriptions and commands; it
does not need to be escaped.

## Trigger Keys

A trigger key is the keypress that activates a chord.

```text
trigger_key -> modifier* ( normal_key | special_key | key_options ) ;
```

### Normal Keys

Any printable, non-whitespace, UTF-8 character.

```text
normal_key -> ( '\\' [\\\[\]{}#":^+()] | [^\s\[\]{}#":^+()] ) ;
```

The following characters have special meaning in wks and must be
escaped with a backslash (`\`) to use as trigger keys:

| Character | Meaning              |
|-----------|----------------------|
| `[` `]`   | Chord array          |
| `{` `}`   | Prefix block         |
| `(` `)`   | Chord expression     |
| `#`       | Comment              |
| `"`       | Description          |
| `:`       | Preprocessor macro   |
| `^`       | Hook                 |
| `+`       | Flag                 |

### Special Keys

Non-printable or whitespace keys use named forms. Whitespace is
required after a special key.

| Key            | wks form   |
|----------------|------------|
| Left arrow     | `Left`     |
| Right arrow    | `Right`    |
| Up arrow       | `Up`       |
| Down arrow     | `Down`     |
| Backspace      | `BS`       |
| Tab            | `TAB`      |
| Space          | `SPC`      |
| Enter/Return   | `RET`      |
| Delete         | `DEL`      |
| Esc            | `ESC`      |
| Home           | `Home`     |
| Page up        | `PgUp`     |
| Page down      | `PgDown`   |
| End            | `End`      |
| Begin          | `Begin`    |
| Function keys  | `F1`-`F35` |
| Volume Down    | `VolDown`  |
| Mute           | `VolMute`  |
| Volume Up      | `VolUp`    |
| Play           | `Play`     |
| Stop           | `Stop`     |
| Previous Track | `Prev`     |
| Next Track     | `Next`     |

```wks
TAB "Cycle" %{{cycle-windows}}
C-RET "Confirm" %{{confirm}}
```

### Modifiers

Zero or more modifiers can precede a key.

| Modifier    | wks form |
|-------------|----------|
| Control     | `C-`     |
| Meta/Alt    | `M-`     |
| Hyper/Super | `H-`     |
| Shift       | `S-`     |

To match `Control+c`, write `C-c`. Modifiers chain: `C-M-x` matches
`Control+Alt+x`.

```{note}
The Shift modifier (`S-`) has a subtlety with normal keys. If pressing
a key with Shift produces a different character (e.g., `a` → `A`), use
the shifted character directly - `A`, not `S-a`. The `S-` modifier is
only needed for keys where Shift does not change the output (e.g.,
special keys like `S-TAB`).
```

### Key Options

Key options let a chord bind to the **first available** key from a
list.

```text
key_options -> '<' ( modifier* ( normal_key | special_key | '...' ) )+ '>' ;
```

```wks
<s i g> "Signal" %{{signal}}
```

The parser tries `s` first; if already bound, it tries `i`, then `g`.
If all keys are taken, an error is raised. The ellipsis (`...`)
expands to the implicit array keys as a fallback:

```wks
<s i g ...> "Signal" %{{signal}}
```

Modifiers before `<` apply to all options. Modifiers inside apply to
individual keys:

```wks
M-<a b>      # Both options get M-
<a M-b>      # Only 'b' gets M-
```

Key options are most useful with [`:include`](#include-macro) to avoid
trigger key collisions across files. See the [key options
example](../examples/key-options).

## Descriptions

A description is a quoted string shown as the hint text in the menu.

```text
description -> '"' ( '\\"' | [^"] | interpolation )* '"' ;
```

Descriptions can contain [interpolations](#interpolations). Use `\"`
for a literal double quote. An empty description (`""`) is valid.

## Commands

A command is the action executed when a chord completes.

```text
command -> '%' delimiter ( . | interpolation )* delimiter ;
```

A command starts with `%` followed by a delimiter pair. Anything
between the delimiters is passed to the shell. If it runs in your
terminal, it works in a wks command.

### Delimiters

```text
delimiter   -> ( open_delim | close_delim | ([^[{(])\1 ) ;
open_delim  -> ( '{{' | '((' | '[[' ) ;
close_delim -> ( '}}' | '))' | ']]' ) ;
```

The three bracket-style delimiters have matching open/close pairs.
Any other ASCII character repeated twice also works as a delimiter:

```wks
%{{echo "hello"}}           # Standard
%((echo "hello"))           # Parentheses
%[[echo "hello"]]           # Brackets
%||echo "hello"||           # Arbitrary
```

This flexibility, inspired by **sed**(1), means your command content
never conflicts with the delimiter - just pick one that doesn't appear
in the command. Different chords can use different delimiters.

## Chords

A chord is a complete key binding - pressing its trigger key performs
an action.

```text
chord -> trigger_key description keyword* ( command | meta_command ) ;
```

```wks
b "Brave" %{{brave}}
C-s "Save" %{{save-file}}
```

## Prefixes

A prefix groups related chords under a common trigger key. Pressing
the prefix key opens a submenu rather than executing a command.

```text
prefix -> trigger_key description keyword* '{' ( key_chord )+ '}' ;
```

Prefixes can be nested arbitrarily deep:

```wks
e "Emacs"
{
    o "Open" %{{emacs}}
    r "Roam"
    {
        h "Home" %{{emacs ~/Documents/roam/home.org}}
        j "Journal" %{{emacs ~/Documents/roam/daily}}
    }
}
```

## Chord Arrays

Chord arrays generate multiple chords from a shared template, reducing
repetition.

```text
chord_array -> ( implicit_array | explicit_array ) ;
```

### Implicit Arrays

An implicit array uses the predefined implicit array keys (set via
`--implicit-keys` or `:implicit-array-keys`) as trigger keys.

```text
implicit_array -> modifier* '...' description keyword* command ;
```

````{tab} Implicit
```wks
... "Workspace %(index+1)" %{{switch-to-workspace %(index)}}
```
````

````{tab} Expanded
```wks
# Assuming implicit keys are "asdfghjkl;"
a "Workspace 1" %{{switch-to-workspace 0}}
s "Workspace 2" %{{switch-to-workspace 1}}
d "Workspace 3" %{{switch-to-workspace 2}}
# ... and so on for each implicit key
```
````

### Explicit Arrays

An explicit array lists its trigger keys between brackets.

```text
explicit_array -> '[' ( trigger_key | chord_expression )+ ']' description keyword* command ;
```

````{tab} Array
```wks
[asdfghjkl] "Workspace %(index+1)" %{{switch-to-workspace %(index)}}
```
````

````{tab} Expanded
```wks
a "Workspace 1" %{{switch-to-workspace 0}}
s "Workspace 2" %{{switch-to-workspace 1}}
d "Workspace 3" %{{switch-to-workspace 2}}
f "Workspace 4" %{{switch-to-workspace 3}}
g "Workspace 5" %{{switch-to-workspace 4}}
h "Workspace 6" %{{switch-to-workspace 5}}
j "Workspace 7" %{{switch-to-workspace 6}}
k "Workspace 8" %{{switch-to-workspace 7}}
l "Workspace 9" %{{switch-to-workspace 8}}
```
````

### Chord Expressions

A chord expression customizes an individual entry within a chord
array. Wrap it in parentheses inside the brackets. Only a trigger key
and description are required - any missing command or keywords are
filled in by the surrounding array.

```text
chord_expression -> '(' trigger_key description keyword* command? ')' ;
```

````{tab} Expressive
```wks
[
    (b "Brave")
    (c "Mullvad Chrome" %{{mullvad-exclude chrome ~/startpage.html}})
    x
] "XDG-OPEN" %{{%(desc,,) ~/startpage.html}}
```
````

````{tab} Expanded
```wks
b "Brave" %{{brave ~/startpage.html}}
c "Mullvad Chrome" %{{mullvad-exclude chrome ~/startpage.html}}
x "XDG-OPEN" %{{xdg-open ~/startpage.html}}
```
````

Here, `b` and `x` inherit the array's command template, while `c`
provides its own. The `x` key has no expression, so it also inherits
the array's description.

## Interpolations

An interpolation inserts dynamic values into descriptions and
commands.

```text
interpolation -> '%(' ( chord_metadata | arg_position | user_variable ) ')' ;
```

There are three kinds:

### Chord Metadata

Built-in identifiers that reference properties of the current chord.
Valid in descriptions and commands.

| Identifier   | Value                                                     |
|--------------|-----------------------------------------------------------|
| `%(key)`     | The trigger key of the current chord                      |
| `%(index)`   | 0-based position in parse order within the current scope  |
| `%(index+1)` | 1-based position (same as above, offset by one)           |
| `%(desc)`    | The description (not valid inside a description)          |
| `%(desc^)`   | Description with first character capitalized              |
| `%(desc^^)`  | Description fully uppercased                              |
| `%(desc,)`   | Description with first character lowercased               |
| `%(desc,,)`  | Description fully lowercased                              |
| `%(wrap_cmd)`| The global wrap command value                             |

```{note}
Index values are resolved **before** sorting. `%(index)` reflects
parse order, not the final display order. Prefixes start a new scope
for their children.
```

### User Variables

Variables defined with the [`:var` macro](#var-macro). Valid in
descriptions, commands, and preprocessor macro arguments.

```text
user_variable -> [^)]+ ;
```

```wks
:var "CMD" "hyprctl dispatch workspace"
[asdfghjkl] "Workspace %(index+1)" %{{%(CMD) %(index)}}
```

### Argument Positions

Positional references to values defined with `+args`. Valid in
descriptions and commands where `+args` is defined.

```text
arg_position -> '$' [0-9]+ ;
```

`%($0)` is the first argument, `%($1)` the second, and so on.
Undefined positions resolve to an empty string.

Arguments follow lexical scoping - descendants within a prefix can
access that prefix's arguments. When a descendant defines its own
`+args`, its values shadow ancestor arguments at the same index, but
other indices remain accessible:

```wks
p "+Prefix" +args "outer0" "outer1"
{
    a "Use both" +write %{{%($0) %($1)}}
    n "+Nested" +args "inner0"
    {
        # %($0) = "inner0" (shadowed), %($1) = "outer1" (inherited)
        b "Mixed" +write %{{%($0) %($1)}}
    }
}
```

## Hooks

Hooks attach additional commands to a chord or prefix.

```text
hook -> '^' ( 'before' | 'after' | 'sync-before' | 'sync-after' ) command ;
```

**^before** / **^after**
: Run a command before or after the chord's own command, asynchronously.

**^sync-before** / **^sync-after**
: Same, but block wk until the hook completes.

```{warning}
A blocking `sync-*` hook that never returns will freeze wk. You may
need to restart your system to regain keyboard control.
```

Hooks on a prefix apply to all direct child chords but **not** to
nested prefixes (see [Inheritance](#inheritance)). Use `+unhook`,
`+no-before`, or `+no-after` on individual chords to opt out:

````{tab} Hooked
```{code-block} wks
:emphasize-lines: 1

e "Emacs" ^before %{{switch-to-workspace 1}}
{
    o "Open" %{{emacs}}
    r "Roam" %{{emacs ~/Documents/roam}}
    s "Scratch" +unhook %{{emacs --eval "(scratch-buffer)"}}
}
```
````

````{tab} Equivalent
```wks
e "Emacs"
{
    o "Open" %{{switch-to-workspace 1; emacs}}
    r "Roam" %{{switch-to-workspace 1; emacs ~/Documents/roam}}
    s "Scratch" %{{emacs --eval "(scratch-buffer)"}}
}
```
````

## Flags

Flags modify the behavior of a chord or prefix.

```text
flag -> '+' ( 'keep' | 'close' | 'inherit' | 'ignore' | 'unhook'
            | 'deflag' | 'no-before' | 'no-after' | 'write'
            | 'execute' | 'sync-command' | 'unwrap'
            | 'release' | 'hold'
            | 'wrap' description | 'title' description?
            | 'args' description+ ) ;
```

### Menu Control

**+keep**
: Keep the menu open after executing the chord. Turns wk into a
  "hydra" for repeated actions.

**+close**
: Force-close the menu. Useful to override `+keep` on the enclosing
  prefix for specific chords.

```wks
m "Music" +keep
{
    n "Next" %{{mpc next}}
    p "Prev" %{{mpc prev}}
    o "Open player" +close %{{kitty -e ncmpcpp}}
}
```

### Keyboard Control

By default wk holds the keyboard grab while a chord runs.
A `+release` chord drops the grab before it runs its
command.

**+release**
: Release the keyboard grab before the chord runs its
  command. Use it for a command requires wk to not be
  focused (e.g., xdotool/ydotool). On a prefix, the flag
  cascades to the chords in the subtree.

**+hold**
: Hold the keyboard grab while the chord runs. This is the
  default. Use it to override a `:release` default or an
  inherited `+release`. A chord with both `+release` and
  `+hold` is an error.

```wks
:release
w "Window" +keep
{
    f "Type something" %{{xdotool type 'foobar'}}
    s "Screenshot" +hold %{{grim}}
}
```

### Output Control

**+write**
: Print the command text to stdout instead of executing it.
  Turns wk into a selection prompt, like dmenu.

**+execute**
: Execute the command normally. Overrides `+write` inherited
  from a parent prefix.

**+sync-command**
: Execute the command synchronously (blocking). See the
  warning in [Hooks](#hooks) about blocking commands.

### Inheritance Control

**+inherit**
: Force a nested prefix to inherit hooks and flags from its
  parent.  Has no effect on chords.

**+ignore**
: Ignore all inherited hooks and flags. Has no effect on
  prefixes.

**+unhook**
: Ignore all inherited hooks only.

**+deflag**
: Ignore all inherited flags only.

**+no-before**
: Ignore inherited `^before` and `^sync-before` hooks.

**+no-after**
: Ignore inherited `^after` and `^sync-after` hooks.

### Command Wrapping

**+wrap** `"wrapper"`
: Wrap this chord's command (or all commands in a prefix) with the
  given string. The command is executed as `/bin/sh -c "WRAPPER CMD"`.
  Supports interpolation.

**+unwrap**
: Prevent wrapping, even if a global wrap or inherited `+wrap` is set.

Wrap precedence, most specific to least:

1. `+unwrap` on chord (no wrap)
2. `+wrap "custom"` on chord
3. `+wrap` inherited from parent prefix
4. Global `:wrap-cmd` / `--wrap-cmd`
5. No wrapper

````{tab} Wrapped
```{code-block} wks
:emphasize-lines: 1,6

:wrap-cmd "uwsm-app --"
a "Apps"
{
    b "Brave" %{{brave}}
}
f "Foot" +wrap "foot -e"
{
    n "ncmpcpp" %{{ncmpcpp}}
}
```
````

````{tab} Equivalent
```wks
a "Apps"
{
    b "Brave" %{{uwsm-app -- brave}}
}
f "Foot"
{
    n "ncmpcpp" %{{foot -e ncmpcpp}}
}
```
````

### Title

**+title** `"text"`
: Set a title displayed above the menu for this chord or prefix.
  Supports interpolation. Overrides the global `--title` setting.
  An empty string (`""`) clears any inherited or global title.
  Omitting the argument sets the title to the chord's description.

### Arguments

**+args** `"arg0"` `"arg1"` ...
: Define positional arguments accessible via `%($0)`, `%($1)`, etc.
  See [Argument Positions](#argument-positions) for scoping rules.

````{tab} Dynamic
```wks
[
    (a "Alice" +args "alice@example.com")
    (b "Bob" +args "bob@example.com")
    c
] "Email %($0)" +args "(default)" %{{mailto:%($0)}}
# a -> mailto:alice@example.com
# b -> mailto:bob@example.com
# c -> mailto:(default)
```
````

````{tab} Static
```wks
a "Alice" %{{mailto:alice@example.com}}
b "Bob" %{{mailto:bob@example.com}}
c "Email (default)" %{{mailto:(default)}}
```
````

## Meta Commands

Meta commands control wk's menu rather than running a shell command.
Every keyword introduced by `@` is a meta command, just as `^`
introduces hooks, `+` introduces flags, and `:` introduces
preprocessor macros. wk provides two meta commands: `@goto`, which
appears in a chord's command position, and `@group`, which appears
wherever a key chord may begin and wraps a block of chords, assigning
them all to the same labeled menu column.

```text
meta_command -> '@' ( 'goto' description
                    | 'group' description '{' ( key_chord )+ '}' ) ;
```

### @goto

Navigates to a different location in the key chord hierarchy without
closing and restarting wk. The argument uses the same syntax as
`--press`:

- `@goto ""` - navigate to the root menu
- `@goto "w"` - navigate to the "w" prefix
- `@goto "w m"` - navigate through "w" then "m"

If the path leads to a chord (not a prefix), that chord's command is
executed.

```wks
w "Window" +keep
{
    m "Move" +keep
    {
        ... "Move to %(index+1)" %{{move-window %(index)}}
        BS "Go back" @goto "w"
        S-BS "Go home" @goto ""
    }
    r "Resize" +keep +inherit
    {
        ... "Resize %(index+1)" %{{resize-window %(index)}}
    }
}
```

```{note}
`@goto` cannot be combined with hooks or commands. Circular goto
chains (e.g., `a @goto "b"` and `b @goto "a"`) are detected at
runtime and produce an error.
```

### @group

Groups a block of chords into a single labeled menu column:

```text
@group "Name" { <chords> }
```

The block is **not** a submenu - the chords inside it belong to the
enclosing scope (root menu or prefix body) exactly as if `@group` were
not there. `@group` only stamps each contained chord with a column
name.

Header text may be aligned. See `:header-align` /
`--header-align` below.

```wks
@group "Chats" {
    a "Chat A" %{{chat-a}}
    b "Chat B" %{{chat-b}}
}

@group "Mail" {
    c "Mail C" %{{mail-c}}
}

@group "Tools" {
    t "Tools" {
        x "Tool X" %{{tool-x}}
    }
}
```

A prefix declared inside a `@group` block joins the column like any
other chord, but its own submenu is an independent scope - it is not
part of the group and may declare its own `@group` blocks (or none).
In the example above, `t` joins the "Tools" column, but the submenu
reached via `t` is ungrouped.

Array expansions ([Chord Arrays](#chord-arrays)) inside a `@group`
block put every expanded chord in the group.

A submenu (root or prefix body) must be entirely grouped or entirely
ungrouped; mixing grouped and ungrouped chords in the same scope is a
compile error.

```{note}
The following are compile errors:

- `Cannot mix grouped and ungrouped chords in the same block.` - a
  submenu has both grouped and ungrouped chords.
- `Empty @group block.` - a `@group` block adds no chords.
- `Cannot nest @group blocks.` - a `@group` appears inside another
  `@group`.
- `Unterminated @group block.` - end of file reached before the
  block's closing `}`.
- `Expected group name after @group.` - no description string follows
  `@group`.
- `Expected '{' to begin @group block.` - the name is not followed
  by `{`.
- `Cannot use chord interpolations in @group name.` - the group name
  uses `%(key)`, `%(index)`, `%(desc)`, or similar per-chord
  interpolations. [User variable](#user-variables) interpolation
  (`%(user_var)`) is allowed, since the name is resolved once for the
  whole block.
```

By default, columns are sorted alphabetically by group name, with
keys sorted within each column; chords sharing a group name coalesce
into one column regardless of where they appear in the file. With
`:unsorted` / `--unsorted`, columns and chords keep declaration
order; **adjacent** `@group` blocks with the same name still coalesce
into one column, but the same name used again later, separated by a
different group, produces a second column with that header. See
[Sorting](#sorting).

Grouped layout divides the menu width evenly across columns
(`menu width / number of groups`); `:max-columns` / `--max-columns`
has no effect on a grouped scope. Header colors are set with
`:fg-header` / `--fg-header` (default `#7FB4CA`). The umbrella `--fg`
flag also sets the header foreground. Header text alignment is set
menu-wide with `:header-align` / `--header-align` (default `left`;
also accepts `center` or `right`). The header row uses its own font,
set with `:header-font` / `--header-font`; when unset it falls back
to the menu font (`:font` / `--font`).

## Preprocessor Macros

Preprocessor macros configure the menu appearance and behavior, or
include other wks files. They start with `:` and override the
equivalent **wk**(1) command-line flags.

```text
preprocessor_macro -> ':' ( string_macro | switch_macro
                          | integer_macro | unsigned_macro
                          | number_macro ) ;
```

### String Macros

String macros take a quoted string argument. All string macro
arguments support [user variable](#user-variables) interpolation.

| Macro            | Equivalent CLI flag  | Description                 |
|------------------|----------------------|-----------------------------|
| `:fg`            | `--fg`               | All foreground text color   |
| `:fg-key`        | `--fg-key`           | Key text color              |
| `:fg-delimiter`  | `--fg-delimiter`     | Delimiter text color        |
| `:fg-prefix`     | `--fg-prefix`        | Prefix text color           |
| `:fg-chord`      | `--fg-chord`         | Chord text color            |
| `:fg-title`      | `--fg-title`         | Title text color            |
| `:fg-goto`       | `--fg-goto`          | Goto text color             |
| `:fg-header`     | `--fg-header`        | Group-header text color     |
| `:bg`            | `--bg`               | Background color            |
| `:bd`            | `--bd`               | Border color                |
| `:shell`         | `--shell`            | Shell for command execution |
| `:font`          | `--font`             | Menu font (Pango format)    |
| `:title`         | `--title`            | Global title text           |
| `:title-font`    | `--title-font`       | Title font (Pango format)   |
| `:delimiter`     | -                    | Default command delimiter   |
| `:wrap-cmd`      | `--wrap-cmd`         | Global command wrapper      |
| `:header-align`  | `--header-align`     | Group-header text alignment (`left`, `center`, or `right`) |
| `:header-font`   | `--header-font`      | Group-header font (Pango format); falls back to the menu font when unset |

```wks
:bg "#27212E"
:fg-key "#74DFC4"
:bd "#EB64B9"
:font "JetBrains Mono, 14"
```

### Include Macro

The `:include` macro inserts the contents of another wks file,
similar to C's `#include`. Relative paths resolve from the directory
of the file being processed.

```wks
b "Browser" { :include "browser.wks" }
e "Emacs" ^before %{{switch-to-workspace 1}} { :include "emacs.wks" }
```

```{note}
Self-includes and recursive includes produce an error. The same file
may be included multiple times. Unlike C's `#include`, the `:include`
macro can appear anywhere - even mid-description (though you probably
shouldn't).
```

See the [modular configs example](../examples/modular-configs).

### Var Macro

The `:var` macro defines a variable for use in descriptions, commands,
and other preprocessor arguments. It takes two string arguments: a
name and a value.

```wks
:var "CMD" "hyprctl dispatch workspace"
[asdfghjkl] "Workspace %(index+1)" %{{%(CMD) %(index)}}
```

Setting the value to an empty string (`""`) unsets the variable. The
variable name can contain any character except `)` and cannot shadow
built-in chord metadata identifiers.

Both name and value support variable interpolation, enabling
**meta-variables** (variables with computed names) and **chained
references**:

```wks
# Meta-variable: name computed from another variable
:var "key" "GREETING"
:var "%(key)" "Hello, World!"
a "Say hello" %{{echo %(GREETING)}}

# Chained references
:var "home" "/home/user"
:var "config" "%(home)/.config/wk"
```

Variables are resolved at definition time in a single pass - define
them before use.

A common pattern is combining `:var` with `:include` for
environment-specific configs:

````{tab} Hyprland
```{code-block} wks
:caption: `~/hyprland.wks`

:var "WORKSPACE_CMD" "hyprctl dispatch workspace"
:include "main.wks"
```
````

````{tab} dwm
```{code-block} wks
:caption: `~/dwm.wks`

:var "WORKSPACE_CMD" "xdotool set_desktop"
:include "main.wks"
```
````

````{tab} Shared
```{code-block} wks
:caption: `~/main.wks`

w "Workspace"
{
    [asdfghjkl] "%(index+1)" %{{%(WORKSPACE_CMD) %(index)}}
}
```
````

See the [variables example](../examples/var-macros).

### Switch Macros

Switch macros are simple toggles with no argument.

| Macro       | Equivalent CLI flag | Description             |
|-------------|---------------------|-------------------------|
| `:debug`    | `--debug`           | Print debug info        |
| `:unsorted` | `--unsorted`        | Disable sorting         |
| `:top`      | `--top`             | Position menu at top    |
| `:bottom`   | `--bottom`          | Position menu at bottom |
| `:center`   | `--center`          | Position menu at center |
| `:release`  | `--release`         | Release grab by default |

### Integer Macros

Take a positive or negative integer argument.

| Macro            | Equivalent CLI flag  |
|------------------|----------------------|
| `:menu-width`    | `--menu-width`       |
| `:menu-gap`      | `--menu-gap`         |
| `:table-padding` | `--table-padding`    |

### Unsigned Macros

Take a positive integer argument.

| Macro              | Equivalent CLI flag  |
|--------------------|----------------------|
| `:max-columns`     | `--max-columns`      |
| `:border-width`    | `--border-width`     |
| `:width-padding`   | `--wpadding`         |
| `:height-padding`  | `--hpadding`         |
| `:delay`           | `--delay`            |
| `:keep-delay`      | `--keep-delay`       |

### Number Macros

Take a decimal number argument.

| Macro            | Equivalent CLI flag  |
|------------------|----------------------|
| `:border-radius` | `--border-radius`    |

## Inheritance

Hooks and flags on a prefix are inherited by its direct child chords,
but **not** by nested prefixes. Use `+inherit` to opt a nested prefix
in, or use the negation flags (`+unhook`, `+deflag`, `+ignore`,
`+no-before`, `+no-after`) to opt individual chords out.

```wks
a "Prefix" +write
{
    w "Written" %{{I get written!}}
    n "Nested"
    {
        r "Runs" %{{echo "I get run!"}}
    }
}
# a w -> prints "I get written!" to stdout
# a n r -> executes echo "I get run!"
```

To make the nested prefix inherit:

```wks
a "Prefix" +write
{
    w "Written" %{{I get written!}}
    n "Nested" +inherit
    {
        r "Also written" %{{I also get written!}}
    }
}
```

## Sorting

Key chords are sorted alphabetically by default. Index interpolations
are resolved **before** sorting, so `%(index)` reflects parse order,
not display order.

```wks
[neio] "Switch %(index+1)" %{{xdotool set_desktop %(index)}}
b "Second" +write %{{%(index)}}
a "First" +write %{{%(index)}}

# Display order (sorted): a, b, e, i, n, o
# But: a's command is %{{5}}, b's is %{{4}}, n's is %{{0}}, etc.
```

Disable sorting with `--unsorted` or `:unsorted`.

A grouped scope (see [@group](#group)) sorts differently: the primary
key is the group name (alphabetical), the secondary key is the chord
key within each column. Chords sharing a group name coalesce into a
single column no matter where they appear in the file. With
`:unsorted`, both chords and `@group` blocks keep declaration order;
only **adjacent** `@group` blocks sharing a name coalesce - the same
name used again later, with a different group in between, produces a
second column with that header. Group names are compared byte-wise
(case-sensitive), so uppercase names sort before lowercase ones.

## Bug Reports

If you find a bug, please report it at
https://github.com/3L0C/wk/issues.

## Authors

3L0C \<dotbox at mailbox.org\>
