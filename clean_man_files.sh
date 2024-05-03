#!/usr/bin/env sh

file_type="${1:?missing file type}"
file_name="${2:?missing file name}"

# Generate the new date string
new_date=$(date '+%B %d, %Y')

fix_header="s/^\.TH \(\"[^\"]*\"\) \(\"[^\"]*\"\).*$/.TH \1 \2 \"$new_date\" \"\" \"$file_type\"/"
remove_zero_width_space="s/\xe2\x80\x8b//g"
fix_bold_caret="s/\*\^\*\^/\\\\f[B]^\\\\f[R]/"
fix_interp_caret="s/\*desc\\\\\[ha]\^\*\^/\\\\f[B]desc^^\\\\f[R]/;s/\*desc\^\*\^/\\\\f[B]desc^\\\\f[R]/"
remove_surrounding_tilde="s/\~\([^\~ ]\+\)\~/_\1/g"

# Update man heading with the new date and file type and remove zero-width-spaces
sed -i \
    -e "$fix_header" \
    -e "$remove_zero_width_space" \
    -e "$fix_bold_caret" \
    -e "$fix_interp_caret" \
    -e "$remove_surrounding_tilde" \
    "$file_name"
