#ifndef WK_CHORDS_H_
#define WK_CHORDS_H_

#include "lib/common.h"
#include "lib/types.h"

#define NULL_CHORD  { WK_MOD_NONE, WK_SPECIAL_NONE, NULL, NULL, NULL, NULL, NULL, false, false, false, false, false, NULL }
#define PREFIX(...) (Chord[]){ __VA_ARGS__, NULL_CHORD }
#define CHORDS(...) { __VA_ARGS__, NULL_CHORD }

const Chord chords[] = CHORDS(
    /* mods,    specials,    key,    description,    command,    before,    after, */
    /* keep,    unhook,    nobefore,    noafter,    write,    chords */
    { WK_MOD_NONE, WK_SPECIAL_NONE, "m", "Tag & follow", "", "echo first", "echo last", 
      false, false, false, false, false, PREFIX(
        { WK_MOD_NONE, WK_SPECIAL_NONE, ":", "Tag1", "echo \":\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_SHIFT, WK_SPECIAL_RETURN, "RET", "Tag2", "echo \"RET\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "a", "Tag3", "echo \"a\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "テ", "Tag4", "echo \"テ\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "r", "Tag5", "echo \"r\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Tag6", "echo \"s\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "t", "Tag7", "echo \"t\"", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "g", "Tag8", "echo \"g\"", "", "", 
          false, false, false, false, false, NULL }
    )},
    { WK_MOD_NONE, WK_SPECIAL_NONE, "b", "Browser", "", "", "", 
      false, false, false, false, false, PREFIX(
        { WK_MOD_NONE, WK_SPECIAL_NONE, "b", "Open Brave", "brave", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "f", "Open Firefox", "firefox", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "l", "Open Librewolf", "librewolf", "", "", 
          false, false, false, false, false, NULL },
        { WK_MOD_NONE, WK_SPECIAL_NONE, "m", "Mullvad-exclude", "", "", "", 
          false, false, false, false, false, PREFIX(
            { WK_MOD_NONE, WK_SPECIAL_NONE, "b", "Exclude Brave", "mullvad-exclude brave", "", "", 
              false, false, false, false, false, NULL },
            { WK_MOD_NONE, WK_SPECIAL_NONE, "f", "Exclude Firefox", "mullvad-exclude firefox", "", "", 
              false, false, false, false, false, NULL },
            { WK_MOD_NONE, WK_SPECIAL_NONE, "l", "Exclude Librewolf", "GTK_USE_PORTAL=1 mullvad-exclude librewolf", "", "", 
              false, false, false, false, false, NULL }
        )},
        { WK_MOD_NONE, WK_SPECIAL_NONE, "p", "Profiles", "", "", "", 
          false, false, false, false, false, PREFIX(
            { WK_MOD_NONE, WK_SPECIAL_NONE, "c", "Profile", "GTK_USE_PORTAL=1 mullvad-exclude librewolf -P banking \"bigbank.org\"", "", "", 
              false, false, false, false, false, NULL }
        )},
        { WK_MOD_NONE, WK_SPECIAL_NONE, "s", "Open sites", "", "", "", 
          false, false, false, false, false, PREFIX(
            { WK_MOD_NONE, WK_SPECIAL_NONE, "c", "Codeberg", "librewolf \"https://www.codeberg.org\"", "", "", 
              false, false, false, false, false, NULL },
            { WK_MOD_NONE, WK_SPECIAL_NONE, "t", "twitch", "librewolf \"https://www.twitch.tv/\"", "", "", 
              false, false, false, false, false, NULL },
            { WK_MOD_NONE, WK_SPECIAL_NONE, "y", "Youtube", "librewolf \"https://vid.puffyan.us/feed/popular\"", "", "", 
              false, false, false, false, false, NULL }
        )}
    )}
);

#endif /* WK_CHORDS_H_ */
