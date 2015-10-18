#include <pebble.h>

#define LANG_SWEDISH 0
#define LANG_ITALIAN 1
#define LANG_DEFAULT 255

char LANG_DAY[][7][10] = {
    {"söndag", "måndag", "tisdag", "onsdag", "torsdag", "fredag", "lördag"},
    {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"}
};


char LANG_MONTH[][12][3] = {
    {"jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec", },
    {"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic", }
};