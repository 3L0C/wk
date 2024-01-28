#ifndef WK_CHORDS_H_
#define WK_CHORDS_H_

#include "lib/common.h"
#include "lib/types.h"

/*                    mods,        special,         key,  desc, hint, command,  before, after, keep,  unhook, nobefore, noafter, write, chords */
#define NULL_CHORD  { WK_MOD_NONE, WK_SPECIAL_NONE, NULL, NULL, NULL, NULL,     NULL,   NULL,  false, false,  false,    false,   false, NULL }
#define PREFIX(...) (Chord[]){ __VA_ARGS__, NULL_CHORD }
#define CHORDS(...) { __VA_ARGS__, NULL_CHORD }

/* mods,    specials,  key,    description,   hint, */
/* command, clen */
/* before, blen */
/* after, alen */
/* keep,    unhook,    nobefore,    noafter,    write,    chords */
const Chord chords[] = CHORDS(
    {
        WK_MOD_NONE, WK_SPECIAL_NONE, "m", "Tag & follow", "m -> Tag & follow",
        NULL, 
        "echo first", 
        "echo last", 
        false, false, false, false, false, 
        PREFIX(
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, ":", "Tag 1", ": -> Tag 1",
                "echo \":\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_CTRL|WK_MOD_SHIFT, WK_SPECIAL_RETURN, "RET", "Tag 2", "C-S-RET -> Tag 2",
                "echo \"RET\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "a", "Tag 3", "a -> Tag 3",
                "echo \"a\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "テ", "Tag 4", "テ -> Tag 4",
                "echo \"テ\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "r", "Tag 5", "r -> Tag 5",
                "echo \"r\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Tag 6", "s -> Tag 6",
                "echo \"s\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "t", "Tag 7", "t -> Tag 7",
                "echo \"t\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "g", "Tag 8", "g -> Tag 8",
                "echo \"g\"", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            }
        )
    },
    {
        WK_MOD_NONE, WK_SPECIAL_HOME, "Home", "Browser", "Home -> Browser",
        NULL, 
        NULL, 
        NULL, 
        false, false, false, false, false, 
        PREFIX(
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "b", "Open Brave", "b -> Open Brave",
                "brave", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "f", "Open Firefox", "f -> Open Firefox",
                "firefox", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "l", "Open Librewolf", "l -> Open Librewolf",
                "librewolf", 
                NULL, 
                NULL, 
                false, false, false, false, false, NULL
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "m", "Mullvad-exclude", "m -> Mullvad-exclude",
                NULL, 
                NULL, 
                NULL, 
                false, false, false, false, false, 
                PREFIX(
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "b", "Exclude Brave", "b -> Exclude Brave",
                        "mullvad-exclude brave", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    },
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "f", "Exclude Firefox", "f -> Exclude Firefox",
                        "mullvad-exclude firefox", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    },
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "l", "Exclude Librewolf", "l -> Exclude Librewolf",
                        "GTK_USE_PORTAL=1 mullvad-exclude librewolf", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    }
                )
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "p", "Profiles", "p -> Profiles",
                NULL, 
                NULL, 
                NULL, 
                false, false, false, false, false, 
                PREFIX(
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "c", "Profile", "c -> Profile",
                        "GTK_USE_PORTAL=1 mullvad-exclude librewolf -P banking \"bigbank.org\"", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    }
                )
            },
            {
                WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Open sites", "s -> Open sites",
                NULL, 
                NULL, 
                NULL, 
                false, false, false, false, false, 
                PREFIX(
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "c", "Codeberg", "c -> Codeberg",
                        "librewolf \"https://www.codeberg.org\"", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    },
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "t", "twitch", "t -> twitch",
                        "librewolf \"https://www.twitch.tv/\"", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    },
                    {
                        WK_MOD_NONE, WK_SPECIAL_NONE, "y", "Youtube", "y -> Youtube",
                        "librewolf \"https://vid.puffyan.us/feed/popular\"", 
                        NULL, 
                        NULL, 
                        false, false, false, false, false, NULL
                    }
                )
            }
        )
    }
);

#endif /* WK_CHORDS_H_ */
