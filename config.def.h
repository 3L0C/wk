#ifndef WK_CONFIG_H_
#define WK_CONFIG_H_

static const char* delimiter = "->"; /* Seperator when displaying binds */
static const unsigned int maxCols = 4; /* Max number of columns to use */
static const int windowWidth = -1; /* Window width. Set to '-1' for 1/2 the width of your screen. */
static const int windowGap = -1; /* Window gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */
static const unsigned int widthPadding = 2; /* X-Padding around key/description text in cells. */
static const unsigned int heightPadding = 2; /* Y-Padding around key/description text in cells. */
static const unsigned int windowPosition = 0; /* Position to place the window. '0' = bottom; '1' = top. */
static const unsigned int borderWidth = 2; /* Window border width */
static const char* foreground = "#DCD7BA"; /* Window foreground color */
static const char* background = "#181616"; /* Window background color */
static const char* border = "#7FB4CA"; /* Window border color */
static const char* shell = "/bin/sh"; /* Default shell to run chord commands with. */
static const char* font = "monospace, 10"; /* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */

#endif /* WK_CONFIG_H_ */
