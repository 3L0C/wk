# Which Key Source Files

wks - A which-key source file used by the [wk](cli) program.

## Description

**wk**'s configuration files use the **wks** syntax to generate key chords.

## Grammar Rules

The following are *ideally* the formal grammar rules for **wks**. I have tried to
note where behavior differs from the expectation. If anything is not behaving as
you expect, please see [Authors](#authors) below to reach out.

```text
key_chord          -> ( chord | prefix | chord_array ) ;

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

meta_command       -> '@' ( 'goto' description ) ;

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
                      | 'bg'
                      | 'bd'
                      | 'shell'
                      | 'font'
                      | 'title'
                      | 'title-font'
                      | 'delimiter'
                      | 'wrap-cmd'
                      | 'var' '"' ( '\\"' | [^"] | user_variable )* '"' ) '"' ( '\\"' | [^"] | user_variable )* '"' ;

switch_macro       -> ( 'debug'
                      | 'sort'
                      | 'top'
                      | 'bottom' );

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

```{note}
In *wks* files, comments can be added using the pound character (*#*).
When a pound character is encountered, it signifies the start of a comment. The
comment extends from the pound character until the end of the line. It's
important to note that the pound character is treated as a literal character
within *descriptions* and *commands* and does not indicate the start of a
comment in those contexts.
```

## Key Chord

A *key chord* is the top-level construct in the grammar and represents a
complete key chord definition.

```text
key_chord -> ( chord | prefix | chord_array ) ;
```

It can be either a *prefix*, a *chord*, or a *chord array*.

## Chord

A *chord* is a *key chord* that results in **wk** performing some action, like
executing a command, when the trigger key is pressed.

```text
chord -> trigger_key description keyword* command ;
```

All chords must have a *trigger key*, *description*, and a *command*. Zero or
more *keywords* may be given between the *description* and *command*.

## Trigger Key

A *trigger key* represents the specific keypress or key combination that
triggers a corresponding action or command. In a *wks* file, it is the written
representation of the physical key(s) pressed by the user on their keyboard.

```text
trigger_key -> modifier* ( normal_key | special_key | key_options ) ;
```

A *trigger key* is then zero or more *modifiers* followed by a *normal key*,
a *special key*, or a set of *key options*.

## Normal Key

A *normal key* is any printable, non-whitespace, utf8 character.

```text
normal_key -> ( '\\' [\\\[\]{}#":^+()] | [^\s\[\]{}#":^+()] ) ;
```

Certain characters have special meanings in *wks* files. To use these characters
as a normal key, simply precede them with a backslash (*\\*).

**\[**
: Begins a *chord array*.

**\]**
: Ends a *chord array*.

**\{**
: Begins a *prefix* block.

**\}**
: Ends a *prefix* block.

**\#**
: Begins a comment.

**"**
: Begins and ends a *description*.

**:**
: Begins a *preprocessor macro*.

**^**
: Begins a *hook*.

**+**
: Begins a *flag*.

**(**
: Begins a *chord expression*.

**)**
: Ends a *chord expression*.

All other non-whitespace, printable utf8 characters prior to a description will
be interpreted as a normal key. Those that are whitespace or non-printable fall
into the special key category.

## Special Key

Special keys like *tab*, *escape*, *spacebar*, and *F1* can still be used as
trigger keys in *wks* files via their special forms.

```text
special_key -> ( 'Left'    | 'Right'   | 'Up'     | 'Down' | 'BS'
               | 'TAB'     | 'SPC'     | 'RET'    | 'DEL'  | 'ESC'
               | 'Home'    | 'PgUp'    | 'PgDown' | 'End'  | 'Begin'
               | 'VolDown' | 'VolMute' | 'VolUp'  | 'Play' | 'Stop'
               | 'Prev'    | 'Next'    | 'F'[1-35] ) \s+ ;
```

Each form should indicate the special key it represents but here is a chart to
make things explicit.

**Left**
: Left arrow

**Right**
: Right arrow

**Up**
: Up arrow

**Down**
: Down arrow

**BS**
: Backspace

**TAB**
: Tab

**SPC**
: Space

**RET**
: Enter/Return

**DEL**
: Delete

**ESC**
: Esc

**Home**
: Home

**PgUp**
: Page up

**PgDown**
: Page down

**End**
: End

**Begin**
: Begin

**F\[1-35\]**
: Function keys 1 through 35.

**VolDown**
: Volume Down

**VolMute**
: Mute Vol

**VolUp**
: Volume Up

**Play**
: Play Audio

**Stop**
: Stop Audio

**Prev**
: Audio Previous

**Next**
: Audio Next

In *wks* files, whitespace is generally not significant around individual parts
of the syntax, with one notable exception: *special keys*. When using *special
keys*, it is required to include whitespace between the end of the special key
and the start of the next item in the *wks* file.

If you have any additional special keys that you would like *wks* files to
support, please open an issue or a pull request.

## Modifier

As mentioned above, zero or more *modifiers* can be given in a *trigger key*.

```text
modifier -> ( 'C' | 'H' | 'M' | 'S' ) '-' ;
```

Modifiers can be used in *wks* files via their special forms.

**C-**
: *Control* key

**H-**
: *Hyper* key

**M-**
: *Meta* key

**S-**
: *Shift* key

Modifiers act as one would expect. To match the keypress *Control+c* use the
form *C-c* in your *wks* file.

Among the modifiers, the Shift modifier (*S-*) has a unique behavior when used
with *normal keys*. Due to the way normal keys are interpreted, the *S-*
modifier is not always necessary. To determine whether *S-* is required, it is
recommended to test the character in a *wks* file by typing it with and without
the Shift key pressed.

If the character is non-whitespace, printable, and the shifted and unshifted
versions produce different output, then the *S-* modifier is not needed. For
instance, pressing the *a* key with the Shift key held down produces an
uppercase *A*. This test demonstrates that the key's output changes based on the
Shift key state.

In such cases, using *S-a* in a *wks* file would not work as expected because
the key will never match when the user presses *Shift+a*.

I am open to changing it so that *S-a* and *A* match the same *Shift+a*
keypress, but I have yet to find a fitting solution. The ones I can think of
either involve depending on some utf8 library, writing the code by hand, or
permitting this syntax for ASCII but not other character sets. Each has its own
drawback, and I find the current solution to be intuitive in practice.

## Key Options

*Key options* allow users to bind a *chord* to one of many *trigger keys*.

```text
key_options -> '<' ( modifier* ( normal_key | special_key | '...' ) )+ '>' ;
```

When a *key options* construct is encountered, the parser selects the **first
unbound key** from the list. If all keys in the options are already bound in the
current scope, an error is raised.

This is useful when *key chords* are generated in some dynamic fashion, or
pulled from a generic context through an *:include*. By providing multiple key
options, chords can gracefully resolve to available keys without conflicts.

The ellipsis (*...*) within key options expands to all *implicit array keys*
(defined by *implicitArrayKeys* in config). This is useful as a fallback when
preferred keys may already be taken:

```wks
<s i g ...> "Signal" %{{signal}}
```

In this example, *s* is tried first, then *i*, then *g*, and finally the
implicit array keys in order until an unbound key is found.

```{note}
Modifiers given before the opening *<* apply to all keys within the
options. Modifiers can also be given to individual keys inside the options:
```

```wks
M-<a b>      # Both options get M- modifier
<a M-b>      # Only 'b' gets M- modifier
```

See [Examples](#key-options-1) for a full demonstration.

## Description

A *description* provide a hint about the purpose of the *chord* or *prefix*.

```text
description -> '"' ( '\\"' | [^"] | interpolation )* '"' ;
```

A *description* starts with a double quote (*"*), followed by zero or more of
the following:

**\\"**
: Escaped double quotes.

**\[^"\]**
: Any non-double quote character.

**interpolation**
: An interpolation.

A *description* ends with a double quote. Aside from *interpolations*, a
*description* looks like your typical string in many programming languages.

## Command

A *command* is some action to be executed upon completing a *key chord* sequence.

```text
command -> '%' delimiter ( . | interpolation )* delimiter ;
```

A *command* begins with the percent character (*%*) followed by a *delimiter*.
After the *delimiter* zero or more characters, or *interpolations* may be given.
A *command* is ended with the same delimiter that followed the percent character.

Because the *delimiter* is user defined, there should be no misinterpretation of
anything between the delimiters. This means any command given at the
command-line should be right at home in between the delimiters.

## Delimiter

A *delimiter* acts as a start and stop marker for a *command* in a *wks* file.

```text
delimiter   -> ( open_delim | close_delim | ([^[{(])\1 )  ;

open_delim  -> ( '{{' | '((' | '[[' ) ;

close_delim -> ( '}}' | '))' | ']]' ) ;
```

A *delimiter* may be one of the following:

**open_delim** or **close_delim**
: The opening and closing delimiters are special delimiters that that have an
  inverse match. If an opening delimiter is given then the corresponding
  closing delimiter is required to end the command (e.g., *\{\{* matches *\}\}*
  and so forth).

**(\[^\[{(])\1**
: Any **ASCII** character that is not any opening bracket (*\[*), opening brace
  (*{*), or any opening parenthesis (*(*), given twice. **NOTE** this excludes
  null bytes (*\0*) as these will indicate the end of a *wks* file or script.
  When an arbitrary delimiter is given the same character is expected to be
  repeated to indicate the end of a command.

The *delimiter* from one *command* to the next may be completely different. This
puts the burden on the user to ensure their *delimiter* is compatible with the
content of the command.

Here are some examples of different delimiters for the same command.

```wks
# Commands with opening and closing delimiters
%{{echo "hello, world"}}
%((echo "hello, world"))
%[[echo "hello, world"]]

# Valid arbitrary delimiters
%||echo "hello, world"||
%%%echo "hello, world"%%
%zzecho "hello, world"zz
```

Inspired by **sed**(1), this should keep *wks* syntax compatible with shell
commands, almost indefinitely. It also makes it possible to nest a *wks* script
within a *wks* command if you want to get really weird.

## Prefix

A *prefix* is a special type of *key chord* that acts as a container for other
*key chords*. It represents an incomplete key combination that does not trigger
a *command* on its own.

```text
prefix -> trigger_key description keyword* '{' ( key_chord )+ '}' ;
```

A *prefix* has many of the same components as a *chord*. It begins with a
*trigger key*, followed by a *description*, zero or more *keywords* and then a
block of one or more *key chords* surrounded by an opening and closing brace
(*{*, and *}*).

**Note** that a key chord may be a *prefix*, a *chord*, or a *chord array*,
meaning many prefixes can be nested one inside another.

Here is a simple example of a prefix:

```wks
m "+Music"
{
    n "Next" %{{mpc next}}
    p "Prev" %{{mpc prev}}
}
```

## Chord Array

*Chords* and *prefixes* are standard fare in the realm of key chords, so what
the heck is a *chord array*? Well, mostly syntactic sugar so you do not have to
repeat yourself when it comes to *chords* that are very similar but only differ
in slightly different ways.

```text
chord_array -> ( implicit_array | explicit_array ) ;
```

A *chord array* comes in two flavors, *implicit* and *explicit*.

## Implicit Array

An *implicit array* is the simplest of the two flavors. It utilizes the
*implicitArrayKeys* variable defined in *config.def.h* to generate *chords* from
these *trigger keys*.

```text
implicit_array -> modifier* '...' description keyword* command ;
```

An *implicit array* is then zero or more modifiers, an ellipsis (*...*), a
description, zero or more keywords, and a command. This is practially a *chord*
in terms of its form, but in behavior an *implicit array* generates any number
of *chords* from this simple syntax.

As an example, say your implicit array keys are set to *h*, *j*, *k*, and *l*,
and you have this *wks* file:

```wks
... "Switch workspace %(index+1)" %{{xdotool set_desktop %(index)}}
```

This is the equivilant *wks* file without the use of an *implicit array*:

```wks
h "Switch workspace 1" %{{xdotool set_desktop 0}}
j "Switch workspace 2" %{{xdotool set_desktop 1}}
k "Switch workspace 3" %{{xdotool set_desktop 2}}
l "Switch workspace 4" %{{xdotool set_desktop 3}}
```

## Explicit Array

An *explicit array* is most useful when the desired *chords* are less homogeneous.

```text
explicit_array -> '[' ( trigger_key | chord_expression )+ ']' description keyword* command ;
```

To use an *explicit array* begin with an open bracket (*\[*) followed by one or
more *trigger keys* or *chord expressions*. The array portion ends with a
closing bracket (*\]*) followed by the standard chord components, a description,
zero or more keywords, and a command.

I think an example will make things clear:

```wks
# Chord array version
[asdfghjkl] "Switch workspace %(index+1)" %{{xdotool set_desktop %(index)}}

# Individual chords and no interpolation
a "Switch workspace 1" %{{xdotool set_desktop 0}}
s "Switch workspace 2" %{{xdotool set_desktop 1}}
d "Switch workspace 3" %{{xdotool set_desktop 2}}
f "Switch workspace 4" %{{xdotool set_desktop 3}}
g "Switch workspace 5" %{{xdotool set_desktop 4}}
h "Switch workspace 6" %{{xdotool set_desktop 5}}
j "Switch workspace 7" %{{xdotool set_desktop 6}}
k "Switch workspace 8" %{{xdotool set_desktop 7}}
l "Switch workspace 9" %{{xdotool set_desktop 8}}
```

In this case, *explicit arrays* are only slightly different than an *implicit
array*. However, *explicit arrays* support *chord expressions* which make them
far more flexible.

## Chord Expression

Explicit arrays can be very simple with each *chord* being only slightly
different from one another. However, it may make sense to include chords that
mostly fit into the *explicit array* with some more distinct differences. For
this situation, *chord expressions* may be the answer.

```text
chord_expression -> '(' trigger_key description keyword* command? ')' ;
```

A *chord expression* is only valid within a *chord array*, and it is essentially
a *chord* wrapped in parentheses with some added flexibility. Normally, a *chord*
requires at least a *trigger key*, a *description*, and a *command*. A *chord
expression*, on the other hand, requires only a *trigger key* and a
*description*. Any other information will be filled in by the surrounding *chord
array*.

Here is an example of a chord expression within a *chord array*:

```wks
# With chord arrays and chord expressions
[
    (b "Brave")
    (c "Mullvad Chrome" %{{mullvad-exclude chrome ~/startpage.html}})
    x
] "XDG-OPEN" %{{%(desc,,) ~/startpage.html}}

# With chords and no interpolation
b "Brave" %{{brave ~/startpage.html}}
c "Mullvad Chrome" %{{mullvad-exclude chrome ~/startpage.html}}
x "XDG-OPEN" %{{xdg-open ~/startpage.html}}
```

Admittedly, *chord expressions* may not be that useful but they were easy to
implement so they are here for those who want to use them.

## Interpolation

An *interpolation* is a means of accessing chord metadata or user-defined
variables from within a *description*, *command*, or *preprocessor macro
argument*.

```text
interpolation  -> '%(' ( chord_metadata | arg_position | user_variable ) ')' ;
```

The basic syntax for an *interpolation* begins with a *%(* delimiter followed by
a *chord_metadata* identifier, *arg_position*, or *user_variable* name, and
closing parenthesis (*)*).

There are three types of interpolations with different scopes:

**Chord Metadata Interpolations**
: Built-in identifiers providing access to metadata about the current chord.
  These are only valid in *descriptions* and *commands*.

**Argument Position Interpolations**
: Positional references to values defined with the *+args* flag. These are
  only valid in *descriptions* and *commands* where *+args* is defined.

**User Variable Interpolations**
: User-defined variables created with the *:var* preprocessor macro. These can
  be used in *descriptions*, *commands*, and *preprocessor macro arguments*.

## Chord Metadata

The following built-in identifiers provide access to chord metadata and are only
valid within *descriptions* and *commands*:

```text
chord_metadata -> ( 'key'
                  | 'index'
                  | 'index+1'
                  | 'desc'
                  | 'desc^'
                  | 'desc^^'
                  | 'desc,'
                  | 'desc,,'
                  | 'wrap_cmd' );
```

**key**
: The *key* *identifier* corresponds to the *trigger key* of the current
  *chord*. This makes the most sense to use within a *chord array* or for a
  *chord* that may change frequently or is not know ahead of time.

**index**
: The 0-based index based on parse order. Scoping rules:
  - Within a *chord array*, index is relative to the array (0 to N-1)
  - For standalone chords, index is the position within the current scope
  - A *prefix* starts a new scope for its children

  Index values are resolved before sorting.

**index+1**
: Same as *index* but 1-based (1 to N).

**desc**
: The *desc* *identifier* correspond to the *description* of the current
  *chord* or *prefix*. The *desc* *identifier* may not be given within a
  *description*. An error will be thrown in the case where this is attempted.

**desc^**
: The *description* of the current *chord* with the **first** character
  capitalized.

**desc^^**
: The *description* of the current *chord* with the **all** characters
  capitalized.

**desc,**
: The *description* of the current *chord* with the **first** character
  downcased.

**desc,,**
: The *description* of the current *chord* with the **all** characters downcased.

**wrap_cmd**
: The value of the global wrap command. This is defined by the *:wrap-cmd*
  preprocessor macro, the *--wrap-cmd* cli flag, or defined by *wrapCmd*
  in your *config.\[def.\]h* file.

## User Variable

User-defined variables created with the *:var* preprocessor macro can be
accessed through interpolation:

```text
user_variable -> [^)]+ ;
```

Unlike *chord_metadata*, user variables can be used in three contexts:

- Descriptions
- Commands
- Preprocessor macro arguments (*:font*, *:fg-\**, *:bg*, *:bd*, *:shell*, *:delimiter*, *:wrap-cmd*, *:include*, and in *:var* itself)

This enables powerful meta-programming capabilities such as meta-variables
(variables with computed names) and dynamic configuration values. See [Var
Macro](#var-macro) and [Examples](#examples) for demonstrations.

## Argument Position

Argument positions allow accessing values defined with the *+args* flag:

```text
arg_position -> '$' [0-9]+ ;
```

The syntax *%($N)* accesses the Nth argument (0-indexed). Arguments are only
valid when defined via *+args* on the current chord or an enclosing prefix.

**$0**
: First argument

**$1**
: Second argument

**$N**
: Nth argument (0-indexed)

Arguments follow lexical scoping: all descendants within a *prefix* can access
that prefix's arguments, not just direct children. When a chord defines its own
arguments, they shadow ancestor arguments at the same index, but other indices
remain accessible from ancestors.

Undefined argument positions resolve to an empty string.

See [Examples](#args) for demonstrations of argument positions.

## Keyword

A *keyword* is an optional instruction to modify the behavior of a *chord* or
*prefix*.

```text
keyword -> ( hook | flag ) ;
```

A *keyword* is either a *hook* or a *flag*. Both have equal precedence, meaning
they can be mixed up wherever they are permitted.

## Hook

Hooks provide means of adding additional commands to a chord or prefix.

```text
hook -> '^' ( 'before'
            | 'after'
            | 'sync-before'
            | 'sync-after' ) command ;
```

A *hook* begins with the caret character (*^*), followed by the type of *hook*,
and finally the command the *hook* will run.

The *hook* type has to do with the order the command will be run. The *before*
hooks run before the chord's command, and the *after* hooks run after the
chord's command.

The *sync-* hooks relate to how **wk** runs the commands. By default, all commands
are run asynchronously to prevent a command from blocking **wk**. However, if the
hook must complete before **wk** can proceed you can use the *sync-\** variant to
enforce this behavior.

```{note}
A blocking command may prevent **wk** from ever resuming execution. In
the event that this happens, users may need to restart their system entirely to
regain control of their keyboard.
```

See [Examples](#hooks) for further discussion about hooks.

## Flag

Flags are similar to command-line flags in that they change the behavior of **wk**.

```text
flag -> '+' ( 'keep'
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
            | 'wrap' '"' ( '\\"' | [^"] | interpolation )* '"'
            | 'title' ( '"' ( '\\"' | [^"] | interpolation )* '"' )?
            | 'args' ( '"' ( '\\"' | [^"] | interpolation )* '"' )+ ) ;
```

Flags begin with a plus character (*+*), followed by the flag itself. Here is
how each flag changes the behavior of **wk**:

**keep**
: Instead of closing after **wk** finds a matching chord, it keeps the **wk** menu
  open.

**close**
: Forces the **wk** window to close. Useful when *+keep* was given to a
  surrounding prefix.

**inherit**
: Causes the prefix to inherit flags and hooks from its parent. Has no effect
  when given to a chord.

**ignore**
: Ignore all hooks and flags from the surrounding prefix. Has no effect when
  given to a prefix.

**unhook**
: Ignore all hooks from the surrounding prefix.

**deflag**
: Ignore all flags from the surrounding prefix.

**no-before**
: Ignore *before* and *sync-before* hooks from the surrounding prefix.

**no-after**
: Ignore *after* and *sync-after* hooks from the surrounding prefix.

**write**
: Write commands to stdout rather than executing them.

**execute**
: Execute the command rather than writing them to stdout. Useful when *+write*
  was given to a surrounding prefix.

**sync-command**
: Execute the command in a blocking fashion. See the note in [Hook](#hook) regarding
  potential issues with blocking commands.

**unwrap**
: Prevent wrapping this chord, even if a global wrap is set or inherited
  from a parent prefix.

**wrap** '"' ( '\\"' | \[^"\] | interpolation )\* '"'
: When given to a prefix, wrap chord commands with the argument. Can be given
  to a chord, or chord-array to wrap the immediate command(s). The argument is
  a string that supports interpolation, the same syntax as a description. The
  "wrap" is applied as "/bin/sh -c WRAP CMD". See [Examples](#wrapping-commands) for
  scenarios where this is useful.

**title** ( '"' ( '\\"' | \[^"\] | interpolation )\* '"' )?
: Set a title for the chord or prefix that is displayed above the menu. The
  argument supports interpolation, the same syntax as a description. Overrides
  the global *--title* setting. Empty string *""* clears any inherited or
  global title. Nested prefixes display their own title when specified.
  Can ommit the argument to set the title to the chord's description.

**args** ( '"' ( '\\"' | \[^"\] | interpolation )\* '"' )+
: Define positional arguments that can be accessed via *%($0)*, *%($1)*, etc.
  in descriptions and commands. Takes one or more string arguments (same syntax
  as a description). Arguments are 0-indexed: *%($0)* is the first argument.

  When given to a *prefix*, all descendant chords can access the arguments -
  not just direct children, but grandchildren and beyond. When a descendant
  defines its own *+args*, those values shadow ancestor arguments at the same
  index, but unspecified indices remain accessible from ancestors.

  In *chord arrays*, *+args* can be given to the template (after the closing
  bracket) to provide default arguments. These defaults are shadowed by any
  *+args* given to individual *chord expressions* within the array.

  Undefined argument positions resolve to an empty string.

See [Examples](#flags) for further discussion about flags.

## Meta Command

A *meta command* is a special directive that controls the **wk**'s menu itself
rather than executing a shell command. Meta commands are mutually exclusive
with *hooks* and regular *commands*.

```text
meta_command -> '@' ( 'goto' description ) ;
```

A *meta command* begins with the at symbol (*@*) followed by the meta command
type and its argument(s), if any.

**@goto**
: Navigates to a different location in the key chord hierarchy without
  closing and restarting **wk**. The argument is a *description* containing
  the path to navigate to, following the same syntax as the **--press** flag.

  - *@goto ""* - Navigate to the root menu
  - *@goto "w"* - Navigate to the "w" prefix
  - *@goto "w m"* - Navigate through "w" then "m"

  If the path leads to a *chord* with a *command* (not a *prefix*), that
  command is executed.

  ```{note}
  *@goto* cannot be combined with *hooks* (*^before*, *^after*)
  or regular *commands* (*%\{\{\}\}*). Circular goto chains (e.g., *a @goto "b"*
  and *b @goto "a"*) are detected at runtime and result in an error.
  ```

See [Examples](#meta-commands) for further discussion about meta commands.

## Preprocessor Macros

There are a number of preprocessor macros that can be used in *wks* files. These
have a number of uses from making *wks* files more modular to controlling the
look and feel of [wk](cli).

```text
preprocessor_macro -> ':' ( string_macro
                          | switch_macro
                          | integer_macro
                          | unsigned_macro
                          | number_macro ) ;
```

A preprocessor macro begins with the colon character (*:*) followed by a
specific macro form.

The majority of macros correspond to the command-line arguments that [wk](cli)
supports. When given, these override anything given at the command-line. They
are here to provide a baked-in alternative to the command-line versions making
it easy to simply run the *wks* file and get the desired look and feel without
having to give the same arguments each time. It can also help distinguish the
purpose of the key chords if it is intended to be used as part of a script by
making the [wk](cli) popup window different from the builtin settings.

## String Macros

String macros require a string argument.

```text
string_macro -> ( 'include'
                | 'fg-color'
                | 'fg-key'
                | 'fg-delimiter'
                | 'fg-prefix'
                | 'fg-chord'
                | 'fg-title'
                | 'fg-goto'
                | 'bg-color'
                | 'bd-color'
                | 'shell'
                | 'font'
                | 'title'
                | 'title-font'
                | 'wrap-cmd'
                | 'var' '"' ( '\\"' | [^"] )* '"' ) '"' ( '\\"' | [^"] )* '"' ;
```

Many of the macros here work the same as their command-line counterparts. Simply
use **:MACRO "ARGUMENT"** to make use of any string macro, (e.g. **:shell
"/usr/bin/env zsh"**).

## Include Macro

The *:include* macro is not present as a command-line argument to [wk](cli). This
is because this macro has more to do with *wks* files than the look and feel of
[wk](cli). The *:include* macro works similarly to the *#include* macro found in
C/C++. It allows users to bring other *wks* files into a single file. **NOTE**,
self includes and recursive includes are not permitted and will cause an error.
**NOTE**, the same file may be included multiple times. This is not an error, and
may even be desirable for some users. **NOTE**, while the *#include* macro in
C/C++ has restrictions on where it can go in a file, the *:include* macro in a
*wks* file may go literally anywhere. As for file resolution, it's pretty
simple. A relative path is assumed to be in the same directory as the file being
processed, and absolute paths are just that, absolute.

See [Examples](#the-include-macro) for a full demonstration of the *:include* macro.

## Var Macro

The *:var* macro allows you to define a variable with some value. It takes two
arguments, the first is the *key* or *variable name*, and the second is the
*value* which can be an empty string, i.e., unset a previously defined *var*.

```{note}
Both arguments support variable interpolation, enabling meta-variables
(variables with computed names) and variables that reference other variables.
```

The *key* can contain any character except a closing parenthesis ('*)*'),
otherwise it would be inaccessible through interpolation. The *key* cannot
shadow builtin chord metadata identifiers.

**Basic Variables**

The simplest use is defining a variable and using it in chord descriptions or
commands:

```wks
:var "WORKSPACE_CMD" "hyprctl dispatch workspace"
[asdfghjkl] "Workspace %(index+1)" %{{%(WORKSPACE_CMD) %(index)}}
```

**Meta-Variables**

Variable names support interpolation, enabling variables with computed names:

```wks
# Variable name from another variable
:var "key_name" "GREETING"
:var "%(key_name)" "Hello, World!"

# Multi-part variable names
:var "prefix" "MY"
:var "suffix" "VAR"
:var "%(prefix)_%(suffix)" "success"
```

```{note}
Variables used in the name must be defined before the *:var* directive.
```

**Variables Referencing Variables**

Variable values can reference other variables:

```wks
:var "original" "first"
:var "derived" "%(original)"  # derived = "first"

:var "base" "foo"
:var "extended" "%(base)_bar"  # extended = "foo_bar"
```

Variables are resolved at definition time, so referenced variables must be
defined first.

**Variables in Preprocessor Directives**

All preprocessor macros that take string arguments support variable
interpolation:

```wks
# Color scheme
:var "r" "ff"
:var "g" "00"
:var "b" "00"
:fg "#%(r)%(g)%(b)"

# Font configuration
:var "font_name" "Monospace"
:var "font_size" "12"
:font "%(font_name), %(font_size)"
```

**Variable Resolution Order**

Variables are resolved at definition time in a single pass:

1. Variable names are resolved first (meta-variables)
2. Variable values are then resolved
3. Undefined variables in either context cause an error

This prevents circular references but requires that variables be defined before
they are used.

See [Examples](#the-var-macro) for more demonstrations.

## Switch Macros

Switch macros are the simplest of the bunch. They are essentially an on switch
for the corresponding menu settings.

```text
switch_macro -> ( 'debug'
                | 'sort'
                | 'top'
                | 'bottom' );
```

All the switch macros correspond to their cli flags for [wk](cli).

## Integer Macros

The integer macros require a positive or negative integer argument to the macro.

```text
integer_macro -> ( 'menu-width'
                 | 'menu-gap' ) '-'? [0-9]+ ;
```

All the integer macros correspond to their cli flags for [wk](cli).

## Unsigned Macros

The unsigned macros require a positive integer argument to the macro.

```text
unsigned_macro -> ( 'max-columns'
                  | 'border-width'
                  | 'width-padding'
                  | 'height-padding'
                  | 'delay'
                  | 'keep-delay' ) [0-9]+ ;
```

All the unsigned macros correspond to their cli flags for [wk](cli).

## Number Macros

The number macros require a positive number argument to the macro.

```text
number_macro -> ( 'border-radius' ) '-'? [0-9]+ ( '.' [0-9]* )? ;
```

All the number macros correspond to their cli flags for [wk](cli).

## Examples

### Hooks

Users can certainly chain commands together the same way one would chain
commands in a regular shell, but hooks help to reduce repetition. They also make
more sense in the context of prefixes.

```wks
# With hooked prefix
e "+Emacs" ^before %{{xdotool set_desktop 1}}
{
    o "Open" %{{emacsclient -c -a ""}}
    r "Roam" %{{emacsclient -c -a "" ~/20240101080032-startpage.org}}
}

# Without hooks
e "+Emacs"
{
    o "Open" %{{xdotool set_desktop 1 ; emacsclient -c -a ""}}
    r "Roam" %{{xdotool set_desktop 1 ; emacsclient -c -a "" ~/20240101080032-startpage.org}}
}
```

As you can see, this helps to cut down on repetition, but it also helps enforce
a workflow rule without the need to setup desktop environment rules and such.

This example also hints at the idea of inheritance as the hook was given to a
prefix and not to individual chords. This topic is covered after introducing
flags as these also factor into the discussion.

### Flags

Each flag has a time and a place but I find *+keep*, and *+write* to be the most
useful out of the bunch.

The *+keep* flag can turn *wk* into a hydra of sorts. I use this to control
music playback on my system like this:

```wks
m "+Music" +keep
{
    c "Clear mpc" %{{mpc clear}}
    d "Display Song" %{{songinfo}}
    h "Seek -5" %{{mpc seek "-5"}}
    l "Seek +5" %{{mpc seek "+5"}}
    n "Next song" %{{mpc next}}
    p "Prev song" %{{mpc prev}}
    o "Open mpc" +close %{{st -e ncmpcpp}}
    y "Playlist" +close %{{st -e ncmpcpp --screen playlist}}
}
```

The *+write* flag is useful for scripting purposes. In the same way that
**dmenu**(1) and co print selections to stdout, this turns **wk**(1) into a prompt
for users to choose from some list of options with less typing.

### Args

The *+args* flag enables parameterized chord definitions, reducing repetition
when multiple chords share similar patterns.

**Basic Usage**

A single chord with multiple arguments:

```wks
# Pressing 'a' outputs: Hello World!
a "Greet" +args "Hello" "World" +write %{{%($0) %($1)!}}
```

**Chord Arrays with Per-Chord Args**

Each chord in an array can have different arguments:

```wks
[
    (a "Alice" +args "alice@example.com")
    (b "Bob" +args "bob@example.com")
    (c "Charlie")
] "Email %($0)" +args "(null)" %{{mailto:%($0)}}
# 'a' -> mailto:alice@example.com
# 'b' -> mailto:bob@example.com
# 'c' -> mailto:(null)
```

**Prefix Inheritance**

All descendants can access ancestor arguments:

```wks
p "+Prefix" +args "outer0" "outer1"
{
    a "Use both" +write %{{%($0) %($1)}}
    n "+Nested" +args "inner0"
    {
        b "Mixed" +write %{{%($0) %($1)}}
    }
}
# 'p a' outputs: outer0 outer1
# 'p n b' outputs: inner0 outer1
```

**Arguments with Interpolations**

Arguments can contain other interpolations:

```wks
[abc] "Workspace %(index+1)" +args "ws-%(index)" +write %{{switch %($0)}}
# 'a' -> switch ws-0
# 'b' -> switch ws-1
# 'c' -> switch ws-2
```

**Nested Scopes**

Each scope maintains its own arguments:

```wks
o "+Outer" +args "outer-arg"
{
    i "+Inner" +args "inner-arg"
    {
        a "Access inner" +write %{{%($0)}}  # inner-arg
    }
    b "Access outer" +write %{{%($0)}}      # outer-arg
}
```

### Meta Commands

The *@goto* meta command is useful for creating "hydra" menus where users can
perform related actions and then navigate elsewhere without restarting **wk**.

```wks
w "+Window" +keep
{
    m "+Move" +keep
    {
        ... "Move to %(index+1)" %{{move-window %(index)}}
        BS "Go back" @goto "w"
        S-BS "Go home" @goto ""
    }
    r "+Resize" +keep +inherit
    {
        ... "Resize %(index+1)" %{{resize-window %(index)}}
    }
}
```

In this example, pressing *w m* enters the move submenu. After moving a window,
pressing *BS* returns to the window prefix, while *S-BS* returns to the root
menu.

### Wrapping Commands

There are two classes of wraps, globally through *:wrap-cmd*, *--wrap-cmd*,
and *wrapCmd*, and locally through the *+wrap* flag. All take a string
argument, but the flag supports interpolation.

The global wrap is most useful for someone using a uwsm managed wayland
environment. Passing *--wrap-cmd "uwsm-app --"* will ensure all commands are
prefixed with *uwsm-app --* before they are run, **or** written to stdout. The
following are equivilant examples:

```wks
# Wrapped
:wrap-cmd "uwsm-app --"
f "Firefox" %{{firefox}}

# Unwrapped
f "Firefox" %{{uwsm-app -- firefox}}
```

Local wraps may be useful with or without a global wrap. Here are some
examples:

```wks
:wrap-cmd "uwsm-app --"
b "+Browse" +wrap "%(wrap_cmd) firefox" # include the global wrap_cmd
{
    g "GNU" %{{gnu.org}}
    [
        (y "YouTube")
        (s "Soundcloud")
    ] "null" %{{%(desc,,).com}}
}
f "+Foot" +wrap "foot -e" # just wrap with "foot -e"
{
    n "ncmpcpp" %{{ncmpcpp}}
}
```

```{note}
On wrap precedence: When multiple wrap definitions exist, the most
specific takes precedence:

1. Chord-level +unwrap (no wrap)
2. Chord-level +wrap "custom" (use custom wrap)
3. Inherited +wrap from parent prefix
4. Global :wrap-cmd / --wrap-cmd / wrapCmd
5. No wrapper set (no wrapping)
```

### The Include Macro

Here is an example of the *:include* macro:

```wks
# File main.wks
# ---------------
# Browser prefix
b "+Browser" { :include "browser_key_chords.wks" }
# Emacs prefix
e "+Emacs" ^before %{{xdotool set_desktop 1}} { :include "emacs_key_chords.wks" }
# Music prefix
m "+Music" +keep { :include "music_key_chords.wks" }
```

```wks
# File browser_key_chords.wks
# -----------------------------
[
    (b "Brave")
    (c "Chrome")
    (f "Firefox")
] "null" %{{%(desc,,)}}

# Mullvad-exclude prefix
m "+Mullvad Exclude"
{
    [
        (b "Brave")
        (c "Chrome")
        (f "Firefox")
    ] "null" %{{mullvad-exclude %(desc_)}}
}
```

```wks
# File emacs_key_chords.wks
# ---------------------------
b "Open blank" %{{emacsclient -c -a ""}}
p "+Projects"
{
    w "wk" %{{emacs "~/Projects/wk"}}
}
```

```wks
# File music_key_chords.wks
# ---------------------------
c "Clear mpc" %{{mpc clear}}
d "Display song" %{{songinfo}}
h "Seek -5s" %{{mpc seek "-5"}}
l "Seek +5s" %{{mpc seek "+5"}}
n "Next song" %{{mpc next}}
p "Prev song" %{{mpc prev}}
o "Open mpc" +close %{{st -e ncmpcpp}}
```

This allows users to create key chords in a more modular manner. This can be
beneficial when you may want to reuse a *wks* file in a different context than
your main key chords.

You can even do silly things like this:

```wks
# File part_one.wks
# -------------------
A "silly :include "part_two.wks"

# File part_two.wks
# -------------------
example" %{{echo "You wouldn't do this right??"}}

# Resulting wks file
# --------------------
A "silly example" %{{echo "You wouldn't do this right??"}}
```

### Key Options

A set of *key options* is most useful when constructing *key chords*
in a dynamic fashion. Here is an attempt to use the *:include* macro
without *key options*:

```wks
# File chat.wks
s "Signal"
{
    # some bindings
}

# File music.wks
s "Spotify"
{
    # some bindings
}

# File main.wks
A "Apps"
{
    :include "chat.wks"
    :include "music.wks"
}
```

In this example, the second include overrides the first because they both use
the same *trigger key* and the last instance always wins out. This can be solved
with *key options*:

```wks
# File chat.wks
<s i g> "Signal"
{
    # some bindings
}

# File music.wks
<s p o> "Spotify"
{
    # some bindings
}

# File main.wks
A "Apps"
{
    :include "chat.wks"
    :include "music.wks"
}
```

Now, both paths exist, but their *trigger key* is not known without full
context. In the above example, **"Signal"** gets **s** while **"Spotify"** gets **p**.

```{note}
It is recommended that if one *key chord* uses *key options* all others
of the same scope should also use *key options*.
```

### The Var Macro

Here is an example of the *:var* macro:

```wks
# File main.wks
w "+Workspace"
{
    [asdfghjkl] "Workspace %(index+1)" %{{%(WORKSPACE_CMD) %(index)}}
}

# File hyprland.wks
:var "WORKSPACE_CMD" "hyprctl activateworkspace"
:include "main.wks"

# File dwm.wks
:var "WORKSPACE_CMD" "xdotool set_desktop"
:include "main.wks"
```

With some clever orchestration you can use vars to construct a more general **wk**
menu, free from concern about your local environment.

### Variables in Preprocessor Directives

All preprocessor macros that take string arguments support variable
interpolation, enabling powerful meta-programming capabilities:

**Meta-Variables**

Variables with computed names:

```wks
# Variable name from another variable
:var "key_name" "GREETING"
:var "%(key_name)" "Hello, World!"
a "Say hello" %{{echo %(GREETING)}}

# Multi-part variable names
:var "prefix" "MY"
:var "suffix" "CONFIG"
:var "%(prefix)_%(suffix)" "value"
b "Test" %{{echo %(MY_CONFIG)}}
```

**Color Scheme Composition**

Build colors dynamically from components:

```wks
# RGB components
:var "r" "ff"
:var "g" "5c"
:var "b" "57"

# Compose colors from components
:fg "#%(r)%(g)%(b)"
:bg "#1a1b26"

# Theme variables
:var "theme" "dark"
:var "accent_%(theme)" "#7fb4ca"
:bd "%(accent_dark)"
```

**Dynamic Configuration**

Configure fonts, shells, and other settings using variables:

```wks
# Font configuration
:var "font_family" "JetBrains Mono"
:var "font_size" "14"
:var "font_style" "Bold"
:font "%(font_family) %(font_style), %(font_size)"

# Shell configuration
:var "default_shell" "/bin/zsh"
:shell "%(default_shell)"

# Wrap command from variable
:var "container" "flatpak run"
:var "app_id" "org.mozilla.firefox"
:wrap-cmd "%(container) %(app_id)"
```

**Variables Referencing Variables**

Chain variable references for flexible configurations:

```wks
# Base paths
:var "home" "/home/user"
:var "config_dir" "%(home)/.config"
:var "wk_config" "%(config_dir)/wk"

# Derived values
:var "browser_cmd" "firefox"
:var "terminal" "alacritty"
:var "editor_cmd" "%(terminal) -e nvim"

# Use in commands
a "Edit config" %{{%(editor_cmd) %(wk_config)/keys.wks}}
```

**Environment-Specific Includes**

Combine variables with includes for environment-specific configs:

```wks
# File: common.wks
:var "env" "wayland"
:include "%(env)_specific.wks"

# File: wayland_specific.wks
:var "launcher" "wofi"
:var "term" "foot"

# File: x11_specific.wks
:var "launcher" "dmenu"
:var "term" "st"
```

## Notes

### Inheritance

Inheritance relates to hooks and flags given to prefixes. The idea is fairly
simple. A hook or flag given to a prefix is inherited by any chord within the
prefix. Nested prefixes do not inherit the hooks and flags given to their parent.

```wks
a "+Prefix" +write
{
    w "Write it!" %{{I get written!}}
    n "+Nested Prefix"
    {
        r "Run it!" %{{echo "I get run!"}}
    }
}
```

In the above example, the key chord **a w** causes **I get written!** to be printed
to stdout. The key chord **a n r** runs the command **echo "I get run!"**.

To force a nested prefix to inherit from its parent the *+inherit* flag must be
given. Additionally, if the prefix only wishes to inherit certain hooks or flags
additional flags may be given to ignore unwanted behavior.

### Sorting

Key chords are sorted by default when processing a *wks* file. Index
interpolations are resolved **before** sorting, so *%(index)* reflects parse
order, not final sorted position. This means sorting only changes display
order, not index values.

```wks
# Base file
[neio] "Switch %(index+1)" %{{xdotool set_desktop %(index)}}
b "Second" +write %{{%(index)}}
a "First" +write %{{%(index)}}

# Result (sorted by default, but indices reflect parse order)
a "First" +write %{{5}}
b "Second" +write %{{4}}
e "Switch 2" %{{xdotool set_desktop 1}}
i "Switch 3" %{{xdotool set_desktop 2}}
n "Switch 1" %{{xdotool set_desktop 0}}
o "Switch 4" %{{xdotool set_desktop 3}}
```

The array *\[neio\]* expands to indices 0-3 in parse order (n=0, e=1, i=2, o=3),
then *b* gets index 4 and *a* gets index 5. After sorting, the display order
changes but indices remain unchanged.

To disable sorting, use the **--unsorted** CLI flag or the **:unsorted**
preprocessor macro.

## Bug Reports

Please see [wk](cli) Bug Reports for info on reporting bugs.

## Authors

3L0C \<dotbox at mailbox.org\>
