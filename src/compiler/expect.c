#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* local includes */
#include "expect.h"
#include "token.h"

typedef struct
{
    const char* name;
    Expectation flag;
} ExpectEntry;

static const ExpectEntry expectEntries[] = {
    { "modifier",      EXPECT_MOD      },
    { "key",           EXPECT_KEY      },
    { "description",   EXPECT_DESC     },
    { "hook",          EXPECT_HOOK     },
    { "flag",          EXPECT_FLAG     },
    { "command",       EXPECT_COMMAND  },
    { "@goto",         EXPECT_META     },
    { "'{'",           EXPECT_LBRACE   },
    { "'}'",           EXPECT_RBRACE   },
    { "'['",           EXPECT_LBRACKET },
    { "']'",           EXPECT_RBRACKET },
    { "'('",           EXPECT_LPAREN   },
    { "')'",           EXPECT_RPAREN   },
    { "'...'",         EXPECT_ELLIPSIS },
    { "interpolation", EXPECT_INTERP   },
    { "EOF",           EXPECT_EOF      },
};

#define EXPECT_ENTRY_COUNT (sizeof(expectEntries) / sizeof(expectEntries[0]))

const char*
expectationToString(Expectation e, char* buffer, size_t size)
{
    assert(buffer);

    if (e == EXPECT_NONE) return "nothing";

    buffer[0]       = '\0';
    size_t len      = 0;
    int    count    = 0;
    int    matching = 0;

    for (size_t i = 0; i < EXPECT_ENTRY_COUNT; i++)
    {
        if (e & expectEntries[i].flag)
        {
            matching++;
        }
    }

    for (size_t i = 0; i < EXPECT_ENTRY_COUNT; i++)
    {
        if (e & expectEntries[i].flag)
        {
            if (count > 0)
            {
                if (count == matching - 1)
                {
                    len += (size_t)snprintf(buffer + len, size - len, " or ");
                }
                else
                {
                    len += (size_t)snprintf(buffer + len, size - len, ", ");
                }
            }
            len += (size_t)snprintf(buffer + len, size - len, "%s", expectEntries[i].name);
            count++;

            if (len >= size - 1) break;
        }
    }

    if (count == 0)
    {
        snprintf(buffer, size, "unknown");
    }

    return buffer;
}

Expectation
tokenToExpectation(TokenType type)
{
    switch (type)
    {
    case TOKEN_MOD_CTRL: /* FALLTHROUGH */
    case TOKEN_MOD_META:
    case TOKEN_MOD_HYPER:
    case TOKEN_MOD_SHIFT: return EXPECT_MOD;

    case TOKEN_KEY: /* FALLTHROUGH */
    case TOKEN_SPECIAL_KEY: return EXPECT_KEY;

    case TOKEN_DESCRIPTION: /* FALLTHROUGH */
    case TOKEN_DESC_INTERP: return EXPECT_DESC;

    case TOKEN_BEFORE: /* FALLTHROUGH */
    case TOKEN_AFTER:
    case TOKEN_SYNC_BEFORE:
    case TOKEN_SYNC_AFTER: return EXPECT_HOOK;

    case TOKEN_KEEP: /* FALLTHROUGH */
    case TOKEN_CLOSE:
    case TOKEN_INHERIT:
    case TOKEN_IGNORE:
    case TOKEN_UNHOOK:
    case TOKEN_DEFLAG:
    case TOKEN_NO_BEFORE:
    case TOKEN_NO_AFTER:
    case TOKEN_WRITE:
    case TOKEN_EXECUTE:
    case TOKEN_SYNC_CMD:
    case TOKEN_WRAP:
    case TOKEN_UNWRAP:
    case TOKEN_TITLE: return EXPECT_FLAG;

    case TOKEN_COMMAND: /* FALLTHROUGH */
    case TOKEN_COMM_INTERP: return EXPECT_COMMAND;

    case TOKEN_GOTO: return EXPECT_META;

    case TOKEN_LEFT_BRACE: return EXPECT_LBRACE;
    case TOKEN_RIGHT_BRACE: return EXPECT_RBRACE;
    case TOKEN_LEFT_BRACKET: return EXPECT_LBRACKET;
    case TOKEN_RIGHT_BRACKET: return EXPECT_RBRACKET;
    case TOKEN_LEFT_PAREN: return EXPECT_LPAREN;
    case TOKEN_RIGHT_PAREN: return EXPECT_RPAREN;

    case TOKEN_ELLIPSIS: return EXPECT_ELLIPSIS;

    case TOKEN_THIS_KEY: /* FALLTHROUGH */
    case TOKEN_INDEX:
    case TOKEN_INDEX_ONE:
    case TOKEN_THIS_DESC:
    case TOKEN_THIS_DESC_UPPER_FIRST:
    case TOKEN_THIS_DESC_LOWER_FIRST:
    case TOKEN_THIS_DESC_UPPER_ALL:
    case TOKEN_THIS_DESC_LOWER_ALL:
    case TOKEN_USER_VAR:
    case TOKEN_WRAP_CMD_INTERP: return EXPECT_INTERP;

    case TOKEN_EOF: return EXPECT_EOF;

    default: return EXPECT_NONE;
    }
}
