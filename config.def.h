#ifndef WK_CONFIG_H_
#define WK_CONFIG_H_

static const char* delimiter = "->"; /* Seperator when displaying binds */
static const unsigned int maxCols = 4; /* Max number of columns to use */
static const int windowWidth = -1; /* Window width. Set to '-1' for 1/2 the width of your screen. */
static const int windowHeight = -1; /* Window height. Set to '-1' to place window 1/10th of the way up the screen from the bottom. */
static const unsigned int windowPosition = 0; /* Position to place the window. '0' = bottom; '1' = top. */
static const unsigned int borderWidth = 2; /* Window border width */
static const char* foreground = "#DCD7BA"; /* Window foreground color */
static const char* background = "#181616"; /* Window background color */
static const char* border = "#7FB4CA"; /* Window border color */
static const char* shell = "/bin/sh";
/* Fonts to use. Max supported is 8 */
static const char* fonts[] = {
    "monospace 10"
};

#endif /* WK_CONFIG_H_ */
