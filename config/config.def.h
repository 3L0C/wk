#ifndef WK_CONFIG_H_
#define WK_CONFIG_H_

/* Seperator when displaying chords. */
static const char* delimiter = " -> ";
/* Max number of columns to use. */
static const unsigned int maxCols = 4;
/* Window width. Set to '-1' for 1/2 the width of your screen. */
static const int windowWidth = -1;
/* Window gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */
static const int windowGap = -1;
/* X-Padding around key/description text in cells. */
static const unsigned int widthPadding = 6;
/* Y-Padding around key/description text in cells. */
static const unsigned int heightPadding = 2;
/* Position to place the window. '0' = bottom; '1' = top. */
static const unsigned int windowPosition = 0;
/* Window border width */
static const unsigned int borderWidth = 4;
/* Window border radius. 0 means no curve */
static const double borderRadius = 0;
/* Window foreground color */
static const char* foreground = "#DCD7BA";
/* Window background color */
static const char* background = "#181616";
/* Window border color */
static const char* border = "#7FB4CA";
/* Default shell to run chord commands with. */
static const char* shell = "/bin/sh";
/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */
static const char* font = "monospace, 14";

#endif /* WK_CONFIG_H_ */
