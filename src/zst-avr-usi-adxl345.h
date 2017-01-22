#define _ADXL345_DDR_SPI    DDRA
#define _ADXL345_PORT_SPI   PORTA
#define _ADXL345_PIN_SPI    PINA
#define _ADXL345_DD_DI      PA6 // aka ISP_MOSI
#define _ADXL345_DD_DO      PA5 // aka ISP_MISO
#define _ADXL345_DD_USCK    PA4 // aka ISP_CLK

uint8_t ADXL345_BitBang_SPI_mode3(uint8_t cData);
void ADXL345_USI_SPI_Init(void);

#define ADXL345_SPI_transfer(x)     ADXL345_BitBang_SPI_mode3(x)
#define ADXL345_CS_DDR()            DDRB  |= _BV(PA0);
#define ADXL345_CS_LOW()            PORTB &= ~_BV(PA0);
#define ADXL345_CS_HIGH()           PORTB |=  _BV(PA0);
#include "sparkfun_adxl345_avr.h"



uint8_t ADXL345_BitBang_SPI_mode3(uint8_t data) {
    uint8_t reply = 0;
    for (int8_t i = 7; i >= 0; i--) {
        reply <<= 1;
        _ADXL345_PORT_SPI &= ~_BV(_ADXL345_DD_USCK); // CLOCK LOW
        if (data & _BV(i)) { // Data after first clock low for polarity=1
            _ADXL345_PORT_SPI |= _BV(_ADXL345_DD_DO);
        } else {
            _ADXL345_PORT_SPI &= ~_BV(_ADXL345_DD_DO);
        }
        _ADXL345_PORT_SPI |= _BV(_ADXL345_DD_USCK); // CLOCK HIGH
        if (_ADXL345_PIN_SPI & _BV(_ADXL345_DD_DI)) {
            reply |= 1; // Sample on rising edge
        }
    }
    //LCD_MoveCursor(3, 0);
    //LCD_Hex(reply);
    return reply;
}

void ADXL345_USI_SPI_Init(void) {
    ADXL345_CS_DDR();
    _ADXL345_DDR_SPI |= _BV(_ADXL345_DD_DO) | _BV(_ADXL345_DD_USCK);
    _ADXL345_DDR_SPI &= ~_BV(_ADXL345_DD_DI); // as input (DI) - data in
    //_ADXL345_DDR_SPI |= _BV(_ADXL345_DD_DI); // Pull up internal


    ADXL345_powerOn();                     // Power on the ADXL345

    ADXL345_setRangeSetting(16);           // Give the range settings
    // Accepted values are 2g, 4g, 8g or 16g
    // Higher Values = Wider Measurement Range
    // Lower Values = Greater Sensitivity

    ADXL345_setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
    // Default: Set to 1
    // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library
/*
    ADXL345_setActivityXYZ(1, 1, 1);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
    ADXL345_setActivityThreshold(75);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)

    ADXL345_setInactivityXYZ(1, 1, 1);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
    ADXL345_setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
    ADXL345_setTimeInactivity(5);         // How many seconds of no activity is inactive?
*/
    // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
    ADXL345_setTapDetectionOnXYZ(1, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
    ADXL345_setTapThreshold(75);           // 62.5 mg per increment
    ADXL345_setTapDuration(20);            // 625 μs per increment
    ADXL345_setDoubleTapLatency(100);       // 1.25 ms per increment
    ADXL345_setDoubleTapWindow(200);       // 1.25 ms per increment

/*
    // Set values for what is considered FREE FALL (0-255)
    ADXL345_setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
    ADXL345_setFreeFallDuration(25);       // (20 - 70) recommended - 5ms per increment
*/

    // Setting all interupts to take place on INT1 pin
    //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
    // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
    // This library may have a problem using INT2 pin. Default to INT1 pin.

    // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
    ADXL345_InactivityINT(0);
    ADXL345_ActivityINT(0);
    ADXL345_FreeFallINT(0);
    ADXL345_doubleTapINT(0);
    ADXL345_singleTapINT(1);

    ADXL345_setInterruptMapping(ADXL345_INT_SINGLE_TAP_BIT, ADXL345_INT1_PIN);
}
