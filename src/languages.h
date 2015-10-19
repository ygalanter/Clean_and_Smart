#include <pebble.h>

#define LANG_SWEDISH 0
#define LANG_ITALIAN 1
#define LANG_RUSSIAN 2
#define LANG_DUTCH 3
#define LANG_NORVEGIAN 4
#define LANG_DEFAULT 255

char LANG_DAY[][7][23] = {
    {"söndag", "måndag", "tisdag", "onsdag", "torsdag", "fredag", "lördag"},
    {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"},
    {"ВОСКРЕСЕНЬЕ\0", "ПОНЕДЕЛЬНИК\0", "ВТОРНИК\0", "СРЕДА\0", "ЧЕТВЕРГ\0", "ПЯТНИЦА\0", "СУББОТА\0"},
    {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"},
    {"Søndag", "Mandag", "Tirsdag", "Onsdag", "Torsdag", "Fredag", "Lørdag"}  
};


char LANG_MONTH[][12][6] = {
    {"jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec" },
    {"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic" },
    {"ЯНВ", "ФЕВ", "МАР", "АПР", "МАЯ", "ИЮН", "ИЮЛ", "АВГ", "СЕН", "ОКТ", "НОЯ", "ДЕК" },
    {"jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec" },  
    {"jan", "feb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "des" }  
};