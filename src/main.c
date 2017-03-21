#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include "zst-avr-usi-max7219.h"
#include "zst-avr-usi-adxl345.h"
#include "font-vert-5x8.h"


/*
VCC
(PCINT8/XTAL1/CLKI) PB0
(PCINT9/XTAL2) PB1
(PCINT11/RESET/dW) PB3
(PCINT10/INT0/OC0A/CKOUT) PB2
(PCINT7/ICP/OC0B/ADC7) PA7
(PCINT6/OC1A/SDA/MOSI/DI/ADC6) PA6

GND
PA0 (ADC0/AREF/PCINT0)
PA1 (ADC1/AIN0/PCINT1)
PA2 (ADC2/AIN1/PCINT2)
PA3 (ADC3/T0/PCINT3)
PA4 (ADC4/USCK/SCL/T1/PCINT4)
PA5 (ADC5/DO/MISO/OC1B/PCINT5)
 */
//https://github.com/sparkfun/SparkFun_ADXL345_Arduino_Library/blob/master/src/SparkFun_ADXL345.cpp



#define START_BRIGHTNESS (0xF)

#define LED_WIDTH (8)
#define LED_WIDTH_HALF (4)

#define LED_HEIGHT_MAX (8)
#define LED_HEIGHT_HALF (4)

#include "waterMode.h"



uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

#define MOVEMENT_SPEED 65  // Delay between frames in msec
void showText(const char input[]) {
    uint8_t x, movement, origOffset;
    for (uint8_t eachChar = 0; input[eachChar] != '\0'; eachChar++) { // FOR EACH CHAR
        for (movement = 0; movement < LED_WIDTH; movement++) {
            for (x = 1; x <= LED_WIDTH-movement; x++) {
                uint8_t a = getFont(input[eachChar], x+movement);
                MAX7219_Shift2Bytes(LED_WIDTH-x+1, reverse(a));
            }
            origOffset = x;
            for (; x <= LED_WIDTH && x <= LED_WIDTH; x++) {
                uint8_t b = getFont(input[eachChar+1], x-origOffset);
                MAX7219_Shift2Bytes(LED_WIDTH-x+1, reverse(b));
            }
            _delay_ms(MOVEMENT_SPEED);
        }
    }
}

void loopPlusMeter(int8_t x, int8_t y, int8_t z) {
    double rad_angle_tilt_vert = atan( x * 1.0 / z ); // x axis is pointing down
    double rad_angle_tilt_hori = atan( y * 1.0 / z ); // y axis is pointing right

    // center 2x2 is always lit
    // if pos, LED_HEIGHT_TOP_HALF_UP to LED_HEIGHT_MAX mapped to 0 to 45 deg
    // if neg, LED_HEIGHT_BOTTOM_HALF_DOWN to 0, mapped to 0 to -45deg


    uint8_t vert_center_col = 0b00011000; // For 0000 1111 -> do 1<<5 - 1 = 0001 0000 - 1/
    int8_t vert_expansion_numb = fabs(rad_angle_tilt_vert / M_PI_4 * LED_HEIGHT_HALF); // -> angle/45deg * 4LEDs
    uint8_t vert_expansion_bits = _BV(vert_expansion_numb + 1) - 1;
    if (0 == vert_expansion_numb) {
        // do nothing
    } else if (rad_angle_tilt_vert > 0) {
        // positive so, expand center 2 columns upwards
        vert_center_col |= (vert_expansion_bits << LED_HEIGHT_HALF );
    } else { // if (rad_angle_tilt_vert < 0) {
        // negative so, expand center 2 columns downwards
        vert_center_col |= (vert_expansion_bits << (LED_HEIGHT_HALF - vert_expansion_numb) );
    }


    int8_t hori_expansion_numb = fabs(rad_angle_tilt_hori / M_PI_4 * LED_WIDTH_HALF);

    MAX7219_Shift2Bytes(4, vert_center_col);
    MAX7219_Shift2Bytes(5, vert_center_col);
    if (0 == rad_angle_tilt_hori) {
        // do nothing
    } else if (rad_angle_tilt_hori < 0) {
        for (uint8_t col = 1; col < LED_WIDTH_HALF; col++) {
            MAX7219_Shift2Bytes(col, ( (LED_WIDTH_HALF - hori_expansion_numb) <= col ) ? 0b00011000 : 0);
        }
        //MAX7219_Shift2Bytes(4, vert_center_col);
        //MAX7219_Shift2Bytes(5, vert_center_col);
        for (uint8_t col = LED_WIDTH_HALF+2; col <= LED_WIDTH; col++) {
            MAX7219_Shift2Bytes(col, 0);
        }
    } else { //if (rad_angle_tilt_hori > 0) {
        for (uint8_t col = 1; col < LED_WIDTH_HALF; col++) {
            MAX7219_Shift2Bytes(col, 0);
        }
        //MAX7219_Shift2Bytes(4, vert_center_col);
        //MAX7219_Shift2Bytes(5, vert_center_col);
        for (uint8_t col = LED_WIDTH_HALF+2; col <= LED_WIDTH; col++) {
            MAX7219_Shift2Bytes(col, ( (LED_WIDTH_HALF + hori_expansion_numb) >= col ) ? 0b00011000 : 0);
        }
    }
}


int main(void) {
    // Pin 13 LED
    DDRA |= _BV(PA0); // output
    PORTA |= _BV(PA0); // output high

    // Pin 10 push button
    DDRA &= ~(_BV(PA3)); // input
    PORTA |= _BV(PA3); // internal pull up

    // Setup MAX7219 LED Matrix
    MAX7219_USI_SPI_Init();
    MAX7219_Shift2Bytes(MAX7219_MODE_SCAN_LIMIT, 7);
    MAX7219_Shift2Bytes(MAX7219_MODE_INTENSITY, START_BRIGHTNESS);
    MAX7219_Shift2Bytes(MAX7219_MODE_POWER, 0x1);
    MAX7219_Shift2Bytes(MAX7219_MODE_DECODE, 0x0);

    //_delay_ms(250);
    //MAX7219_Shift2Bytes(MAX7219_MODE_TEST, 1);
    //_delay_ms(250);
    //MAX7219_Shift2Bytes(MAX7219_MODE_TEST, 0);

    // Setup ADXL345 3 axis accelerometer
    ADXL345_USI_SPI_Init();
    int16_t x, y, z;
    uint8_t currentMode = 0,
            previousMode = 10,
            brightness = START_BRIGHTNESS;
    // previousMode=10 so scrolling text is shown on boot

    while (1) {
        // If mode change, show splash screen
        if (currentMode != previousMode) {
            // show mode as a letter
            char input[5];
            sprintf(input, " #%d", (currentMode+1));
            showText(input);
            previousMode = currentMode;
        }
        // Accelerometer Readings
        ADXL345_readAccel(&x, &y, &z);
        // ADXL Interrupts
        if (ADXL345_triggered(ADXL345_getInterruptSource(), ADXL345_DOUBLE_TAP)) {
            if (brightness >= 0xF) {
                brightness = 0;
            } else {
                brightness += 0x5;
            }
            MAX7219_Shift2Bytes(MAX7219_MODE_INTENSITY, brightness);
        }

        switch(currentMode) {
            case 0: // water level no-tilt
            case 1: // water level with-tilt
                loopWaterLevel(currentMode, x, y, z); // pass mode as tilt boolean
                break;
            case 2: // Plus  Meter
                loopPlusMeter(x, y, z);
            default:
                break;
        }

        _delay_ms(75);

        if ((PINA & _BV(PA3)) == 0) {
            int8_t confidence;
            // button debouncing
            for (confidence = 12; confidence > 0 && ((PINA & _BV(PA3)) == 0); confidence--) {
                _delay_ms(10);
            }
            if (confidence <= 0) {
                if (currentMode >= 2) {
                    currentMode = 0;
                } else {
                    currentMode++;
                }
            }
            _delay_ms(100);
        }

    }
}



/*
void displayRotatingLine(int deg) {
    while (deg <= 360) {
        displayLine(deg, 4);
    }
}

void displayMovingRow() {
    int8_t i, j;

    while (1) {
        MAX7219_Shift2Bytes(j, 0);

        for (i = 0; i < 8; i++) {
            for (j = 1; j <= 0x8; j++) {
                MAX7219_Shift2Bytes(j, _BV(i));
            }
            _delay_ms(30);
        }
        for (i = 7; i >= 0; i--) {
            for (j = 1; j <= 0x8; j++) {
                MAX7219_Shift2Bytes(j, _BV(i));
            }
            _delay_ms(30);
        }
    }
}*/