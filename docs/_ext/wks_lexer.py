#!/usr/bin/env python3

from pygments.lexer import RegexLexer, bygroups, words
from pygments.token import (
    Comment,
    String,
    Keyword,
    Name,
    Operator,
    Punctuation,
    Text,
    Literal,
)


class WksLexer(RegexLexer):
    name = "wks"
    aliases = ["wks"]
    filenames = ["*.wks"]

    tokens = {
        "root": [
            (r"#.*$", Comment.Single),
            # Preprocessor: :include, :fg-color, :var, :sort, etc.
            (r":[a-z][-a-z]*", Keyword.Pseudo),
            # Hooks: ^before, ^after, ^sync-before, ^sync-after
            (
                r"(\^)(before|after|sync-before|sync-after)\b",
                bygroups(Operator, Name.Decorator),
            ),
            # Flags: +keep, +close, +inherit, +wrap, +args, +title, etc.
            (
                r"(\+)(keep|close|inherit|ignore|unhook|deflag|no-before|"
                r"no-after|write|execute|sync-command|unwrap|wrap|title|args)\b",
                bygroups(Operator, Name.Builtin),
            ),
            # Meta commands: @goto
            (r"(@)(goto)\b", bygroups(Operator, Name.Builtin)),
            # Special keys
            (
                words(
                    (
                        "Left",
                        "Right",
                        "Up",
                        "Down",
                        "BS",
                        "TAB",
                        "SPC",
                        "RET",
                        "DEL",
                        "ESC",
                        "Home",
                        "PgUp",
                        "PgDown",
                        "End",
                        "Begin",
                        "VolDown",
                        "VolMute",
                        "VolUp",
                        "Play",
                        "Stop",
                        "Prev",
                        "Next",
                    ),
                    suffix=r"(?=\s)",
                ),
                Name.Constant,
            ),
            # Function keys F1-F35
            (r"\bF[1-9]\b|\bF[12][0-9]\b|\bF3[0-5]\b", Name.Constant),
            # Modifiers: C-, M-, H-, S-
            (r"[CMHS]-", Name.Label),
            # Implicit array ellipsis
            (r"\.\.\.", Operator),
            # Commands: %{{ ... }}, %(( ... )), %[[ ... ]]
            (r"%\{\{", String.Backtick, "cmd_brace"),
            (r"%\(\(", String.Backtick, "cmd_paren"),
            (r"%\[\[", String.Backtick, "cmd_bracket"),
            # Descriptions
            (r'"', String.Double, "string"),
            # Structural punctuation
            (r"[\[\]<>{}()]", Punctuation),
            (r"\\.", Literal),
            (r"\s+", Text.Whitespace),
            (r"\S", Text),
        ],
        "string": [
            (r'\\"', String.Escape),
            (r"%\([^)]+\)", String.Interpol),
            (r'[^"\\%]+', String.Double),
            (r"%(?!\()", String.Double),
            (r'"', String.Double, "#pop"),
        ],
        "cmd_brace": [
            (r"%\([^)]+\)", String.Interpol),
            (r"\}\}", String.Backtick, "#pop"),
            (r"[^}%]+", Text),
            (r"%(?!\()", Text),
            (r"\}", Text),
        ],
        "cmd_paren": [
            (r"%\([^)]+\)", String.Interpol),
            (r"\)\)", String.Backtick, "#pop"),
            (r"[^)%]+", Text),
            (r"%(?!\()", Text),
            (r"\)", Text),
        ],
        "cmd_bracket": [
            (r"%\([^)]+\)", String.Interpol),
            (r"\]\]", String.Backtick, "#pop"),
            (r"[^\]%]+", Text),
            (r"%(?!\()", Text),
            (r"\]", Text),
        ],
    }
