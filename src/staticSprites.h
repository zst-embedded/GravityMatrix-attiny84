#pragma once

/*
 *
 *

// 3
 0
0
0b01111111
0b01001001
0b01001001
0b01001001
0
0
 */

PROGMEM const uint8_t progmem_smiley[] = {
        0x00,
        0x64,
        0x62,
        0x02,
        0x02,
        0x62,
        0x64,
        0x00
};
PROGMEM const uint8_t progmem_sad[] = {
        0x00,
        0x22,
        0x64,
        0x04,
        0x04,
        0x64,
        0x22,
        0x00
};

PROGMEM const uint8_t progmem_go[] = {
        0b00001110,
        0b00001010,
        0b00001110,
        0,
        0b01101110,
        0b01001010,
        0b01000010,
        0b01111110
};

uint8_t reverse(uint8_t b);

inline void showSprite_smileyFace() {
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_Shift2Bytes(i, pgm_read_byte_near(progmem_smiley + i - 1));
    }
}

void showSprite_sadFace() {
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_Shift2Bytes(i, pgm_read_byte_near(progmem_sad + i - 1));
    }
}

void showSprite_blank() {
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_Shift2Bytes(1, 0);
    }
}

void showSprite_numb(uint8_t numb) {
    MAX7219_Shift2Bytes(1, 0);
    for (uint8_t i = 0; i < 5; i++) {
        MAX7219_Shift2Bytes(6-i, reverse(getFont('0'+numb, i)));
    }
    MAX7219_Shift2Bytes(7, 0);
    MAX7219_Shift2Bytes(8, 0);
}

void showSprite_go() {
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_Shift2Bytes(i, pgm_read_byte_near(progmem_go + i - 1));
    }
}

void showSprite_321Go() {
    for (uint8_t i = 3; i > 0; i--) {
        showSprite_numb(i);
        _delay_ms(800);
    }
    showSprite_go();
    _delay_ms(800);
}
