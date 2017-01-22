#include <avr/io.h>
#include <math.h>
#include <stdbool.h>
#include <util/delay.h>
#include "zst-avr-usi-max7219.h"
#include "zst-avr-usi-adxl345.h"
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


#define PI_OVER_180 (0.01745329251) // = 1/180.0 * M_PI
#define DEG180_OVER_PI (57.2957795) // =
#define numb_between(a,b,c) ((a) < (b) && (b) < (c))

void displayLine(int, int);
void calculateAngle(double * angleAround, double * angleTilt, int16_t x, int16_t y, int16_t z);

int main(void) {
    DDRB |= _BV(PB2);
    DDRA |= _BV(PA7);
    DDRA &= ~_BV(PA0);
    PORTA |= _BV(PA0); // internal pull up

    // Setup MAX7219 LED Matrix
    MAX7219_USI_SPI_Init();

    _delay_ms(250);
    MAX7219_Shift2Bytes(MAX7219_MODE_TEST, 1);
    _delay_ms(250);
    MAX7219_Shift2Bytes(MAX7219_MODE_TEST, 0);
    MAX7219_Shift2Bytes(MAX7219_MODE_SCAN_LIMIT, 7);
    MAX7219_Shift2Bytes(MAX7219_MODE_INTENSITY, 0x0);
    MAX7219_Shift2Bytes(MAX7219_MODE_POWER, 0x1);
    MAX7219_Shift2Bytes(MAX7219_MODE_DECODE, 0x0);

    // Setup ADXL345 3 axis accelerometer
    ADXL345_USI_SPI_Init();

    double angle_around, angle_tilt;
    int16_t x, y, z;
    uint8_t interruptSource, brightness = 0x0;
    while (1) {
        // Accelerometer Readings
        ADXL345_readAccel(&x, &y, &z);

        // Calculate angles
        calculateAngle(&angle_around, &angle_tilt, x, y, z);
        if (angle_around == -100) { // don't update as we don't know 0 or 180 deg.
            // do nothing
        } else if (angle_around != -1) {
            if (PINA & _BV(PA0)) { // if switched pulled to ground, then active
                displayLine(angle_around, 4);
            } else {
                displayLine(angle_around, 4.5 + (angle_tilt / 90.0 * 3));
            }
        } else {
            MAX7219_Shift2Bytes(1, 0b10000001);
            MAX7219_Shift2Bytes(2, 0b01000010);
            MAX7219_Shift2Bytes(3, 0b00100100);
            MAX7219_Shift2Bytes(4, 0b00011000);
            MAX7219_Shift2Bytes(5, 0b00011000);
            MAX7219_Shift2Bytes(6, 0b00100100);
            MAX7219_Shift2Bytes(7, 0b01000010);
            MAX7219_Shift2Bytes(8, 0b10000001);
        }

        // ADXL Interrupts
        interruptSource = ADXL345_getInterruptSource();
        if (ADXL345_triggered(interruptSource , ADXL345_SINGLE_TAP)) {
            PORTB |= _BV(PB2);
            if (brightness >= 0xF) {
                brightness = 0;
            } else {
                brightness += 0x5;
            }
            MAX7219_Shift2Bytes(MAX7219_MODE_INTENSITY, brightness);
        } else {
            PORTB &= ~_BV(PB2);
        }

        if (ADXL345_triggered(interruptSource, ADXL345_ACTIVITY)) {
            PORTA |= _BV(PA7);
        } else {
            PORTA &= ~_BV(PA7);
        }
        _delay_ms(10);
    }
}

#define LED_WIDTH (8)
#define LED_WIDTH_HALF (4)

void displayLine(int deg, const int middle_height) {
    //deg is in anticlockwise
    if (numb_between(80, deg, 100)) {
        for (deg = 8; deg > 0; deg--) {
            MAX7219_Shift2Bytes(deg, (middle_height < deg) * 0xFF);
        }
    } else if (numb_between(260, deg, 280)) {
        for (deg = 8; deg > 0; deg--) {
            MAX7219_Shift2Bytes(deg, (middle_height >= deg) * 0xFF);
        }
    } else {
        const bool bool_180 = numb_between(90, deg, 270);
        const double opp_over_adj = tan(deg * PI_OVER_180);
        uint8_t col, fill;
        int8_t adj, offset;
        for (col = 0; col < LED_WIDTH; col++) {
            //adj = (col < LED_WIDTH_HALF) ? (LED_WIDTH_HALF - col) : (LED_WIDTH_HALF - col);
            adj = (LED_WIDTH_HALF - col);
            offset = (int8_t) round(opp_over_adj * adj);

            fill = _BV(middle_height - offset) - 1;
            // If I want position of 5 (0001 1111),
            // then I should use 1<<6 to get 0010 0000.
            // Minus 1 from it to get my position filled.
            MAX7219_Shift2Bytes(col+1, bool_180 ? ~fill : fill);
        }
    }
    deg+=1;
    _delay_ms(15);
}

//http://morf.lv/modules.php?name=tutorials&lasit=31
void calculateAngle(double * angleAround, double * angleTilt, int16_t x, int16_t y, int16_t z) {
    double angle_around = atan(x*1.0/y) * DEG180_OVER_PI;

    if (numb_between(-5, y, 5) && numb_between(-5, x, 5)) { // lying flat on table, show X
        angle_around = -1;
    } else if (y == 0) { // skip, unknown if 0 deg or 180 deg || not connected
        angle_around = -100;
    } else if (y > 0) { // y (+)
        angle_around += 90; // because upper ranger is +/- 90, so it will offset to 0~180
    } else { // y (-)
        angle_around += 270; // because upper ranger is +/- 90, so it will offset to 180~360
    }

    double angle_tilt;

    if (numb_between(90,angle_around,180)) {
        angle_tilt = atan( x * 1.0 / z ) * DEG180_OVER_PI;
        if (x <= 0)
            angle_tilt = -angle_tilt; // fix tilt when it is upside down
    } else {
        angle_tilt = atan( y * 1.0 / z ) * DEG180_OVER_PI;
        if (y <= 0)
            angle_tilt = -angle_tilt; // fix tilt when it is upside down
    }

    if (angle_tilt >= 0) {
        angle_tilt = 90 - angle_tilt;
    } else {
        angle_tilt = -90 - angle_tilt;
    }

    *angleTilt = angle_tilt;
    *angleAround = angle_around;
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