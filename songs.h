#ifndef SONGS_H
#define SONGS_H

#include "Arduino.h"

// ================= NOTE FREQUENCIES =================
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

// ================= SONGS DATA (PROGMEM) =================

// 1. Super Mario Bros Theme
const int song1_melody[] PROGMEM = {
  NOTE_E5, NOTE_E5, REST, NOTE_E5, REST, NOTE_C5, NOTE_E5, REST,
  NOTE_G5, REST, NOTE_G4, REST,
  NOTE_C5, NOTE_G4, REST, NOTE_E4,
  NOTE_A4, NOTE_B4, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_F5, NOTE_G5,
  REST, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_B4
};
const int song1_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 8,
  4, 4, 4, 4,
  4, 8, 4, 4,
  4, 4, 8, 4,
  8, 8, 8, 4, 8, 8,
  8, 4, 8, 8, 4
};

// 2. Zelda: Song of Storms
const int song2_melody[] PROGMEM = {
  NOTE_D4, NOTE_F4, NOTE_D5,
  NOTE_D4, NOTE_F4, NOTE_D5,
  NOTE_E5, NOTE_F5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_A4,
  NOTE_A4, NOTE_D4, NOTE_F4, NOTE_G4, NOTE_A4,
  NOTE_A4, NOTE_D4, NOTE_F4, NOTE_G4, NOTE_E4
};
const int song2_tempo[] PROGMEM = {
  8, 8, 2,
  8, 8, 2,
  6, 16, 6, 16, 6, 8, 2,
  4, 4, 8, 8, 2,
  4, 4, 8, 8, 2
};

// 3. Star Wars: Imperial March
const int song3_melody[] PROGMEM = {
  NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_C5,
  NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4,
  NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_C5,
  NOTE_GS4, NOTE_F4, NOTE_C5, NOTE_A4
};
const int song3_tempo[] PROGMEM = {
  4, 4, 4, 8, 16,
  4, 8, 16, 2,
  4, 4, 4, 8, 16,
  4, 8, 16, 2
};

// 4. Happy Birthday
const int song4_melody[] PROGMEM = {
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4,
  NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_AS4, NOTE_AS4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4
};
const int song4_tempo[] PROGMEM = {
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 4, 4,
  8, 8, 4, 4, 4, 2
};

// 5. Tetris Theme
const int song5_melody[] PROGMEM = {
  NOTE_E5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4,
  NOTE_A4, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_C5, NOTE_A4, NOTE_A4
};
const int song5_tempo[] PROGMEM = {
  4, 8, 8, 4, 8, 8,
  4, 8, 8, 4, 8, 8,
  4, 8, 4, 4,
  4, 4, 4
};

// 6. Harry Potter Theme
const int song6_melody[] PROGMEM = {
  NOTE_D4, NOTE_G4, NOTE_AS4, NOTE_A4,
  NOTE_G4, NOTE_D5, NOTE_C5,
  NOTE_A4, NOTE_G4, NOTE_AS4, NOTE_A4,
  NOTE_F4, NOTE_GS4, NOTE_D4
};
const int song6_tempo[] PROGMEM = {
  4, 4, 8, 4,
  2, 4, 2,
  2, 4, 8, 4,
  2, 4, 2
};

// 7. Pink Panther
const int song7_melody[] PROGMEM = {
  REST, NOTE_DS4, NOTE_E4, REST, NOTE_FS4, NOTE_G4, REST, NOTE_DS4, NOTE_E4, NOTE_FS4,  NOTE_G4, NOTE_C5, NOTE_B4, NOTE_E4, NOTE_G4, NOTE_B4,
  NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_E4, NOTE_D4, NOTE_E4
};
const int song7_tempo[] PROGMEM = {
  2, 8, 4, 2, 8, 4, 2, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  2, 16, 16, 16, 16, 2
};

// 8. Nokia Ringtone
const int song8_melody[] PROGMEM = {
  NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4,
  NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4,
  NOTE_B4, NOTE_A4, NOTE_CS4, NOTE_E4,
  NOTE_A4
};
const int song8_tempo[] PROGMEM = {
  8, 8, 4, 4,
  8, 8, 4, 4,
  8, 8, 4, 4,
  2
};

// 9. Twinkle Twinkle Little Star
const int song9_melody[] PROGMEM = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};
const int song9_tempo[] PROGMEM = {
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2
};

// 10. Jingle Bells
const int song10_melody[] PROGMEM = {
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_G5
};
const int song10_tempo[] PROGMEM = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8, 2,
  8, 8, 8, 8, 8, 8, 8, 16, 16, 8, 8, 8, 4, 4
};

// 11. Silent Night
const int song11_melody[] PROGMEM = {
  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4,
  NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4,
  NOTE_D5, NOTE_D5, NOTE_B4,
  NOTE_C5, NOTE_C5, NOTE_G4
};
const int song11_tempo[] PROGMEM = {
  6, 8, 4, 2,
  6, 8, 4, 2,
  2, 4, 2,
  2, 4, 2
};

// 12. Take On Me
const int song12_melody[] PROGMEM = {
  NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, NOTE_B4, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, NOTE_D5, NOTE_FS5, NOTE_FS5, NOTE_FS5, NOTE_E5, NOTE_E5, NOTE_FS5, NOTE_E5
};
const int song12_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

// 13. Cantina Band (Star Wars)
const int song13_melody[] PROGMEM = {
  NOTE_B4, NOTE_E5, NOTE_B4, NOTE_E5,
  NOTE_B4, NOTE_E5, NOTE_B4,
  NOTE_AS4, NOTE_B4, NOTE_AS4, NOTE_B4, NOTE_AS4, NOTE_E4
};
const int song13_tempo[] PROGMEM = {
  8, 8, 8, 8,
  8, 8, 8,
  16, 16, 16, 16, 8, 8
};

// 14. Sweet Child O' Mine
const int song14_melody[] PROGMEM = {
  NOTE_CS5, NOTE_CS6, NOTE_GS5, NOTE_FS5, NOTE_FS6, NOTE_GS5, NOTE_F6, NOTE_GS5,
  NOTE_CS5, NOTE_CS6, NOTE_GS5, NOTE_FS5, NOTE_FS6, NOTE_GS5, NOTE_F6, NOTE_GS5
};
const int song14_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8
};

// 15. Fur Elise
const int song15_melody[] PROGMEM = {
  NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_B4, NOTE_D5, NOTE_C5, NOTE_A4,
  REST, NOTE_C4, NOTE_E4, NOTE_A4, NOTE_B4,
  REST, NOTE_E4, NOTE_GS4, NOTE_B4, NOTE_C5
};
const int song15_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 8, 2,
  8, 8, 8, 8, 2,
  8, 8, 8, 8, 2
};

// 16. Ode to Joy
const int song16_melody[] PROGMEM = {
  NOTE_E4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4
};
const int song16_tempo[] PROGMEM = {
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 8, 2
};

// 17. Pirates of Caribbean
const int song17_melody[] PROGMEM = {
  NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
  NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
  NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
  NOTE_A4, NOTE_G4, NOTE_A4
};
const int song17_tempo[] PROGMEM = {
  8, 8, 4, 8, 8,
  8, 8, 4, 8, 8,
  8, 8, 4, 8, 8,
  8, 8, 2
};

// 18. Mission Impossible
const int song18_melody[] PROGMEM = {
  NOTE_G4, NOTE_G4, NOTE_AS4, NOTE_C5, NOTE_G4, NOTE_G4, NOTE_F4, NOTE_FS4,
  NOTE_G4, NOTE_G4, NOTE_AS4, NOTE_C5, NOTE_G4, NOTE_G4, NOTE_F4, NOTE_FS4
};
const int song18_tempo[] PROGMEM = {
  2, 2, 8, 8, 2, 2, 8, 8,
  2, 2, 8, 8, 2, 2, 8, 8
};

// 19. Indiana Jones
const int song19_melody[] PROGMEM = {
  NOTE_E5, NOTE_F5, NOTE_G5, NOTE_C6,
  NOTE_D5, NOTE_E5, NOTE_F5,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_F6,
  NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6, NOTE_E6
};
const int song19_tempo[] PROGMEM = {
  8, 8, 4, 1,
  8, 8, 1,
  8, 8, 4, 1,
  8, 8, 4, 4, 1
};

// 20. James Bond
const int song20_melody[] PROGMEM = {
  NOTE_E4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_F4
};
const int song20_tempo[] PROGMEM = {
  8, 16, 16, 8, 8, 8, 8, 8,
  8, 16, 16, 8, 8, 8, 8, 8
};

// 21. Pokemon Theme
const int song21_melody[] PROGMEM = {
  NOTE_B3, NOTE_B3, NOTE_B3, NOTE_B3, NOTE_A3, NOTE_B3, NOTE_E4,
  NOTE_E4, NOTE_D4, NOTE_B3, NOTE_A3,
  NOTE_A3, NOTE_A3, NOTE_G3, NOTE_A3, NOTE_D4, NOTE_B3
};
const int song21_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 2,
  8, 8, 8, 2,
  8, 8, 8, 8, 4, 2
};

// 22. Gravity Falls
const int song22_melody[] PROGMEM = {
  NOTE_F4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_D4,
  NOTE_D4, NOTE_F4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_E4
};
const int song22_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

// 23. Sherlock Theme
const int song23_melody[] PROGMEM = {
  NOTE_D4, NOTE_F4, NOTE_A4, NOTE_D5,
  NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_D4
};
const int song23_tempo[] PROGMEM = {
  4, 4, 4, 2,
  8, 8, 8, 8, 2, 2
};

// 24. Coffin Dance (Astronomia)
const int song24_melody[] PROGMEM = {
  NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5,
  NOTE_F5, NOTE_F5, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_A4
};
const int song24_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 2
};

// 25. Rick Roll (Never Gonna Give You Up)
const int song25_melody[] PROGMEM = {
  NOTE_D5, NOTE_E5, NOTE_A4, NOTE_E5, NOTE_FS5, NOTE_A5, NOTE_GS5, NOTE_E5,
  NOTE_D5, NOTE_E5, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_E5, NOTE_FS5, NOTE_A5, NOTE_B5, NOTE_A5, NOTE_GS5, NOTE_D5
};
const int song25_tempo[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 4,
  8, 8, 8, 16, 16, 8, 8, 8, 8, 8, 8, 2
};

// ================= SONGS TABLE =================

struct Song {
  const char* name;
  const int* melody;
  const int* tempo;
  int length;
};

const Song SONGS[] = {
  {"Super Mario", song1_melody, song1_tempo, sizeof(song1_melody)/sizeof(int)},
  {"Zelda Storms", song2_melody, song2_tempo, sizeof(song2_melody)/sizeof(int)},
  {"Imperial March", song3_melody, song3_tempo, sizeof(song3_melody)/sizeof(int)},
  {"Happy Birthday", song4_melody, song4_tempo, sizeof(song4_melody)/sizeof(int)},
  {"Tetris", song5_melody, song5_tempo, sizeof(song5_melody)/sizeof(int)},
  {"Harry Potter", song6_melody, song6_tempo, sizeof(song6_melody)/sizeof(int)},
  {"Pink Panther", song7_melody, song7_tempo, sizeof(song7_melody)/sizeof(int)},
  {"Nokia Ringtone", song8_melody, song8_tempo, sizeof(song8_melody)/sizeof(int)},
  {"Twinkle Star", song9_melody, song9_tempo, sizeof(song9_melody)/sizeof(int)},
  {"Jingle Bells", song10_melody, song10_tempo, sizeof(song10_melody)/sizeof(int)},
  {"Silent Night", song11_melody, song11_tempo, sizeof(song11_melody)/sizeof(int)},
  {"Take On Me", song12_melody, song12_tempo, sizeof(song12_melody)/sizeof(int)},
  {"StarWars Cantina", song13_melody, song13_tempo, sizeof(song13_melody)/sizeof(int)},
  {"Sweet Child", song14_melody, song14_tempo, sizeof(song14_melody)/sizeof(int)},
  {"Fur Elise", song15_melody, song15_tempo, sizeof(song15_melody)/sizeof(int)},
  {"Ode to Joy", song16_melody, song16_tempo, sizeof(song16_melody)/sizeof(int)},
  {"Pirates Caribbean", song17_melody, song17_tempo, sizeof(song17_melody)/sizeof(int)},
  {"Mission Impossible", song18_melody, song18_tempo, sizeof(song18_melody)/sizeof(int)},
  {"Indiana Jones", song19_melody, song19_tempo, sizeof(song19_melody)/sizeof(int)},
  {"James Bond", song20_melody, song20_tempo, sizeof(song20_melody)/sizeof(int)},
  {"Pokemon Theme", song21_melody, song21_tempo, sizeof(song21_melody)/sizeof(int)},
  {"Gravity Falls", song22_melody, song22_tempo, sizeof(song22_melody)/sizeof(int)},
  {"Sherlock", song23_melody, song23_tempo, sizeof(song23_melody)/sizeof(int)},
  {"Coffin Dance", song24_melody, song24_tempo, sizeof(song24_melody)/sizeof(int)},
  {"Rick Roll", song25_melody, song25_tempo, sizeof(song25_melody)/sizeof(int)}
};

#endif
