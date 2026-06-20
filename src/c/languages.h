#include <pebble.h>

#define LANG_SWEDISH 0
#define LANG_ITALIAN 1
#define LANG_RUSSIAN 2
#define LANG_DUTCH 3
#define LANG_NORVEGIAN 4
#define LANG_CATALAN 5
#define LANG_MALAY 6
#define LANG_POLISH 7
#define LANG_HUNGARIAN 8
#define LANG_DEFAULT 255

char LANG_DAY[][7][23] = {
    {"söndag", "måndag", "tisdag", "onsdag", "torsdag", "fredag", "lördag"},
    {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"},
    {"ВОСКРЕСЕНЬЕ\0", "ПОНЕДЕЛЬНИК\0", "ВТОРНИК\0", "СРЕДА\0", "ЧЕТВЕРГ\0", "ПЯТНИЦА\0", "СУББОТА\0"},
    {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"},
    {"Søndag", "Mandag", "Tirsdag", "Onsdag", "Torsdag", "Fredag", "Lørdag"},
    {"Diumenge", "Dilluns", "Dimarts", "Dimecres", "Dijous", "Divendres", "Dissabte"},
    {"Ahad", "Isnin", "Selasa", "Rabu", "Khamis", "Jumaat", "Sabtu"},
    {"Niedziela", "Poniedziałek", "Wtorek", "Šroda", "Czwartek", "Pia¸tek", "Sobota"},
    {"Vasárnap", "Hétfő", "Kedd", "Szerda", "Csütörtök", "Péntek", "Szombat"}};

char LANG_MONTH[][12][6] = {
    {"jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec"},
    {"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic"},
    {"ЯНВ", "ФЕВ", "МАР", "АПР", "МАЯ", "ИЮН", "ИЮЛ", "АВГ", "СЕН", "ОКТ", "НОЯ", "ДЕК"},
    {"jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"},
    {"jan", "feb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "des"},
    {"Gen", "Feb", "Mar", "Abr", "Mai", "jun", "jul", "Ago", "Set", "oct", "nov", "des"},
    {"jan", "Feb", "Mar", "Apr", "Mei", "jun", "jul", "Ogs", "Sep", "okt", "nov", "dis"},
    {"Sty", "Lut", "Mar", "Kwi", "Maj", "Cze", "Lip", "Sie", "Wrz", "Paž", "Lis", "Gru"},
    {"Jan", "Feb", "Már", "Ápr", "Máj", "Jún", "Júl", "Aug", "Sze", "Okt", "Nov", "Dec"}};

char LANG_DAY_ABBR[][7][4] = {
    {"SÖN", "MÅN", "TIS", "ONS", "TOR", "FRE", "LÖR"},
    {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"},
    {"ВС", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"},
    {"ZON", "MAA", "DIN", "WOE", "DON", "VRI", "ZAT"},
    {"SØN", "MAN", "TIR", "ONS", "TOR", "FRE", "LØR"},
    {"DIU", "DIL", "DIM", "DMC", "DIJ", "DIV", "DIS"},
    {"AHA", "ISN", "SEL", "RAB", "KHA", "JUM", "SAB"},
    {"NIE", "PON", "WTO", "SRO", "CZW", "PIA", "SOB"},
    {"VAS", "HÉT", "KED", "SZE", "CSÜ", "PÉN", "SZO"}};

char LANG_MONTH_UPPER[][12][6] = {
    {"JAN", "FEB", "MAR", "APR", "MAJ", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEC"},
    {"GEN", "FEB", "MAR", "APR", "MAG", "GIU", "LUG", "AGO", "SET", "OTT", "NOV", "DIC"},
    {"ЯНВ", "ФЕВ", "МАР", "АПР", "МАЯ", "ИЮН", "ИЮЛ", "АВГ", "СЕН", "ОКТ", "НОЯ", "ДЕК"},
    {"JAN", "FEB", "MAA", "APR", "MEI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEC"},
    {"JAN", "FEB", "MAR", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DES"},
    {"GEN", "FEB", "MAR", "ABR", "MAI", "JUN", "JUL", "AGO", "SET", "OCT", "NOV", "DES"},
    {"JAN", "FEB", "MAR", "APR", "MEI", "JUN", "JUL", "OGS", "SEP", "OKT", "NOV", "DIS"},
    {"STY", "LUT", "MAR", "KWI", "MAJ", "CZE", "LIP", "SIE", "WRZ", "PAZ", "LIS", "GRU"},
    {"JAN", "FEB", "MAR", "APR", "MAJ", "JUN", "JUL", "AUG", "SZE", "OKT", "NOV", "DEC"}};