#ifndef WK_CHORDS_H_
#define WK_CHORDS_H_

#include "lib/common.h"
#include "lib/types.h"

/* mods, specials,
 * key, description, hint,
 * command
 * before
 * after
 * flags, chords
 */
const Chord chords[] = CHORDS(
    {
        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
        "e", "Echo keys", "e -> Echo keys",
        NULL, 
        "echo first", 
        "echo last", 
        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), 
        PREFIX(
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "a", "A prefix!", "a -> A prefix!",
                NULL, 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, true, false, false, false, false, false, false, false), 
                PREFIX(
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "a", "a chord!", "a -> a chord!",
                        "echo a", 
                        "echo first", 
                        "echo last", 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "b", "b chord!", "b -> b chord!",
                        "echo b", 
                        "echo first", 
                        "echo last", 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "c", "c chord!", "c -> c chord!",
                        "echo c", 
                        "echo first", 
                        "echo last", 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "d", "d chord!", "d -> d chord!",
                        "echo d", 
                        "echo first", 
                        "echo last", 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    }
                )
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                ":", "Tag 1", ": -> Tag 1",
                "echo \":\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(true, false, false, true), WK_SPECIAL_RETURN, 
                "RET", "Tag 2", "C-S-RET -> Tag 2",
                "echo \"RET\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "テ", "Tag 3", "テ -> Tag 3",
                "echo \"テ\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "r", "Tag 4", "r -> Tag 4",
                "echo \"r\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "s", "Tag 5", "s -> Tag 5",
                "echo \"s\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "t", "Tag 6", "t -> Tag 6",
                "echo \"t\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "g", "Tag 7", "g -> Tag 7",
                "echo \"g\"", 
                "echo first", 
                "echo last", 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            }
        )
    },
    {
        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
        "t", "Tag & follow", "t -> Tag & follow",
        NULL, 
        "dwmc viewex 2", 
        NULL, 
        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), 
        PREFIX(
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "a", "Tag 1", "a -> Tag 1",
                "notify-send \"dwmc\" \"Viewing tag 1\"", 
                "dwmc viewex 0", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "r", "Tag 2", "r -> Tag 2",
                "notify-send \"dwmc\" \"Viewing tag 2\"", 
                "dwmc viewex 1", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "s", "Tag 3", "s -> Tag 3",
                "notify-send \"dwmc\" \"Viewing tag 3\"", 
                "dwmc viewex 2", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "t", "Tag 4", "t -> Tag 4",
                "notify-send \"dwmc\" \"Viewing tag 4\"", 
                "dwmc viewex 3", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "n", "Tag 5", "n -> Tag 5",
                "notify-send \"dwmc\" \"Viewing tag 5\"", 
                "dwmc viewex 4", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "e", "Tag 6", "e -> Tag 6",
                "notify-send \"dwmc\" \"Viewing tag 6\"", 
                "dwmc viewex 5", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "i", "Tag 7", "i -> Tag 7",
                "notify-send \"dwmc\" \"Viewing tag 7\"", 
                "dwmc viewex 6", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "o", "Tag 8", "o -> Tag 8",
                "notify-send \"dwmc\" \"Viewing tag 8\"", 
                "dwmc viewex 7", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            }
        )
    }
);

#endif /* WK_CHORDS_H_ */
