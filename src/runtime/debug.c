#include <stdio.h>

#include "debug.h"

void
debugCairoPaint(const CairoPaint* paint)
{
    /* FOREGROUND */
    printf("[DEBUG] |     - Foreground value -\n");
    printf("[DEBUG] |     Red:            %#02X\n", (uint8_t)(paint->fg.r * 255));
    printf("[DEBUG] |     Green:          %#02X\n", (uint8_t)(paint->fg.g * 255));
    printf("[DEBUG] |     Blue:           %#02X\n", (uint8_t)(paint->fg.b * 255));
    printf("[DEBUG] |     Alpha:          %#02X\n", (uint8_t)(paint->fg.a * 255));

    /* BACKGROUND */
    printf("[DEBUG] |     - Background value -\n");
    printf("[DEBUG] |     Red:            %#02X\n", (uint8_t)(paint->bg.r * 255));
    printf("[DEBUG] |     Green:          %#02X\n", (uint8_t)(paint->bg.g * 255));
    printf("[DEBUG] |     Blue:           %#02X\n", (uint8_t)(paint->bg.b * 255));
    printf("[DEBUG] |     Alpha:          %#02X\n", (uint8_t)(paint->bg.a * 255));

    /* BORDER */
    printf("[DEBUG] |     --- Border value ---\n");
    printf("[DEBUG] |     Red:            %#02X\n", (uint8_t)(paint->bd.r * 255));
    printf("[DEBUG] |     Green:          %#02X\n", (uint8_t)(paint->bd.g * 255));
    printf("[DEBUG] |     Blue:           %#02X\n", (uint8_t)(paint->bd.b * 255));
    printf("[DEBUG] |     Alpha:          %#02X\n", (uint8_t)(paint->bd.a * 255));
}
