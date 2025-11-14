#!/usr/bin/env sh

file_type="${1:?missing file type}"
file_name="${2:?missing file name}"

# Generate the new date string
new_date=$(date '+%B %d, %Y')

fix_header="s/^\.TH \(\"[^\"]*\"\) \(\"[^\"]*\"\).*$/.TH \1 \2 \"$new_date\" \"\" \"$file_type\"/"

# I write the man files in org and have to insert a zero-width-space
# to get the exports to work correctly. They aren't an issue in the final
# man pages, but they aren't needed either. Feel free to comment this line out.
remove_zero_width_space="s/\xe2\x80\x8b//g"

# Caret/circumflex fixes - handle various patterns from org export
fix_standalone_caret="s/\*\^\*\^/\\\\f[B]^\\\\f[R]/g"
fix_bold_caret="s/\*\(\^\+\)\*\(\1\)/\\\\f[B]\1\\\\f[R]\2/g"
fix_interp_caret="s/\*desc\\\\\[ha]\^\*\^/\\\\f[B]desc^^\\\\f[R]/g;s/\*desc\^\*\^/\\\\f[B]desc^\\\\f[R]/g"

# Tilde fixes - surrounding tildes and underscores rendered as tildes in identifiers
remove_surrounding_tilde="s/\~\([^\~ ]\+\)\~/_\1/g"
fix_tilde_in_quotes="s/\\\\(aq\([^(]*\)\~\([^~(]*\)\~/\\\\(aq\1_\2_/g"

# Long option fixes - convert en-dash to double-minus for command-line options
fix_long_options="s/, \\\\\\[en\\]/, \\\\-\\\\-/g"

# Backtick fixes - convert pandoc's grave accent escapes back to backticks
fix_backticks="s/\\\\(ga/\`/g"

# Apply all fixes in order
sed -i \
    -e "$fix_header" \
    -e "$remove_zero_width_space" \
    -e "$fix_standalone_caret" \
    -e "$fix_bold_caret" \
    -e "$fix_interp_caret" \
    -e "$remove_surrounding_tilde" \
    -e "$fix_tilde_in_quotes" \
    -e "$fix_long_options" \
    -e "$fix_backticks" \
    "$file_name"
