#ifndef WK_CHORDS_H_
#define WK_CHORDS_H_

#include "lib/common.h"
#include "lib/types.h"

/* mods,    specials,  key,    description,   hint, */
/* command */
/* before */
/* after */
/* keep, unhook, nobefore, noafter, write, async, chords */
const Chord chords[] = CHORDS(
    {
        WK_MOD_NONE, WK_SPECIAL_NONE, "m", "Tag & follow", "m -> Tag & follow",
        NULL, 
        "echo first", 
        "echo last", 
        false, false, false, false, false, false, 
        PREFIX(
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, ":", "Tag 1", ": -> Tag 1",
                "echo \":\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_CTRL|WK_MOD_SHIFT, WK_SPECIAL_RETURN, "RET", "Tag 2", "C-S-RET -> Tag 2",
                "echo \"RET\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "a", "Tag 3", "a -> Tag 3",
                "echo \"a\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "テ", "Tag 4", "テ -> Tag 4",
                "echo \"テ\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "r", "Tag 5", "r -> Tag 5",
                "echo \"r\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Tag 6", "s -> Tag 6",
                "echo \"s\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "t", "Tag 7", "t -> Tag 7",
                "echo \"t\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "g", "Tag 8", "g -> Tag 8",
                "echo \"g\"", 
                NULL, 
                NULL, 
                true, false, false, false, false, true, NULL
            }
        )
    }
);

#endif /* WK_CHORDS_H_ */
