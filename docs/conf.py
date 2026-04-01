#!/usr/bin/env python3

import sys
import os
from pathlib import Path

sys.path.insert(0, os.path.abspath("_ext"))

# -- Project info -------------------------------------------------------------
project = "wk"
author = "3L0C"
copyright = "2024, 3L0C"

version_file = Path(__file__).resolve().parent.parent / "VERSION"
version = version_file.read_text().strip()
release = version

# -- Extensions ---------------------------------------------------------------
extensions = [
    "myst_parser",
    "sphinx_copybutton",
    "sphinx_prompt",
    "sphinx_design",
    "sphinx_inline_tabs",
]

myst_enable_extensions = [
    "colon_fence",  # ::: directive syntax
    "fieldlist",  # Field lists for options
    "deflist",  # Definition lists (term\n: definition)
    "substitution",  # {{variable}} in Markdown
    "attrs_inline",
]

myst_heading_anchors = 3
myst_substitutions = {
    "version": version,
    "project": project,
}

source_suffix = {".md": "markdown"}
exclude_patterns = ["_build", "DOCS_PLAN.md"]

# -- HTML output --------------------------------------------------------------
html_theme = "furo"
html_title = f"wk {version}"
html_static_path = ["_static"]
html_favicon = "_static/favicon.svg"
html_css_files = ["custom.css"]

html_theme_options = {
    "source_repository": "https://github.com/3L0C/wk",
    "source_branch": "master",
    "source_directory": "docs/",
    "light_css_variables": {
        "color-brand-primary": "#7FB4CA",
        "color-brand-content": "#7FB4CA",
    },
    "dark_css_variables": {
        "color-brand-primary": "#7FB4CA",
        "color-brand-content": "#7FB4CA",
    },
}

# -- Man page output ----------------------------------------------------------
man_pages = [
    ("reference/cli", "wk", "Which-Key via X11 and Wayland", ["3L0C"], 1),
    ("reference/wks", "wks", "Which-Key source file for wk", ["3L0C"], 5),
]
man_show_urls = False

# -- Pygments lexer -----------------------------------------------------------
from wks_lexer import WksLexer
from sphinx.highlighting import lexers

lexers["wks"] = WksLexer()
