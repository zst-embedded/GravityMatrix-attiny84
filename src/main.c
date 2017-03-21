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

void showText(const char input[]);

uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

#define debouncingButton(confidence, condition) \
for ((confidence) = 12; (confidence) > 0 && (condition); (confidence)--) { \
    _delay_ms(10);\
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

        showText(" 321");
        /*
        for (y = 3; y > 0; y--) {
            for (x = 0; x < 8; x++) {
                uint8_t a = getFont('0'+ y, x);
                MAX7219_Shift2Bytes(8-x, reverse(a));
            }
            _delay_ms(750);
        }*/

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
            //int8_t paddleShift = fabs(rad_angle_tilt / M_PI_4 * LED_WIDTH_HALF);
            // rad_angle_tilt:                         -pi/4 -> shift -1 | +pi/4 -> shift 6
            // rad_angle_tilt/M_PI_4:                  -1 -> shift -1 | +1 -> shift 6
            // ((rad_angle_tilt/M_PI_4)+1):             0 -> shift -1 | 2 -> shift 6
            // ((rad_angle_tilt/M_PI_4)+1)*2.5:         0 -> shift -1 | 5 -> shift 6
            // ((rad_angle_tilt/M_PI_4)+1)*2.5 -1:     -1 -> shift -1 | 6 -> shift 6
            //
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
                        /*

                        if (DOWN_STRAIGHT == ball_dir) {
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
                            }
                        } else {
                            if (DOWN_LEFT == ball_dir) {
                                if (paddleDirectlyBelowBall) {
                                    // if center then direct up, if not then ball bounces sideways
                                    ball_dir = isCenter ? UP_STRAIGHT : UP_RIGHT;
                                } else {
                                    // check edge
                                    ball_bits <<= 1;
                                    bool paddleEdgeHitBall = (ball_bits & matrix_col[0]) != 0;
                                    if (paddleEdgeHitBall) {
                                        ball_dir = UP_RIGHT;
                                    }
                                }
                            } else if (DOWN_RIGHT == ball_dir) {
                                if (paddleDirectlyBelowBall) {
                                    ball_dir = isCenter ? UP_STRAIGHT : UP_LEFT;
                                } else {
                                    // check edge
                                    ball_bits >>= 1;
                                    bool paddleEdgeHitBall = (ball_bits & matrix_col[0]) != 0;
                                    if (paddleEdgeHitBall) {
                                        ball_dir = UP_LEFT;
                                    }
                                }
                            }
                        }*/
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

            if ((PINA & _BV(PA3)) == 0) {
                int8_t confidence;
                debouncingButton(confidence, (PINA & _BV(PA3)) == 0);
                if (confidence <= 0) {
                    return;
                }
            }
        }

        // Check if win or lose.
        // showText(didWin ? " WIN!" : " LOSE");
        if (didWin) {
            // SMILEY FACE
            MAX7219_Shift2Bytes(1, 0x00);
            MAX7219_Shift2Bytes(2, 0x64);
            MAX7219_Shift2Bytes(3, 0x62);
            MAX7219_Shift2Bytes(4, 0x02);
            MAX7219_Shift2Bytes(5, 0x02);
            MAX7219_Shift2Bytes(6, 0x62);
            MAX7219_Shift2Bytes(7, 0x64);
            MAX7219_Shift2Bytes(8, 0x00);
        } else {
            // SAD FACE
            MAX7219_Shift2Bytes(1, 0x00);
            MAX7219_Shift2Bytes(2, 0x22);
            MAX7219_Shift2Bytes(3, 0x64);
            MAX7219_Shift2Bytes(4, 0x04);
            MAX7219_Shift2Bytes(5, 0x04);
            MAX7219_Shift2Bytes(6, 0x64);
            MAX7219_Shift2Bytes(7, 0x22);
            MAX7219_Shift2Bytes(8, 0x00);
        }
        _delay_ms(1500);
    }
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
            case 4:
                loopAtariBreakout();
                currentMode = 0;
                break;
            default:
                previousMode = 0;
                currentMode = 0;
                break;
        }

        _delay_ms(75);

        if ((PINA & _BV(PA3)) == 0) {
            int8_t confidence;
            debouncingButton(confidence, (PINA & _BV(PA3)) == 0);
            /*for (confidence = 12; confidence > 0 && ((PINA & _BV(PA3)) == 0); confidence--) {
                _delay_ms(10);
            }*/
            if (confidence <= 0) {
                if (currentMode >= 4) {
                    currentMode = 0;
                } else {
                    currentMode++;
                }
            }
            _delay_ms(100);
        }

    }
}
