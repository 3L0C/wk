#ifndef WK_CHORDS_H_
#define WK_CHORDS_H_

#include "lib/common.h"
#include "lib/types.h"

/* mods,    specials,  key,    description,   hint, */
/* command */
/* before */
/* after */
/* flags, chords */
const Chord chords[] = CHORDS(
    {
        WK_MOD_NONE, WK_SPECIAL_NONE, "e", "Echo keys", "e -> Echo keys",
        NULL, 
        "echo first", 
        "echo last", 
        WK_FLAG_DEFAULTS, 
        PREFIX(
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, ":", "Tag 1", ": -> Tag 1",
                "echo \":\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_CTRL|WK_MOD_SHIFT, WK_SPECIAL_RETURN, "RET", "Tag 2", "C-S-RET -> Tag 2",
                "echo \"RET\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "a", "Tag 3", "a -> Tag 3",
                "echo \"a\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "テ", "Tag 4", "テ -> Tag 4",
                "echo \"テ\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "r", "Tag 5", "r -> Tag 5",
                "echo \"r\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Tag 6", "s -> Tag 6",
                "echo \"s\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "t", "Tag 7", "t -> Tag 7",
                "echo \"t\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "g", "Tag 8", "g -> Tag 8",
                "echo \"g\"", 
                "echo first", 
                "echo last", 
                WK_FLAG_DEFAULTS, NULL
            }
        )
    },
    {
        WK_MOD_NONE, WK_SPECIAL_NONE, "t", "Tag & follow", "t -> Tag & follow",
        NULL, 
        "dwmc viewex 1", 
        NULL, 
        WK_FLAG_DEFAULTS, 
        PREFIX(
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "a", "Tag 1", "a -> Tag 1",
                "notify-send \"dwmc\" \"Viewing tag 1\"", 
                "dwmc viewex 0", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "r", "Tag 2", "r -> Tag 2",
                "notify-send \"dwmc\" \"Viewing tag 2\"", 
                "dwmc viewex 1", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Tag 3", "s -> Tag 3",
                "notify-send \"dwmc\" \"Viewing tag 3\"", 
                "dwmc viewex 2", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "t", "Tag 4", "t -> Tag 4",
                "notify-send \"dwmc\" \"Viewing tag 4\"", 
                "dwmc viewex 3", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "n", "Tag 5", "n -> Tag 5",
                "notify-send \"dwmc\" \"Viewing tag 5\"", 
                "dwmc viewex 4", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "e", "Tag 6", "e -> Tag 6",
                "notify-send \"dwmc\" \"Viewing tag 6\"", 
                "dwmc viewex 5", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "i", "Tag 7", "i -> Tag 7",
                "notify-send \"dwmc\" \"Viewing tag 7\"", 
                "dwmc viewex 6", 
                NULL, 
                WK_FLAG_KEEP, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "o", "Tag 8", "o -> Tag 8",
                "notify-send \"dwmc\" \"Viewing tag 8\"", 
                "dwmc viewex 7", 
                NULL, 
                WK_FLAG_KEEP, NULL
            }
        )
    }
);

#endif /* WK_CHORDS_H_ */
