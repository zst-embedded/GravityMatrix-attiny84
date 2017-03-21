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
#include "plusMeterMode.h"



uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void loopParallelLine(int8_t x, int8_t y, int8_t z) {
    /*
     * Gradient, m = (v1 - v2) / (h1 - h2)
     * Coordinates of center is (4.5, 4.5)
     *
     * m = 4.5-v2 / 4.5 - col
     * v2 = 4.5 - m*(4.5-col)
     */

    float gradient = -y*1.0 / x; // negative sign as the gradient is opposite in real life
    uint8_t bitPosition;
    if (x == 0 || fabs(gradient) > 6) { // infinite gradient or steeper than 4 gradient (8/2)
        for (uint8_t col = 1; col <= LED_WIDTH; col++) {
            MAX7219_Shift2Bytes(col, (col == 4 || col == 5) ? 0xFF : 0);
        }
    } else {
        uint8_t bits = 0xFF;
        for (uint8_t col = 1; col <= LED_WIDTH_HALF; col++) {
            bitPosition = 4.5 - gradient * (4.5-col); // equation from above
            if (col == 4) {
                uint8_t mask = 0b00011000; // center 2 always lit
                if (bits == 0) { // previous bits == 0
                    bits = (gradient > 0) ? 0x0F : 0xF0;
                } else {
                    /* Bug workaround: fix gaps */
                    if (bits == 0x03) { // previous bits is 11 bottom most, then fix gap
                        mask |= 0b0100;
                    } else if (bits == 0x01) { // previous bits is 01 bottom most, then fix gap
                        mask |= 0b0110;
                    } else if (bits == 0xC0) { // previous bits is 11 top most, then fix gap
                        mask |= 0b0010<<4;
                    } else if (bits == 0x80) { // previous bits is 01 top most, then fix gap
                        mask |= 0b0110<<4;
                    }
                    bits = ~((1<<(bitPosition)) - 1) & 0x0F;
                }
                bits |= mask;
            } else {
                if (0 <= bitPosition && bitPosition <= 8) {
                    bits = 0b11 << (bitPosition-1); //ie. for position 4, 0000 1000, shift only 3 times
                } else {
                    // fix bug of not allowing shifts to be overflown
                    bits = 0;
                }
            }
            MAX7219_Shift2Bytes(col, bits);
            MAX7219_Shift2Bytes(LED_WIDTH + 1 - col, reverse(bits)); // Reverse bytes as diagonally symmetrical
        }
    }
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
            char input[5] = {' ', '#', '1'+currentMode, '\0'};
            //sprintf(input, " #%d", (currentMode+1));
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
                break;
            case 3:
                loopParallelLine(x, y, z);
                break;
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
                if (currentMode >= 3) {
                    currentMode = 0;
                } else {
                    currentMode++;
                }
            }
            _delay_ms(100);
        }

    }
}