#ifndef WK_CONFIG_CONFIG_H_
#define WK_CONFIG_CONFIG_H_

#include <stdint.h>

/* menu include */
#include "src/common/menu.h"

/* Delimiter when displaying chords. */
static const char* delimiter = " -> ";
/* Delay between last keypress and first time displaying the menu. Value in milliseconds. */
static uint32_t delay = 1000;
/* Max number of columns to use. */
static const uint32_t maxCols = 5;
/* Window width. Set to '-1' for 1/2 the width of your screen. */
static const int32_t windowWidth = -1;
/* Window gap between top/bottom of screen. Set to '-1' for a gap of 1/10th of the screen height. */
static const int32_t windowGap = -1;
/* X-Padding around key/description text in cells. */
static const uint32_t widthPadding = 6;
/* Y-Padding around key/description text in cells. */
static const uint32_t heightPadding = 2;
/* Position to place the window. '0' = bottom; '1' = top. */
static const uint32_t windowPosition = 0;
/* Window border width */
static const uint32_t borderWidth = 4;
/* Window border radius. 0 means no curve */
static const double borderRadius = 0;
/* Window foreground color */
static const char* foreground[FOREGROUND_COLOR_LAST] = {
    "#DCD7BA", /* Key color */
    "#525259", /* Delimiter color */
    "#AF9FC9", /* Prefix color */
    "#DCD7BA", /* Chord color */
};
/* Window background color */
static const char* background = "#181616";
/* Window border color */
static const char* border = "#7FB4CA";
/* Default shell to run chord commands with. */
static const char* shell = "/bin/sh";
/* Pango font description i.e. 'Noto Mono, M+ 1c, ..., 16'. */
static const char* font = "monospace, 14";

#endif /* WK_CONFIG_CONFIG_H_ */
