#ifndef WK_COMPILER_COMPILER_H_
#define WK_COMPILER_COMPILER_H_

/* common includes */
#include "common/key_chord.h"
#include "common/menu.h"

/* local includes */
#include "token.h"
#include "scanner.h"

typedef struct
{
    Scanner scanner;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    KeyChord keyChord;
    ChordArray* keyChordDest;
    ChordArray* keyChordPrefix;
    ChordArray keyChords;
    char* source;
    bool sort;
} Compiler;

KeyChord* compileKeyChords(Compiler* compiler, Menu* menu);
void initCompiler(Compiler* compiler, char* source, const char* filepath);


#endif /* WK_COMPILER_COMPILER_H_ */
