#include <pebble.h>

#define LANG_SWEDISH 0
#define LANG_ITALIAN 1
#define LANG_RUSSIAN 2
#define LANG_DUTCH 3
#define LANG_NORVEGIAN 4
#define LANG_CATALAN 5
#define LANG_MALAY 6
#define LANG_DEFAULT 255

char LANG_DAY[][7][23] = {
    {"söndag", "måndag", "tisdag", "onsdag", "torsdag", "fredag", "lördag"},
    {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"},
    {"ВОСКРЕСЕНЬЕ\0", "ПОНЕДЕЛЬНИК\0", "ВТОРНИК\0", "СРЕДА\0", "ЧЕТВЕРГ\0", "ПЯТНИЦА\0", "СУББОТА\0"},
    {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"},
    {"Søndag", "Mandag", "Tirsdag", "Onsdag", "Torsdag", "Fredag", "Lørdag"},
    {"Diumenge", "Dilluns", "Dimarts", "Dimecres", "Dijous", "Divendres", "Dissabte"},
    {"Ahad", "Isnin", "Selasa", "Rabu", "Khamis", "Jumaat", "Sabtu"}  
};


char LANG_MONTH[][12][6] = {
    {"jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec" },
    {"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic" },
    {"ЯНВ", "ФЕВ", "МАР", "АПР", "МАЯ", "ИЮН", "ИЮЛ", "АВГ", "СЕН", "ОКТ", "НОЯ", "ДЕК" },
    {"jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec" },  
    {"jan", "feb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "des" },
    {"Gen", "Feb", "Mar", "Abr", "Mai", "jun", "jul", "Ago", "Set", "oct", "nov", "des" },
    {"jan", "Feb", "Mar", "Apr", "Mei", "jun", "jul", "Ogs", "Sep", "okt", "nov", "dis" }  
};