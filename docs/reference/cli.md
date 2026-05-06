# Command Line Options

wk is a popup menu for your custom keyboard shortcuts.
Inspired by
[emacs-which-key](https://github.com/justbur/emacs-which-key),
[dmenu](https://tools.suckless.org/dmenu/), and
[bemenu](https://github.com/Cloudef/bemenu).

## Synopsis

**wk** [options]

## Description

**wk** provides visual feedback when executing user defined key chords via a popup
menu. The key chords may be precompiled into **wk** for users who maintain their
own copy of **wk**. For users looking for a more dynamic experience, **wk** can read
key chords from a [wks](wks) file or script.

## Options

**-h, --help**
: Display help message and exit.

**-v, --version**
: Display version number and exit.

**-d, --debug**
: Print debug information during execution.

**-D, --delay** *INT*
: Delay the popup menu by *INT* milliseconds from startup or last keypress
  (default 1000 ms).

**--keep-delay** *INT*
: Delay in milliseconds after keyboard ungrab before command execution for
  +keep chords. Helps prevent focus-stealing issues with compositor commands
  (default 75 ms).

**-t, --top**
: Position menu at top of screen.

**-b, --bottom**
: Position menu at bottom of screen.

**-c, --center**
: Position menu at center of screen.

**-s, --script**
: Read **wks** script from stdin to use as key chords.

**-U, --unsorted**
: Disable sorting of key chords (sorted by default).

**-m, --max-columns** *INT*
: Set the maximum menu columns to *INT* (default 5).

**-p, --press** *KEY(s)*
: Press *KEY(s)* before dispalying menu. See [Trigger Key](wks.md#trigger-keys)
  in the **wks** man page for more info.

**-T, --transpile** *FILE*
: Transpile *FILE* to valid 'key_chords.h' syntax and print to stdout.

**-k, --key-chords** *FILE*
: Use *FILE* for key chords rather than those precompiled.

**-w, --menu-width** *INT*
: Set menu width to *INT*. Set to '-1' for a width equal to 1/2 of the
  screen width (default -1).

**-g, --menu-gap** *INT*
: Set menu gap between top/bottom of screen to *INT*. Set to '-1' for a gap
  equal to 1/10th of the screen height (default -1).

**--wrap-cmd** *STRING*
: Wrap all chord commands by executing them as */bin/sh -c "STRING CMD"*
  where CMD is the original command. This does not apply to hooks (default "").

**--border-width** *INT*
: Set border width to *INT* (default 4).

**--border-radius** *NUM*
: Set border radius to *NUM* degrees. 0 means no curve (default 0).

**--wpadding** *INT*
: Set left and right padding around hint text to *INT* (default 6).

**--hpadding** *INT*
: Set up and down padding around hint text to *INT*. (default 2)

**--table-padding** *INT*
: Set additional padding between the outermost cells and the border to
  *INT*. -1 = same as cell padding, 0 = no additional padding (default -1).

**--fg** *COLOR*
: Set all menu foreground text to *COLOR* where color is some hex string
  i.e. '#F1CD39' (default unset).

**--fg-key** *COLOR*
: Set foreground key to COLOR (default '#DCD7BA').

**--fg-delimiter** *COLOR*
: Set foreground delimiter to COLOR (default '#525259').

**--fg-prefix** *COLOR*
: Set foreground prefix to COLOR (default '#AF9FC9').

**--fg-chord** *COLOR*
: Set foreground chord to COLOR (default '#DCD7BA').

**--fg-title** *COLOR*
: Set foreground title to COLOR (default '#DCD7BA').

**--fg-goto** *COLOR*
: Set foreground goto to COLOR (default '#E6C384').

**--title** *STRING*
: Set global title displayed above menu to STRING.

**--title-font** *STRING*
: Set title font to STRING. Should be a valid Pango font description
  (default 'sans-serif, 16').

**--bg** *COLOR*
: Set background to COLOR (default '#181616').

**--bd** *COLOR*
: Set border to COLOR (default '#7FB4CA').

**--shell** *STRING*
: Set shell to STRING (default '/bin/sh').

**--font** *STRING*
: Set font to STRING. Should be a valid Pango font description
  (default 'monospace, 14').

**--implicit-keys** *STRING*
: Set implicit keys to STRING (default 'asdfghjkl;').

## Bug Reports

If you find a bug in **wk**, please report it at https://github.com/3L0C/wk.

## See Also

[wks(5)](wks)

## Authors

3L0C \<dotbox at mailbox.org\>
