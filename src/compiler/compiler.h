#ifndef WK_COMPILER_COMPILER_H_
#define WK_COMPILER_COMPILER_H_

/* common includes */
#include "common/key_chord.h"
#include "common/menu.h"

/* local includes */
#include "token.h"
#include "scanner.h"

typedef struct PseudoChord PseudoChord;

typedef struct
{
    PseudoChord* chords;
    size_t count;
    size_t capacity;
} PseudoChordArray;

struct PseudoChord
{
    KeyChordState state;
    Key key;
    TokenArray description;
    TokenArray command;
    TokenArray before;
    TokenArray after;
    ChordFlags flags;
    PseudoChordArray chords;
};

typedef struct
{
    Scanner scanner;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    PseudoChord chord;
    PseudoChordArray* chordsDest;
    PseudoChordArray chords;
    char* source;
    bool sort;
} Compiler;

KeyChord* compileKeyChords(Compiler* compiler, Menu* menu);
void initCompiler(Compiler* compiler, char* source, const char* filepath);


#endif /* WK_COMPILER_COMPILER_H_ */
