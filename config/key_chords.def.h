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
 * command
 * before
 * after
 * flags, chords
 */
KeyChord builtinKeyChords[] = {
    {
        .state = KEY_CHORD_STATE_NOT_NULL, 
        .key = {
            .mods = {
                .ctrl = false, .alt = false, .hyper = false, .shift = false
            },
            .special = SPECIAL_KEY_NONE,
            .repr = "a", .len = 1
        },
        .description = "A chord", 
        .command = "echo \"Hello, world!\"", 
        .before = NULL, 
        .after = NULL, 
        .flags = {
            false, false, false, false, false, false, false,
            false, false, false, false, false, false, false
        }, .keyChords = NULL
    },
    {
        .state = KEY_CHORD_STATE_NOT_NULL, 
        .key = {
            .mods = {
                .ctrl = false, .alt = false, .hyper = false, .shift = false
            },
            .special = SPECIAL_KEY_NONE,
            .repr = "p", .len = 1
        },
        .description = "A prefix", 
        .command = NULL, 
        .before = NULL, 
        .after = NULL, 
        .flags = {
            false, false, false, false, false, false, false,
            false, false, false, false, false, false, false
        }, 
        .keyChords = (KeyChord[]){
            {
                .state = KEY_CHORD_STATE_NOT_NULL, 
                .key = {
                    .mods = {
                        .ctrl = false, .alt = false, .hyper = false, .shift = false
                    },
                    .special = SPECIAL_KEY_NONE,
                    .repr = "b", .len = 1
                },
                .description = "A chord", 
                .command = "echo \"Hello from inside prefix 'C-a'\"", 
                .before = NULL, 
                .after = NULL, 
                .flags = {
                    false, false, false, false, false, false, false,
                    false, false, false, false, false, false, false
                }, .keyChords = NULL
            },
            {
                .state = KEY_CHORD_STATE_NOT_NULL, 
                .key = {
                    .mods = {
                        .ctrl = false, .alt = false, .hyper = false, .shift = false
                    },
                    .special = SPECIAL_KEY_NONE,
                    .repr = "c", .len = 1
                },
                .description = "Another prefix", 
                .command = NULL, 
                .before = NULL, 
                .after = NULL, 
                .flags = {
                    false, false, false, false, false, false, false,
                    false, false, false, false, false, false, false
                }, 
                .keyChords = (KeyChord[]){
                    {
                        .state = KEY_CHORD_STATE_NOT_NULL, 
                        .key = {
                            .mods = {
                                .ctrl = false, .alt = false, .hyper = false, .shift = false
                            },
                            .special = SPECIAL_KEY_NONE,
                            .repr = "d", .len = 1
                        },
                        .description = "Done", 
                        .command = "echo \"You've reached the end!\"", 
                        .before = NULL, 
                        .after = NULL, 
                        .flags = {
                            false, false, false, false, false, false, false,
                            false, false, false, false, false, false, false
                        }, .keyChords = NULL
                    },
                    { .state = KEY_CHORD_STATE_IS_NULL }
                }
            },
            { .state = KEY_CHORD_STATE_IS_NULL }
        }
    },
    { .state = KEY_CHORD_STATE_IS_NULL }
};

#endif /* WK_CONFIG_KEY_CHORDS_H_ */
