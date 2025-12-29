#ifndef WK_COMPILER_PARSER_H_
#define WK_COMPILER_PARSER_H_

#include <stdbool.h>
#include <stddef.h>

/* common includes */
#include "common/arena.h"
#include "common/key_chord.h"
#include "common/menu.h"
#include "common/stack.h"
#include "common/vector.h"

/* local includes */
#include "expect.h"
#include "scanner.h"
#include "token.h"

typedef struct Parser Parser;

Vector parse(Scanner* scanner, Menu* m);

TokenType   parserAdvance(Parser* p);
KeyChord*   parserAllocChord(Parser* p);
Arena*      parserArena(Parser* p);
Stack*      parserArgEnvStack(Parser* p);
bool        parserCheck(Parser* p, TokenType type);
Vector*     parserChildVector(Parser* p, size_t depth);
bool        parserChordPushedEnv(Parser* p);
KeyChord*   parserCurrentChord(Parser* p);
Token*      parserCurrentToken(Parser* p);
bool        parserDebug(Parser* p);
void        parserDebugAt(Parser* p, Token* token, const char* fmt, ...);
size_t      parserDepth(Parser* p);
Vector*     parserDest(Parser* p);
void        parserErrorAt(Parser* p, Token* token, const char* fmt, ...);
void        parserErrorAtCurrent(Parser* p, const char* fmt, ...);
void        parserErrorUnexpected(Parser* p, Expectation expected, Expectation got);
void        parserFinishChord(Parser* p);
bool        parserHadError(Parser* p);
Vector*     parserImplicitKeys(Parser* p);
bool        parserInPanicMode(Parser* p);
bool        parserInTemplateContext(Parser* p);
bool        parserIsAtEnd(Parser* p);
Menu*       parserMenu(Parser* p);
Expectation parserNextChordExpectation(Parser* p);
bool        parserPopState(Parser* p);
Token*      parserPreviousToken(Parser* p);
bool        parserPushedEnvAtDepth(Parser* p, size_t depth);
void        parserPushState(Parser* p);
KeyChord*   parserSavedChord(Parser* p, size_t depth);
Vector*     parserSavedDest(Parser* p, size_t depth);
Scanner*    parserScanner(Parser* p);
void        parserSetChord(Parser* p, KeyChord* chord);
void        parserSetChordPushedEnv(Parser* p, bool pushed);
void        parserSetDest(Parser* p, Vector* dest);
void        parserSetError(Parser* p);
void        parserSetInTemplateContext(Parser* p, bool inTemplate);
void        parserSetPanicMode(Parser* p, bool mode);
void        parserSetPushedEnvAtDepth(Parser* p, size_t depth, bool pushed);
Vector*     parserUserVars(Parser* p);
void        parserWarnAt(Parser* p, Token* token, const char* fmt, ...);

#endif /* WK_COMPILER_PARSER_H_ */
