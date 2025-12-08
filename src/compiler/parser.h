#ifndef WK_COMPILER_PARSER_H_
#define WK_COMPILER_PARSER_H_

#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/arena.h"
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/vector.h"

/* local includes */
#include "expect.h"
#include "scanner.h"
#include "token.h"

typedef struct Parser Parser;

Vector parse(Scanner* scanner, Menu* m);

Token*      parserCurrentToken(Parser* p);
Token*      parserPreviousToken(Parser* p);
TokenType   parserAdvance(Parser* p);
bool        parserCheck(Parser* p, TokenType type);
bool        parserIsAtEnd(Parser* p);
KeyChord*   parserCurrentChord(Parser* p);
void        parserSetChord(Parser* p, KeyChord* chord);
KeyChord*   parserAllocChord(Parser* p);
void        parserFinishChord(Parser* p);
Vector*     parserDest(Parser* p);
void        parserSetDest(Parser* p, Vector* dest);
Arena*      parserArena(Parser* p);
Vector*     parserUserVars(Parser* p);
Vector*     parserImplicitKeys(Parser* p);
Menu*       parserMenu(Parser* p);
Scanner*    parserScanner(Parser* p);
bool        parserHadError(Parser* p);
void        parserSetError(Parser* p);
bool        parserInPanicMode(Parser* p);
void        parserSetPanicMode(Parser* p, bool mode);
size_t      parserDepth(Parser* p);
void        parserPushState(Parser* p);
bool        parserPopState(Parser* p);
Vector*     parserSavedDest(Parser* p, size_t depth);
KeyChord*   parserSavedChord(Parser* p, size_t depth);
Vector*     parserChildVector(Parser* p, size_t depth);
void        parserErrorAt(Parser* p, Token* token, const char* fmt, ...);
void        parserErrorAtCurrent(Parser* p, const char* fmt, ...);
void        parserErrorUnexpected(Parser* p, Expectation expected, Expectation got);
Expectation parserNextChordExpectation(Parser* p);

#endif /* WK_COMPILER_PARSER_H_ */
