#ifndef WK_COMPILER_COMPILER_H_
#define WK_COMPILER_COMPILER_H_

#include <stdint.h>

/* common includes */
#include "common/key_chord.h"

/* local includes */
#include "common/menu.h"
#include "token.h"
#include "scanner.h"

typedef struct
{
    Scanner scanner;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    uint32_t index;
    KeyChord keyChord;
    KeyChordArray* keyChordDest;
    KeyChordArray* keyChordPrefix;
    KeyChordArray keyChords;
    char* source;
} Compiler;

KeyChord* compileKeyChords(Compiler* compiler, Menu* menu);
void initCompiler(Compiler* compiler, char* source, const char* filepath);


#endif /* WK_COMPILER_COMPILER_H_ */
