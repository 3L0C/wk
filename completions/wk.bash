# Bash completion for wk (Which-Key)
# shellcheck shell=bash

_wk() {
    local cur prev words cword
    _init_completion || return

    # Options that take file arguments (.wks files)
    local file_opts='-T --transpile -k --key-chords'

    # Options that take integer arguments
    local int_opts='-D --delay -m --max-columns -w --menu-width -g --menu-gap
                    --keep-delay --border-width --wpadding --hpadding --table-padding'

    # Options that take number arguments (float)
    local num_opts='--border-radius'

    # Options that take color arguments
    local color_opts='--fg --fg-key --fg-delimiter --fg-prefix --fg-chord
                      --fg-title --bg --bd'

    # Options that take string arguments
    local str_opts='-p --press --shell --font --title-font --title
                    --implicit-keys --wrap-cmd'

    # All options
    local all_opts='-h --help -v --version -d --debug -t --top -b --bottom
                    -s --script -U --unsorted
                    -D --delay -m --max-columns -p --press -T --transpile
                    -k --key-chords -w --menu-width -g --menu-gap
                    --keep-delay --border-width --border-radius
                    --wpadding --hpadding --table-padding
                    --fg --fg-key --fg-delimiter --fg-prefix --fg-chord
                    --fg-title --bg --bd
                    --shell --font --title-font --title --implicit-keys --wrap-cmd'

    case $prev in
        -T|--transpile|-k|--key-chords)
            # Complete .wks files
            _filedir wks
            return
            ;;
        --shell)
            # Complete executable paths
            _filedir
            return
            ;;
        -D|--delay|-m|--max-columns|-w|--menu-width|-g|--menu-gap|\
        --keep-delay|--border-width|--wpadding|--hpadding|--table-padding|\
        --border-radius)
            # Numeric arguments - no completion
            return
            ;;
        --fg|--fg-key|--fg-delimiter|--fg-prefix|--fg-chord|--fg-title|--bg|--bd)
            # Color arguments - no completion (user enters hex color)
            return
            ;;
        -p|--press|--font|--title-font|--title|--implicit-keys|--wrap-cmd)
            # String arguments - no completion
            return
            ;;
    esac

    if [[ $cur == -* ]]; then
        COMPREPLY=($(compgen -W "$all_opts" -- "$cur"))
        return
    fi

    # Default: complete .wks files as positional arguments are not used
    _filedir wks
}

complete -F _wk wk
