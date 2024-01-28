#ifndef WK_LINE_H_
#define WK_LINE_H_

#include "lib/common.h"

#include "scanner.h"
#include "token.h"

typedef struct LineArray LineArray;
typedef struct Line Line;

struct LineArray
{
    Line*  lines;
    size_t count;
    size_t capacity;
};

struct Line
{
    int         index;
    int         mods;
    Token       key;
    TokenArray  description;
    TokenArray  command;
    TokenArray  before;
    TokenArray  after;
    bool        keep;
    bool        unhook;
    bool        nobefore;
    bool        noafter;
    bool        write;
    bool        async;
    LineArray   array;
};

void initLine(Line* line);
void initLineArray(LineArray* array);
void copyLine(Line* from, Line* to);
void writeLineArray(LineArray* array, Line* line);
void freeLine(Line* line);
void freeLineArray(LineArray* array);
void printLineArray(LineArray* array);

#endif /* WK_LINE_H_ */
