#ifndef WK_CONFIG_KEY_CHORDS_H_
#define WK_CONFIG_KEY_CHORDS_H_

#include <stddef.h>

#include "src/common/key_chord.h"

/* state,
 * KEY(
 *     mods,
 *     special,
 *     key, key_len
 * ),
 * description,
 * hint,
 * command
 * before
 * after
 * flags, chords
 */
KeyChord builtinKeyChords[] = KEY_CHORDS(
    {
        KEY_CHORD_STATE_NOT_NULL, 
        MAKE_KEY(
            false, false, false, false,
            SPECIAL_KEY_NONE,
            "a", 1
        ),
        "A chord", "a -> A chord", 
        "echo \"Hello, world!\"", 
        NULL, 
        NULL, 
        MAKE_FLAGS(
            false, false, false, false, false, false, false,
            false, false, false, false, false, false
        ), NULL
    },
    {
        KEY_CHORD_STATE_NOT_NULL, 
        MAKE_KEY(
            true , false, false, false,
            SPECIAL_KEY_NONE,
            "a", 1
        ),
        "A prefix", "C-a -> A prefix", 
        NULL, 
        NULL, 
        NULL, 
        MAKE_FLAGS(
            false, false, false, false, false, false, false,
            false, false, false, false, false, false
        ), 
        PREFIX(
            {
                KEY_CHORD_STATE_NOT_NULL, 
                MAKE_KEY(
                    false, false, false, false,
                    SPECIAL_KEY_NONE,
                    "b", 1
                ),
                "A chord", "b -> A chord", 
                "echo \"Hello from inside prefix 'C-a'\"", 
                NULL, 
                NULL, 
                MAKE_FLAGS(
                    false, false, false, false, false, false, false,
                    false, false, false, false, false, false
                ), NULL
            },
            {
                KEY_CHORD_STATE_NOT_NULL, 
                MAKE_KEY(
                    false, false, false, false,
                    SPECIAL_KEY_NONE,
                    "c", 1
                ),
                "Another prefix", "c -> Another prefix", 
                NULL, 
                NULL, 
                NULL, 
                MAKE_FLAGS(
                    false, false, false, false, false, false, false,
                    false, false, false, false, false, false
                ), 
                PREFIX(
                    {
                        KEY_CHORD_STATE_NOT_NULL, 
                        MAKE_KEY(
                            false, false, false, false,
                            SPECIAL_KEY_NONE,
                            "d", 1
                        ),
                        "Done", "d -> Done", 
                        "echo \"You've reached the end!\"", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(
                            false, false, false, false, false, false, false,
                            false, false, false, false, false, false
                        ), NULL
                    }
                )
            }
        )
    }
);

#endif /* WK_CONFIG_KEY_CHORDS_H_ */
