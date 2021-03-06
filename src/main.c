#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/delay.h>
#include "zst-avr-usi-max7219.h"
#include "zst-avr-usi-adxl345.h"
#include "font-vert-5x8.h"
#include "staticSprites.h"

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

void showText(const char input[]);
uint8_t reverse(uint8_t b);

#include "waterMode.h"
#include "plusMeterMode.h"
#include "parallelLineMode.h"

uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

// http://www.avrfreaks.net/forum/tiny-fast-prng
uint8_t rnd(void) {
    static uint8_t s=0xaa,a=0;
    s^=s<<3;
    s^=s>>5;
    s^=a++>>2;
    return s;
}

#define debouncingButton(confidence, condition) \
for ((confidence) = 12; (confidence) > 0 && (condition); (confidence)--) { \
    _delay_ms(10);\
}


// Inspired by: https://github.com/brnunes/Arduino-Doodle-Jump
// and: https://www.youtube.com/watch?v=oJbJWe_KpdU

#define DOODLER_JUMP_MAX_THRESH (4) // max threshold to jump, before shifting screen down
#define DOODLER_JUMP_HEIGHT (5) // max height to jump
#define DOODLER_FALL_MAX_THRESH (-4) // fall below amount of dots and you lose
#define DOODLER_JUMP_CYCLES (15.0)
#define DOODLER_REFRESH_DELAY (50)
#define DOODLER_DEFAULT_BITS (0b11U)
#define DOODLER_TILT_ANGLE (M_PI/6)

struct Doodler {
    volatile int8_t col;
    int8_t orig_pos;
    uint8_t bits; // beginning shift 2  start at the center 0b0001 1100
    int8_t jump_shift;
    uint8_t frame;
    bool blink;
};

void loopDoodleJump() {
    for(;;) {
        showSprite_321Go();
        uint8_t matrix_col[8] = {
                0b00001000,
                0b00001000,
                0b00001001,
                0b00000001,
                0b01000001,
                0b01000000,
                0b01000000,
                0000000000
        };

        /** Setup game variables*/
        struct Doodler doodler = {
                .orig_pos = 0,
                .frame = 0,
                .blink = false,
        };

        int16_t x, y, z;
        double rad_angle_tilt = 0;
        int8_t score = -2; // -2 to fix first line and last line (upon losing)
        while (true) {
            /** Process sensor data */
            ADXL345_readAccel(&x, &y, &z);
            rad_angle_tilt = atan(y * 1.0 / z);

            /** Update doodler location */
            // rad_angle_tilt:              -pi/4 -> col 0 | +pi/4 -> col 7
            // rad_angle_tilt/M_PI_4:       -1 -> col 0 | +1 -> col 7
            // rad_angle_tilt/M_PI_4 + 1:    0 -> col 0 | +2 -> col 7
            // 3.5*(rad_angle_tilt/M_PI_4 + 1):   0 -> col 0 | +7 -> col 7
            doodler.col = ((rad_angle_tilt / DOODLER_TILT_ANGLE + 1) * 3.5);
            if (doodler.col < 0) {
                doodler.col = 0;
            } else if (doodler.col > 7) {
                doodler.col = 7;
            }

            doodler.jump_shift = DOODLER_JUMP_HEIGHT * sinf(doodler.frame / DOODLER_JUMP_CYCLES * M_PI);
            doodler.frame++;

            uint8_t doodlerResultantShift = (doodler.orig_pos + doodler.jump_shift);
            if (doodler.jump_shift > DOODLER_FALL_MAX_THRESH) {
                doodler.bits = DOODLER_DEFAULT_BITS << doodlerResultantShift;
            } else {
                // Lose as fall too low
                break;
            }

            /** Check if doodler bouncing on platform */
            // If doodler is falling and it touches a platform, change direction up
            if (doodler.frame > (0.5*DOODLER_JUMP_CYCLES) &&
                ((matrix_col[doodler.col] >> doodlerResultantShift) & 0x01) != 0) {
                doodler.frame = 0;
                doodler.orig_pos = doodlerResultantShift;
            }

            /** If doodler too high, shift screen down */
            if (doodlerResultantShift > DOODLER_JUMP_MAX_THRESH) {
                // Shift screen down
                // also, save last row for later
                uint8_t platformBitsLeavingRow = 0;
                for (uint8_t col = 0; col < 8; col++) {
                    platformBitsLeavingRow |= (0x01 & matrix_col[col]);
                    platformBitsLeavingRow <<= 1;
                    matrix_col[col] >>= 1;
                }
                // Shift doodler down
                doodler.orig_pos--;
                doodler.bits >>= 1;

                // Generate new platform, if we shifted out a platform
                if (platformBitsLeavingRow != 0) {
                    PORTA ^= _BV(PA0);
                    uint8_t newPlatformShift = (rnd() % 6);
                    for (uint8_t col = 0; col < 3; col++) {
                        matrix_col[newPlatformShift + col] |= 0x80;
                    }
                    score++;
                }
                // Also add points on leaving platform
                // Because we may bounce on the same platform twice
                // So a better score system will be to check how many platforms are cleared
            }

            /** Doodler is constantly flashing every cycle */
            doodler.blink = !doodler.blink; // ball blinks every cycle

            /** Update Screen */
            for (uint8_t col = 0; col < 8; col++) {
                if (col == doodler.col && doodler.blink) {
                    MAX7219_Shift2Bytes(col + 1, matrix_col[col] | doodler.bits);
                } else {
                    MAX7219_Shift2Bytes(col + 1, matrix_col[col]);
                }
            }

            _delay_ms(DOODLER_REFRESH_DELAY);

            if ((PINA & _BV(PA1)) == 0) {
                int8_t confidence;
                debouncingButton(confidence, (PINA & _BV(PA1)) == 0);
                if (confidence <= 0) {
                    return;
                }
            }
        }
        char buffer[5], output[6] = " ";
        itoa(score,buffer,10);
        for (uint8_t i = 1; i <= 5; i++) {
            output[i] = buffer[i-1];
        }
        showText(output);
        showSprite_sadFace();
        _delay_ms(2000);
    }
}

#define BREAKOUT_PADDLE_BITS (0b111)
#define BREAKOUT_PADDLE_INITIAL (2) // How much to shift paddle bits on start. // If this is 0, then paddle will be bottom most initially
#define BREAKOUT_BALL_WAIT_CYCLE (4) // Lower = faster speed.
#define BREAKOUT_REFRESH_DELAY (50) // Lower = faster speed.
#define BREAKOUT_TILT_ANGLE (M_PI/6)

// Inspired by https://www.youtube.com/watch?v=mFzewcuDCFY
// col 1 = paddle
// col 8,7,6 = pieces
// FOR BALL:
// - OR the bit of the ball ball onto col 2-5
// - If ball in col 6-8, clear bits
// - ball <</>> on 45 deg
void loopAtariBreakout() {
    for(;;) {
        uint8_t matrix_col[8] = {
                BREAKOUT_PADDLE_BITS << BREAKOUT_PADDLE_INITIAL, // paddle row
                0, 0, 0, 0, // center playing area
                0xFF, 0xFF, 0xFF // pieces
        };
        uint8_t ball_bits = 0b010 << BREAKOUT_PADDLE_INITIAL, // center of the paddle
                ball_colno = 2; // paddle is at col 1

        volatile int8_t ball_updatecount = -BREAKOUT_BALL_WAIT_CYCLE;
        bool ball_blink = true;

        enum BallDirection { // up and down direction are with respect to paddle viewing up
            UP_STRAIGHT,
            UP_LEFT,        // << shift up the row
            UP_RIGHT,       // >> shift down the row
            DOWN_STRAIGHT,
            DOWN_LEFT,      // << shift up the row
            DOWN_RIGHT,     // >> shift down the row
        };

        enum BallDirection ball_dir = UP_STRAIGHT;
        int16_t x, y, z, paddle_shift;

        showSprite_321Go();

        bool didWin;
        while(true) {
            /** if win already, break out */
            didWin = (matrix_col[7] == 0) &&
                     (matrix_col[6] == 0) &&
                     (matrix_col[5] == 0);
            if (didWin)
                break;

            /** Process sensor data */
            ADXL345_readAccel(&x, &y, &z);
            double rad_angle_tilt = atan( x * 1.0 / z );
            // rad_angle_tilt:                         -pi/4 -> shift -1 | +pi/4 -> shift 6
            // rad_angle_tilt/M_PI_4:                  -1 -> shift -1 | +1 -> shift 6
            // ((rad_angle_tilt/M_PI_4)+1):             0 -> shift -1 | 2 -> shift 6
            // ((rad_angle_tilt/M_PI_4)+1)*2.5:         0 -> shift -1 | 5 -> shift 6
            // ((rad_angle_tilt/M_PI_4)+1)*2.5 -1:     -1 -> shift -1 | 6 -> shift 6
            paddle_shift = round ( ((rad_angle_tilt/BREAKOUT_TILT_ANGLE)+1) * 2.5 - 1 );
            if (paddle_shift <= -1) {
                //paddle_shift = -1;
                matrix_col[0] = BREAKOUT_PADDLE_BITS >> 1;
            } else {
                if (paddle_shift >= 6)
                    paddle_shift = 6;
                matrix_col[0] = BREAKOUT_PADDLE_BITS << paddle_shift;
            }

            /** Update ball location */
            ball_updatecount++;

            if (BREAKOUT_BALL_WAIT_CYCLE <= ball_updatecount) {
                // Handle bouncing balls on cols/ends
                if (ball_colno <= 2) { // Handle ball bounce on paddle
                    if (ball_colno <= 1) {
                        didWin = false;
                        break; // LOST GAME
                    } else {
                        // Handle ball bounce on paddle
                        bool paddleDirectlyBelowBall = (ball_bits & matrix_col[0]) != 0;
                        bool isCenter = (ball_bits | ball_bits << 1 | ball_bits >> 1) == matrix_col[0];
                        if (paddleDirectlyBelowBall) {
                            if (isCenter) {
                                ball_dir = UP_STRAIGHT;
                            } else {
                                if ((ball_bits >> 1 & matrix_col[0]) == 0) {
                                    // bit to the left is empty -> hence bounce towards right most of paddle
                                    ball_dir = UP_RIGHT;
                                } else if ((ball_bits << 1 & matrix_col[0]) == 0) {
                                    // bit to the right is empty -> hence bounce towards left most of paddle
                                    ball_dir = UP_LEFT;
                                }
                            }
                        } else {
                            if (DOWN_LEFT == ball_dir) {
                                // check edge
                                ball_bits <<= 1;
                                bool paddleEdgeHitBall = (ball_bits & matrix_col[0]) != 0;
                                if (paddleEdgeHitBall) {
                                    ball_dir = UP_RIGHT;
                                }
                            }
                            if (DOWN_RIGHT == ball_dir) {
                                // check edge
                                ball_bits >>= 1;
                                bool paddleEdgeHitBall = (ball_bits & matrix_col[0]) != 0;
                                if (paddleEdgeHitBall) {
                                    ball_dir = UP_LEFT;
                                }
                            }
                        }
                    }
                } else if (8 <= ball_colno) {
                    switch (ball_dir) {
                        case UP_STRAIGHT:
                            ball_dir = DOWN_STRAIGHT;
                            break;
                        case UP_LEFT:
                            ball_dir = DOWN_LEFT;//DOWN_RIGHT;
                            break;
                        case UP_RIGHT:
                            ball_dir = DOWN_RIGHT;//DOWN_LEFT;
                            break;
                    }
                }
                // Handle bouncing balls on rows/sides
                if ((ball_bits & 0x80) != 0) { // bouncing left
                    switch (ball_dir) {
                        case DOWN_LEFT:
                            ball_dir = DOWN_RIGHT;
                            break;
                        case UP_LEFT:
                            ball_dir = UP_RIGHT;
                            break;
                    }
                } else if ((ball_bits & 0x01) != 0) { // bouncing right
                    switch (ball_dir) {
                        case DOWN_RIGHT:
                            ball_dir = DOWN_LEFT;
                            break;
                        case UP_RIGHT:
                            ball_dir = UP_LEFT;
                            break;
                    }
                }

                // Move ball to next position
                switch (ball_dir) {
                    case UP_STRAIGHT:
                        ball_colno++;
                        break;
                    case UP_LEFT:
                        ball_colno++;
                        ball_bits <<= 1;
                        break;
                    case UP_RIGHT:
                        ball_colno++;
                        ball_bits >>= 1;
                        break;

                    case DOWN_STRAIGHT:
                        ball_colno--;
                        break;
                    case DOWN_LEFT:
                        ball_colno--;
                        ball_bits <<= 1;
                        break;
                    case DOWN_RIGHT:
                        ball_colno--;
                        ball_bits >>= 1;
                        break;
                }

                // fix bug if ball goes out of screen
                if ((ball_bits & 0xFF) == 0) {
                    switch (ball_dir) {
                        case UP_LEFT:
                            ball_bits = 0x80;
                            ball_dir = UP_RIGHT;
                            break;
                        case DOWN_LEFT:
                            ball_bits = 0x80;
                            ball_dir = DOWN_RIGHT;
                        case UP_RIGHT:
                            ball_bits = 0x01;
                            ball_dir = UP_LEFT;
                            break;
                        case DOWN_RIGHT:
                            ball_bits = 0x01;
                            ball_dir = DOWN_LEFT;
                    }
                }

                /** Clear pieces bits if ball touches it */
                uint8_t prevCol = matrix_col[ball_colno - 1];
                matrix_col[ball_colno - 1] &= ~(ball_bits);
                if (prevCol != matrix_col[ball_colno - 1]) {
                    // A piece was touched. Switch direction
                    switch (ball_dir) {
                        case UP_STRAIGHT:
                            ball_dir = DOWN_STRAIGHT;
                            break;
                        case UP_LEFT:
                            ball_dir = DOWN_RIGHT;
                            break;
                        case UP_RIGHT:
                            ball_dir = DOWN_LEFT;
                            break;
                    }
                }
                ball_updatecount = 0;
            }

            /** Update Screen */
            ball_blink = !ball_blink; // ball blinks every cycle
            for (uint8_t col = 1; col <= 8; col++) {
                if (col == ball_colno && ball_blink) {
                    MAX7219_Shift2Bytes(col, matrix_col[col-1] | ball_bits);
                } else {
                    MAX7219_Shift2Bytes(col, matrix_col[col-1]);
                }
            }

            _delay_ms(BREAKOUT_REFRESH_DELAY);

            if ((PINA & _BV(PA1)) == 0) {
                int8_t confidence;
                debouncingButton(confidence, (PINA & _BV(PA1)) == 0);
                if (confidence <= 0) {
                    return;
                }
            }
        }

        // Check if win or lose.
        // showText(didWin ? " WIN!" : " LOSE");
        if (didWin) {
            // SMILEY FACE
            showSprite_smileyFace();
        } else {
            // SAD FACE
            showSprite_sadFace();
        }
        _delay_ms(1500);
    }
}

#define MOVEMENT_SPEED 50  // Delay between frames in msec
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

    // Pin 12 push button
    DDRA &= ~(_BV(PA1)); // input
    PORTA |= _BV(PA1); // internal pull up

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
                loopMovingDots(x, y, z);
                break;
            case 4:
                loopParallelLine(x, y, z);
                break;
            case 5:
                loopDoodleJump();
                currentMode = 6;
                continue;
                break;
            case 6:
                loopAtariBreakout();
                currentMode = 0;
                break;
            default:
                previousMode = 0;
                currentMode = 0;
                break;
        }

        _delay_ms(75);

        if ((PINA & _BV(PA1)) == 0) {
            int8_t confidence;
            debouncingButton(confidence, (PINA & _BV(PA1)) == 0);
            /*for (confidence = 12; confidence > 0 && ((PINA & _BV(PA1)) == 0); confidence--) {
                _delay_ms(10);
            }*/
            if (confidence <= 0) {
                if (currentMode >= 5) {
                    currentMode = 0;
                } else {
                    currentMode++;
                }
            }
            _delay_ms(100);
        }
    }
}
