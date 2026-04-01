# Scripting

wk can be used to prompt the user for a selection in shell
scripts. This is a task where tools like
[dmenu](https://tools.suckless.org/dmenu/),
[rofi](https://github.com/davatorium/rofi), and
[fzf](https://junegunn.github.io/fzf/) excel. wk works just
as well, if not better in some cases.

<video autoplay loop muted playsinline controls width="100%">
    <source src="../_static/demo-scripting-wk-2.webm" type="video/webm">
</video>

```bash
#!/usr/bin/env bash

wks_source='
:title "Do you like wk?"
:font "monospace, 50"
:title-font "sans-serif, 50"
:delay 0
y "Yes" +write %{{yes}}
n "No" +write %{{no}}
'

response="$(printf '%s\n' "$wks_source" | wk --script)"

echo "${response:-other}"
```

In this case, there are three possible responses: yes, no,
or some other keypress. In this example, the script prints
the response or "other" if the user pressed a key other than
`y` or `n`. It is perfectly reasonable to treat an unbound
keypress as a negative response, but it is good to be aware
of each possible response.

```{seealso}
Read about the `--script` flag [here](../reference/cli.md#options), and the `+write` flag [here](../reference/wks.md#output-control).
```

```{attention}
This guide does not explain bash scripting itself; only using wk in a bash script. If you are unfamiliar with any part of the examples please consult the bash [documentation](https://www.gnu.org/software/bash/manual/html_node/index.html).
```

## Why script with wk?

I've written a fair number of interactive shell scripts. I
don't always use wk for interactive prompts, but when I do
it is for one or more of the following reasons:

- I'll be selecting one of the first 10 options
- I don't need to search the results
- I want to make a selection with as few key strokes as
  possible

All three come down to one thing: wk lets you turn a short
list into a single-keystroke selection. Here are some
examples.

## Example 1

wk works just as well with more dynamic content:

<video autoplay loop muted playsinline controls width="100%">
    <source src="../_static/demo-scripting-wk-3.webm" type="video/webm">
</video>

```bash
#!/usr/bin/env bash

wk_keys=(1 2 3 4 5 6 7 8 9)
wk_settings=(
    ':font "monospace, 50"'
    ':title "Watch"'
    ':title-font "sans-serif, 50"'
    ':max-columns 1'
    ':menu-width 1800'
)
declare -a wk_options

readarray -t videos <"./watchlater.m3u"

if [[ "${#videos[@]}" -eq 0 ]]; then
    echo "No videos queued to watch later..." >&2
    exit 1
fi

for i in "${!wk_keys[@]}"; do
    if [[ "$i" -lt "${#videos[@]}" ]]; then
        wk_options+=(
            "${wk_keys[$i]} \"${videos[$i]##*/}\" +write %{{${videos[$i]}}}"
        )
    fi
done

selection="$(printf '%s\n' "${wk_settings[@]}" "${wk_options[@]}" | wk --script)"

if [[ -n "$selection" ]]; then
    # Remove the selection from the list
    perl -i -ne 'BEGIN { $s = shift } print unless $_ eq "$s\n"' "$selection" ./watchlater.m3u
    mpv "${selection}"
fi
```

There is a lot going on in the above script, but the main
idea is simple: build a list of videos for the user to
select from. In the intro, the list was fixed and we could
define the wks code as a fixed string. In this example, we
build the wks code into two arrays: `wk_settings` for
preprocessor commands and `wk_options` for the key chords.
Both are piped to `wk --script` the same way.

`wk_options` is built using a for loop which pairs each
video with a key from `wk_keys`. If there are more videos
than keys, only the first 9 are shown.

## Example 2

Example 1 is limited to 9 options, but using
[wk-prompt](https://github.com/3L0C/wk-prompt) we can get a
longer list of options:

<video autoplay loop muted playsinline controls width="100%">
    <source src="../_static/demo-scripting-wk-4.webm" type="video/webm">
</video>

```bash
#!/usr/bin/env bash

source wk-prompt # must be in your system's $PATH

wk_keys=(1 2 3 4 5 6 7 8 9)
wk_keywords=("+write") # write all commands
wk_prompt_title="Pick a number"
wk_preprocessor_cmds=(
    ':font "monospace, 50"'
    ':title-font "sans-serif, 50"'
    ':delay 0'
    ':max-columns 1'
)

wk_prompt {1..45}
```

With wk-prompt you can easily use wk for arbitrarily long
lists. The script takes care of the code generation, mapping
items to keys, pagination, etc. You only need to tweak the
configuration to your liking and supply the items to
`wk_prompt`.

```{note}
wk-prompt is a helper script. It should be sourced by other scripts which use the helper function `wk_prompt` on a list of inputs.
```

## Example 3

Let's revisit example 1, but this time using wk-prompt:

<video autoplay loop muted playsinline controls width="100%">
    <source src="../_static/demo-scripting-wk-5.webm" type="video/webm">
</video>

```bash
#!/usr/bin/env bash

source wk-prompt # must be in your system's $PATH

wk_preprocessor_cmds=(
    ':font "monospace, 50"'
    ':title-font "sans-serif, 50"'
    ':max-columns 1'
    ':menu-width 1800'
)
wk_prompt_title="Watch"
wk_keywords=('+write')
wk_keys=(1 2 3 4 5 6 7 8 9)

# Override `wk-prompt`'s description generator
build_wks_description() {
    _wks_desc="${1##*/}" # trim everything but the filename
}

readarray -t videos <"./watchlater.m3u"

selection="$(wk_prompt "${videos[@]}")"

if [[ -n "$selection" ]]; then
    # Remove the selection from the list
    perl -i -ne 'BEGIN { $s = shift } print unless $_ eq "$s\n"' "$selection" ./watchlater.m3u
    mpv "${selection}"
fi
```

The loop and manual wks generation from example 1 are gone.
wk-prompt handles key assignment, so the script only needs
to supply the video list and override the
`build_wks_description` function to customize how each item
is displayed. Both ways are valid. Play around with each to
see which works better for you.

## Conclusion

wk is a capable tool for interactive prompts. From simple to
complex scripts, there are many ways you can use wk in your
new or existing scripts. If you have any other examples,
suggestions, comments, or concerns, please open an
[issue](https://github.com/3L0C/wk/issues) or
[pr](https://github.com/3L0C/wk/pulls).
