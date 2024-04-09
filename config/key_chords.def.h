#ifndef WK_CHORDS_H_
#define WK_CHORDS_H_

#include "lib/types.h"

/* mods, specials,
 * key, description, hint,
 * command
 * before
 * after
 * flags, chords
 */
Chord chords[] = CHORDS(
    {
        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
        "b", "Browser", "b -> Browser",
        NULL, 
        NULL, 
        NULL, 
        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), 
        PREFIX(
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "b", "Open Brave", "b -> Open Brave",
                "brave", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "f", "Open Firefox", "f -> Open Firefox",
                "firefox", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "l", "Open Librewolf", "l -> Open Librewolf",
                "librewolf", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "m", "Mullvad-exclude", "m -> Mullvad-exclude",
                NULL, 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), 
                PREFIX(
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "b", "Exclude Brave", "b -> Exclude Brave",
                        "mullvad-exclude brave", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "f", "Exclude Firefox", "f -> Exclude Firefox",
                        "mullvad-exclude firefox", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "l", "Exclude Librewolf", "l -> Exclude Librewolf",
                        "mullvad-exclude librewolf", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                    }
                )
            }
        )
    },
    {
        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
        "c", "Client", "c -> Client",
        NULL, 
        NULL, 
        NULL, 
        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), 
        PREFIX(
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "l", "Tag Right", "l -> Tag Right",
                "dwmc shifttag +1 ; dwmc shiftview +1", 
                NULL, 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "L", "Mon Right", "L -> Mon Right",
                "dwmc tagnmon 1", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "h", "Tag Left", "h -> Tag Left",
                "dwmc shifttag -1 ; dwmc shiftview -1", 
                NULL, 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "H", "Mon Left", "H -> Mon Left",
                "dwmc tagnmon 0", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "m", "Tag & follow", "m -> Tag & follow",
                NULL, 
                "dwmc tagex 13", 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), 
                PREFIX(
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "a", "Tag 1", "a -> Tag 1",
                        "dwmc viewex 0", 
                        "dwmc tagex 0", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "r", "Tag 2", "r -> Tag 2",
                        "dwmc viewex 1", 
                        "dwmc tagex 1", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "s", "Tag 3", "s -> Tag 3",
                        "dwmc viewex 2", 
                        "dwmc tagex 2", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "t", "Tag 4", "t -> Tag 4",
                        "dwmc viewex 3", 
                        "dwmc tagex 3", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "g", "Tag 5", "g -> Tag 5",
                        "dwmc viewex 4", 
                        "dwmc tagex 4", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "m", "Tag 6", "m -> Tag 6",
                        "dwmc viewex 5", 
                        "dwmc tagex 5", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "n", "Tag 7", "n -> Tag 7",
                        "dwmc viewex 6", 
                        "dwmc tagex 6", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "e", "Tag 8", "e -> Tag 8",
                        "dwmc viewex 7", 
                        "dwmc tagex 7", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "i", "Tag 9", "i -> Tag 9",
                        "dwmc viewex 8", 
                        "dwmc tagex 8", 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    }
                )
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "q", "Quit", "q -> Quit",
                "sleep 0.1 ; dwmc killclient", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "s", "Sticky", "s -> Sticky",
                "dwmc togglesticky", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "t", "Tag", "t -> Tag",
                NULL, 
                NULL, 
                NULL, 
                MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), 
                PREFIX(
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "a", "Tag 1", "a -> Tag 1",
                        "dwmc tagex 0", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "r", "Tag 2", "r -> Tag 2",
                        "dwmc tagex 1", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "s", "Tag 3", "s -> Tag 3",
                        "dwmc tagex 2", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "t", "Tag 4", "t -> Tag 4",
                        "dwmc tagex 3", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "g", "Tag 5", "g -> Tag 5",
                        "dwmc tagex 4", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "m", "Tag 6", "m -> Tag 6",
                        "dwmc tagex 5", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "n", "Tag 7", "n -> Tag 7",
                        "dwmc tagex 6", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "e", "Tag 8", "e -> Tag 8",
                        "dwmc tagex 7", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    },
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "i", "Tag 9", "i -> Tag 9",
                        "dwmc tagex 8", 
                        NULL, 
                        NULL, 
                        MAKE_FLAGS(true, false, false, false, false, false, false, false, false, false), NULL
                    }
                )
            }
        )
    },
    {
        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
        "e", "Emacs", "e -> Emacs",
        NULL, 
        "dwmc viewex 1", 
        NULL, 
        MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), 
        PREFIX(
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "e", "Open blank", "e -> Open blank",
                "emacsclient -c -a ''", 
                "dwmc viewex 1", 
                NULL, 
                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "p", "Projects", "p -> Projects",
                NULL, 
                "dwmc viewex 1", 
                NULL, 
                MAKE_FLAGS(false, false, true, false, false, false, false, false, false, false), 
                PREFIX(
                    {
                        MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                        "c", "C projects", "c -> C projects",
                        NULL, 
                        "dwmc viewex 1", 
                        NULL, 
                        MAKE_FLAGS(false, false, true, false, false, false, false, false, false, false), 
                        PREFIX(
                            {
                                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                                "c", "Projects dir", "c -> Projects dir",
                                "emacs \"~/programs/C\"", 
                                "dwmc viewex 1", 
                                NULL, 
                                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                            },
                            {
                                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                                "d", "dwm", "d -> dwm",
                                "emacs \"~/.local/src/dwm\"", 
                                "dwmc viewex 1", 
                                NULL, 
                                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                            },
                            {
                                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                                "m", "dmenu", "m -> dmenu",
                                "emacs \"~/.local/src/dmenu\"", 
                                "dwmc viewex 1", 
                                NULL, 
                                MAKE_FLAGS(false, false, false, false, false, false, false, false, false, false), NULL
                            }
                        )
                    }
                )
            },
            {
                MAKE_MODS(false, false, false, false), WK_SPECIAL_NONE, 
                "r", "Roam", "r -> Roam",
                "emacsclient -c -a '' \"$HOME/ewiki/20230620214854-startpage.org\"", 
                NULL, 
                NULL, 
                MAKE_FLAGS(false, false, false, true, false, false, false, false, false, false), NULL
            }
        )
    }
);

#endif /* WK_CHORDS_H_ */
