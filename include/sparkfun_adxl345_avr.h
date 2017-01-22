/*
Modified by zst for AVR GCC
Removed I2C support.
*/


/*
Sparkfun's ADXL345 Library Main Source File
SparkFun_ADXL345.cpp

E.Robert @ SparkFun Electronics
Created: Jul 13, 2016
Updated: Sep 06, 2016

Modified Bildr ADXL345 Source File @ http://code.bildr.org/download/959.zip
to support both I2C and SPI Communication

Hardware Resources:
- Arduino Development Board
- SparkFun Triple Access Accelerometer ADXL345

Development Environment Specifics:
Arduino 1.6.8
SparkFun Triple Axis Accelerometer Breakout - ADXL345
Arduino Uno
*/


/*************************** REGISTER MAP ***************************/
#define ADXL345_DEVID			0x00		// Device ID
#define ADXL345_RESERVED1		0x01		// Reserved. Do Not Access.
#define ADXL345_THRESH_TAP		0x1D		// Tap Threshold.
#define ADXL345_OFSX			0x1E		// X-Axis Offset.
#define ADXL345_OFSY			0x1F		// Y-Axis Offset.
#define ADXL345_OFSZ			0x20		// Z- Axis Offset.
#define ADXL345_DUR				0x21		// Tap Duration.
#define ADXL345_LATENT			0x22		// Tap Latency.
#define ADXL345_WINDOW			0x23		// Tap Window.
#define ADXL345_THRESH_ACT		0x24		// Activity Threshold
#define ADXL345_THRESH_INACT	0x25		// Inactivity Threshold
#define ADXL345_TIME_INACT		0x26		// Inactivity Time
#define ADXL345_ACT_INACT_CTL	0x27		// Axis Enable Control for Activity and Inactivity Detection
#define ADXL345_THRESH_FF		0x28		// Free-Fall Threshold.
#define ADXL345_TIME_FF			0x29		// Free-Fall Time.
#define ADXL345_TAP_AXES		0x2A		// Axis Control for Tap/Double Tap.
#define ADXL345_ACT_TAP_STATUS	0x2B		// Source of Tap/Double Tap
#define ADXL345_BW_RATE			0x2C		// Data Rate and Power mode Control
#define ADXL345_POWER_CTL		0x2D		// Power-Saving Features Control
#define ADXL345_INT_ENABLE		0x2E		// Interrupt Enable Control
#define ADXL345_INT_MAP			0x2F		// Interrupt Mapping Control
#define ADXL345_INT_SOURCE		0x30		// Source of Interrupts
#define ADXL345_DATA_FORMAT		0x31		// Data Format Control
#define ADXL345_DATAX0			0x32		// X-Axis Data 0
#define ADXL345_DATAX1			0x33		// X-Axis Data 1
#define ADXL345_DATAY0			0x34		// Y-Axis Data 0
#define ADXL345_DATAY1			0x35		// Y-Axis Data 1
#define ADXL345_DATAZ0			0x36		// Z-Axis Data 0
#define ADXL345_DATAZ1			0x37		// Z-Axis Data 1
#define ADXL345_FIFO_CTL		0x38		// FIFO Control
#define ADXL345_FIFO_STATUS		0x39		// FIFO Status

#define ADXL345_BW_1600			0xF			// 1111		IDD = 40uA
#define ADXL345_BW_800			0xE			// 1110		IDD = 90uA
#define ADXL345_BW_400			0xD			// 1101		IDD = 140uA
#define ADXL345_BW_200			0xC			// 1100		IDD = 140uA
#define ADXL345_BW_100			0xB			// 1011		IDD = 140uA
#define ADXL345_BW_50			0xA			// 1010		IDD = 140uA
#define ADXL345_BW_25			0x9			// 1001		IDD = 90uA
#define ADXL345_BW_12_5		    0x8			// 1000		IDD = 60uA
#define ADXL345_BW_6_25			0x7			// 0111		IDD = 50uA
#define ADXL345_BW_3_13			0x6			// 0110		IDD = 45uA
#define ADXL345_BW_1_56			0x5			// 0101		IDD = 40uA
#define ADXL345_BW_0_78			0x4			// 0100		IDD = 34uA
#define ADXL345_BW_0_39			0x3			// 0011		IDD = 23uA
#define ADXL345_BW_0_20			0x2			// 0010		IDD = 23uA
#define ADXL345_BW_0_10			0x1			// 0001		IDD = 23uA
#define ADXL345_BW_0_05			0x0			// 0000		IDD = 23uA


/************************** INTERRUPT PINS **************************/
#define ADXL345_INT1_PIN		0x00		//INT1: 0
#define ADXL345_INT2_PIN		0x01		//INT2: 1


/********************** INTERRUPT BIT POSITION **********************/
#define ADXL345_INT_DATA_READY_BIT		0x07
#define ADXL345_INT_SINGLE_TAP_BIT		0x06
#define ADXL345_INT_DOUBLE_TAP_BIT		0x05
#define ADXL345_INT_ACTIVITY_BIT		0x04
#define ADXL345_INT_INACTIVITY_BIT		0x03
#define ADXL345_INT_FREE_FALL_BIT		0x02
#define ADXL345_INT_WATERMARK_BIT		0x01
#define ADXL345_INT_OVERRUNY_BIT		0x00

#define ADXL345_DATA_READY				0x07
#define ADXL345_SINGLE_TAP				0x06
#define ADXL345_DOUBLE_TAP				0x05
#define ADXL345_ACTIVITY				0x04
#define ADXL345_INACTIVITY				0x03
#define ADXL345_FREE_FALL				0x02
#define ADXL345_WATERMARK				0x01
#define ADXL345_OVERRUNY				0x00


/****************************** ERRORS ******************************/
#define ADXL345_OK			1		// No Error
#define ADXL345_ERROR		0		// Error Exists

#define ADXL345_NO_ERROR	0		// Initial State
#define ADXL345_READ_ERROR	1		// Accelerometer Reading Error
#define ADXL345_BAD_ARG		2		// Bad Argument


bool status;                    // Set When Error Exists
uint8_t error_code;                // Initial State
double gains[3];                // Counts to Gs

uint8_t _buff[6] ;		//	6 Bytes Buffer

#define ADXL345_DEVICE (0x53)    // Device Address for ADXL345
#define ADXL345_TO_READ (6)      // Number of Bytes Read - Two Bytes Per Axis

#ifndef ADXL345_CS_LOW
#error "MUST DEFINE CS LOW AND HIGH"
#endif
#ifndef ADXL345_CS_HIGH
#error "MUST DEFINE CS LOW AND HIGH"
#endif

bool ADXL345_getRegisterBit(uint8_t regAdress, uint8_t bitPos);

void ADXL345_setup() {
    status = ADXL345_OK;
    error_code = ADXL345_NO_ERROR;

    gains[0] = 0.00376390;
    gains[1] = 0.00376009;
    gains[2] = 0.00349265;

    _CS_HIGH();
}

void ADXL345_powerOn() {
    //ADXL345 TURN ON
    ADXL345_writeTo(ADXL345_POWER_CTL, 0);    // Wakeup
    ADXL345_writeTo(ADXL345_POWER_CTL, 16);    // Auto_Sleep
    ADXL345_writeTo(ADXL345_POWER_CTL, 8);    // Measure
}


/*********************** READING ACCELERATION ***********************/
/*    Reads Acceleration into Three Variables:  x, y and z          */


void ADXL345_readAccelArray(int *xyz) {
    ADXL345_readAccel(xyz, xyz + 1, xyz + 2);
}

void ADXL345_readAccel(int *x, int *y, int *z) {
    ADXL345_readFrom(ADXL345_DATAX0, ADXL345_TO_READ, _buff);    // Read Accel Data from ADXL345

    // Each Axis @ All g Ranges: 10 Bit Resolution (2 Bytes)
    *x = (int16_t)((((uint16_t) _buff[1]) << 8) | _buff[0]);
    *y = (int16_t)((((uint16_t) _buff[3]) << 8) | _buff[2]);
    *z = (int16_t)((((uint16_t) _buff[5]) << 8) | _buff[4]);
}

void ADXL345_get_Gxyz(double *xyz) {
    int xyz_int[3];
    ADXL345_readAccelArray(xyz_int);
    for (uint8_t i = 0; i < 3; i++) {
        xyz[i] = xyz_int[i] * gains[i];
    }
}

/***************** WRITES VALUE TO ADDRESS REGISTER *****************/
inline void ADXL345_writeTo(uint8_t address, uint8_t val) {
    ADXL345_writeToSPI(address, val);
}

/************************ READING NUM BYTES *************************/
/*    Reads Num Bytes. Starts from Address Reg to _buff Array        */
void ADXL345_readFrom(uint8_t address, uint8_t num, uint8_t _buff[]) {
    ADXL345_readFromSPI(address, num, _buff);    // If SPI Communication
}

/************************** WRITE FROM SPI **************************/
/*         Point to Destination; Write Value; Turn Off              */
void ADXL345_writeToSPI(uint8_t __reg_address, uint8_t __val) {
    ADXL345_CS_LOW();
    ADXL345_SPI_transfer(__reg_address);
    ADXL345_SPI_transfer(__val);
    ADXL345_CS_HIGH();
}

/*************************** READ FROM SPI **************************/
/*                                                                  */
void ADXL345_readFromSPI(uint8_t __reg_address, uint8_t num, uint8_t _buff[]) {
    // Read: Most Sig Bit of Reg Address Set
    char _address = 0x80 | __reg_address;
    // If Multi-Byte Read: Bit 6 Set
    if (num > 1) {
        _address = _address | 0x40;
    }

    ADXL345_CS_LOW();
    ADXL345_SPI_transfer(_address);        // Transfer Starting Reg Address To Be Read
    for (int i = 0; i < num; i++) {
        _buff[i] = ADXL345_SPI_transfer(0x00);
    }
    ADXL345_CS_HIGH();
}

/*************************** RANGE SETTING **************************/
/*          ACCEPTABLE VALUES: 2g, 4g, 8g, 16g ~ GET & SET          */
void ADXL345_getRangeSetting(uint8_t *rangeSetting) {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_DATA_FORMAT, 1, &_b);
    *rangeSetting = _b & 0b00000011;
}

void ADXL345_setRangeSetting(uint8_t val) {
    uint8_t _s;
    uint8_t _b;

    switch (val) {
        case 2:
            _s = 0x00;
            break;
        case 4:
            _s = 0x01;
            break;
        case 8:
            _s = 0x02;
            break;
        case 16:
            _s = 0x03;
            break;
        default:
            _s = 0x00;
    }
    ADXL345_readFrom(ADXL345_DATA_FORMAT, 1, &_b);
    _s |= (_b & 0b11101100);
    ADXL345_writeTo(ADXL345_DATA_FORMAT, _s);
}

/*************************** SELF_TEST BIT **************************/
/*                            ~ GET & SET                           */
bool ADXL345_getSelfTestBit() {
    return ADXL345_getRegisterBit(ADXL345_DATA_FORMAT, 7);
}

// If Set (1) Self-Test Applied. Electrostatic Force exerted on the sensor
//  causing a shift in the output data.
// If Set (0) Self-Test Disabled.
void ADXL345_setSelfTestBit(bool selfTestBit) {
    ADXL345_setRegisterBit(ADXL345_DATA_FORMAT, 7, selfTestBit);
}

/*************************** SPI BIT STATE **************************/
/*                           ~ GET & SET                            */
bool ADXL345_getSpiBit() {
    return ADXL345_getRegisterBit(ADXL345_DATA_FORMAT, 6);
}

// If Set (1) Puts Device in 3-wire Mode
// If Set (0) Puts Device in 4-wire SPI Mode
void ADXL345_setSpiBit(bool spiBit) {
    ADXL345_setRegisterBit(ADXL345_DATA_FORMAT, 6, spiBit);
}

/*********************** INT_INVERT BIT STATE ***********************/
/*                           ~ GET & SET                            */
bool ADXL345_getInterruptLevelBit() {
    return ADXL345_getRegisterBit(ADXL345_DATA_FORMAT, 5);
}

// If Set (0) Sets the Interrupts to Active HIGH
// If Set (1) Sets the Interrupts to Active LOW
void ADXL345_setInterruptLevelBit(bool interruptLevelBit) {
    ADXL345_setRegisterBit(ADXL345_DATA_FORMAT, 5, interruptLevelBit);
}

/************************* FULL_RES BIT STATE ***********************/
/*                           ~ GET & SET                            */
bool ADXL345_getFullResBit() {
    return ADXL345_getRegisterBit(ADXL345_DATA_FORMAT, 3);
}

// If Set (1) Device is in Full Resolution Mode: Output Resolution Increase with G Range
//  Set by the Range Bits to Maintain a 4mg/LSB Scale Factor
// If Set (0) Device is in 10-bit Mode: Range Bits Determine Maximum G Range
//  And Scale Factor
void ADXL345_setFullResBit(bool fullResBit) {
    ADXL345_setRegisterBit(ADXL345_DATA_FORMAT, 3, fullResBit);
}

/*************************** JUSTIFY BIT STATE **************************/
/*                           ~ GET & SET                            */
bool ADXL345_getJustifyBit() {
    return ADXL345_getRegisterBit(ADXL345_DATA_FORMAT, 2);
}

// If Set (1) Selects the Left Justified Mode
// If Set (0) Selects Right Justified Mode with Sign Extension
void ADXL345_setJustifyBit(bool justifyBit) {
    ADXL345_setRegisterBit(ADXL345_DATA_FORMAT, 2, justifyBit);
}

/*********************** THRESH_TAP BYTE VALUE **********************/
/*                          ~ SET & GET                             */
// Should Set Between 0 and 255
// Scale Factor is 62.5 mg/LSB
// A Value of 0 May Result in Undesirable Behavior
void ADXL345_setTapThreshold(uint8_t tapThreshold) {
    ADXL345_writeTo(ADXL345_THRESH_TAP, tapThreshold);
}

// Return Value Between 0 and 255
// Scale Factor is 62.5 mg/LSB
uint8_t ADXL345_getTapThreshold() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_THRESH_TAP, 1, &_b);
    return _b;
}

/****************** GAIN FOR EACH AXIS IN Gs / COUNT *****************/
/*                           ~ SET & GET                            */
void ADXL345_setAxisGains(double *_gains) {
    for (uint8_t i = 0; i < 3; i++) {
        gains[i] = _gains[i];
    }
}

void ADXL345_getAxisGains(double *_gains) {
    for (uint8_t i = 0; i < 3; i++) {
        _gains[i] = gains[i];
    }
}

/********************* OFSX, OFSY and OFSZ BYTES ********************/
/*                           ~ SET & GET                            */
// OFSX, OFSY and OFSZ: User Offset Adjustments in Twos Complement Format
// Scale Factor of 15.6mg/LSB
void ADXL345_setAxisOffset(uint8_t x, uint8_t y, uint8_t z) {
    ADXL345_writeTo(ADXL345_OFSX, x);
    ADXL345_writeTo(ADXL345_OFSY, y);
    ADXL345_writeTo(ADXL345_OFSZ, z);
}

void ADXL345_getAxisOffset(uint8_t *x, uint8_t *y, uint8_t *z) {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_OFSX, 1, &_b);
    *x = _b;
    ADXL345_readFrom(ADXL345_OFSY, 1, &_b);
    *y = _b;
    ADXL345_readFrom(ADXL345_OFSZ, 1, &_b);
    *z = _b;
}

/****************************** DUR BYTE ****************************/
/*                           ~ SET & GET                            */
// DUR Byte: Contains an Unsigned Time Value Representing the Max Time 
//  that an Event must be Above the THRESH_TAP Threshold to qualify 
//  as a Tap Event
// The scale factor is 625µs/LSB
// Value of 0 Disables the Tap/Double Tap Funcitons. Max value is 255.
void ADXL345_setTapDuration(uint8_t tapDuration) {
    ADXL345_writeTo(ADXL345_DUR, tapDuration);
}

uint8_t ADXL345_getTapDuration() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_DUR, 1, &_b);
    return _b;
}

/************************** LATENT REGISTER *************************/
/*                           ~ SET & GET                            */
// Contains Unsigned Time Value Representing the Wait Time from the Detection
//  of a Tap Event to the Start of the Time Window (defined by the Window 
//  Register) during which a possible Second Tap Even can be Detected.
// Scale Factor is 1.25ms/LSB. 
// A Value of 0 Disables the Double Tap Function.
// It Accepts a Maximum Value of 255.
void ADXL345_setDoubleTapLatency(uint8_t doubleTapLatency) {
    ADXL345_writeTo(ADXL345_LATENT, doubleTapLatency);
}

uint8_t ADXL345_getDoubleTapLatency() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_LATENT, 1, &_b);
    return _b;
}

/************************** WINDOW REGISTER *************************/
/*                           ~ SET & GET                            */
// Contains an Unsigned Time Value Representing the Amount of Time 
//  After the Expiration of the Latency Time (determined by Latent register)
//  During which a Second Valid Tape can Begin. 
// Scale Factor is 1.25ms/LSB. 
// Value of 0 Disables the Double Tap Function. 
// It Accepts a Maximum Value of 255.
void ADXL345_setDoubleTapWindow(uint8_t doubleTapWindow) {
    ADXL345_writeTo(ADXL345_WINDOW, doubleTapWindow);
}

uint8_t ADXL345_getDoubleTapWindow() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_WINDOW, 1, &_b);
    return _b;
}

/*********************** THRESH_ACT REGISTER ************************/
/*                          ~ SET & GET                             */
// Holds the Threshold Value for Detecting Activity.
// Data Format is Unsigned, so the Magnitude of the Activity Event is Compared 
//  with the Value is Compared with the Value in the THRESH_ACT Register. 
// The Scale Factor is 62.5mg/LSB. 
// Value of 0 may Result in Undesirable Behavior if the Activity Interrupt Enabled. 
// It Accepts a Maximum Value of 255.
void ADXL345_setActivityThreshold(uint8_t activityThreshold) {
    ADXL345_writeTo(ADXL345_THRESH_ACT, activityThreshold);
}

// Gets the THRESH_ACT byte
uint8_t ADXL345_getActivityThreshold() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_THRESH_ACT, 1, &_b);
    return _b;
}

/********************** THRESH_INACT REGISTER ***********************/
/*                          ~ SET & GET                             */
// Holds the Threshold Value for Detecting Inactivity.
// The Data Format is Unsigned, so the Magnitude of the INactivity Event is 
//  Compared with the value in the THRESH_INACT Register. 
// Scale Factor is 62.5mg/LSB. 
// Value of 0 May Result in Undesirable Behavior if the Inactivity Interrupt Enabled. 
// It Accepts a Maximum Value of 255.
void ADXL345_setInactivityThreshold(uint8_t inactivityThreshold) {
    ADXL345_writeTo(ADXL345_THRESH_INACT, inactivityThreshold);
}

uint8_t ADXL345_getInactivityThreshold() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_THRESH_INACT, 1, &_b);
    return _b;
}

/*********************** TIME_INACT RESIGER *************************/
/*                          ~ SET & GET                             */
// Contains an Unsigned Time Value Representing the Amount of Time that
//  Acceleration must be Less Than the Value in the THRESH_INACT Register
//  for Inactivity to be Declared. 
// Uses Filtered Output Data* unlike other Interrupt Functions
// Scale Factor is 1sec/LSB. 
// Value Must Be Between 0 and 255. 
void ADXL345_setTimeInactivity(uint8_t timeInactivity) {
    ADXL345_writeTo(ADXL345_TIME_INACT, timeInactivity);
}

uint8_t ADXL345_getTimeInactivity() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_TIME_INACT, 1, &_b);
    return _b;
}

/*********************** THRESH_FF Register *************************/
/*                          ~ SET & GET                             */
// Holds the Threshold Value, in Unsigned Format, for Free-Fall Detection
// The Acceleration on all Axes is Compared with the Value in THRES_FF to
//  Determine if a Free-Fall Event Occurred. 
// Scale Factor is 62.5mg/LSB. 
// Value of 0 May Result in Undesirable Behavior if the Free-Fall interrupt Enabled.
// Accepts a Maximum Value of 255.
void ADXL345_setFreeFallThreshold(uint8_t freeFallThreshold) {
    ADXL345_writeTo(ADXL345_THRESH_FF, freeFallThreshold);
}

uint8_t ADXL345_getFreeFallThreshold() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_THRESH_FF, 1, &_b);
    return _b;
}

/************************ TIME_FF Register **************************/
/*                          ~ SET & GET                             */
// Stores an Unsigned Time Value Representing the Minimum Time that the Value 
//  of all Axes must be Less Than THRES_FF to Generate a Free-Fall Interrupt.
// Scale Factor is 5ms/LSB. 
// Value of 0 May Result in Undesirable Behavior if the Free-Fall Interrupt Enabled.
// Accepts a Maximum Value of 255.
void ADXL345_setFreeFallDuration(uint8_t freeFallDuration) {
    ADXL345_writeTo(ADXL345_TIME_FF, freeFallDuration);
}

uint8_t ADXL345_getFreeFallDuration() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_TIME_FF, 1, &_b);
    return _b;
}

/************************** ACTIVITY BITS ***************************/
/*                                                                  */
bool ADXL345_isActivityXEnabled() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 6);
}

bool ADXL345_isActivityYEnabled() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 5);
}

bool ADXL345_isActivityZEnabled() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 4);
}

bool ADXL345_isInactivityXEnabled() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 2);
}

bool ADXL345_isInactivityYEnabled() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 1);
}

bool ADXL345_isInactivityZEnabled() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 0);
}

void ADXL345_setActivityX(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 6, state);
}

void ADXL345_setActivityY(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 5, state);
}

void ADXL345_setActivityZ(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 4, state);
}

void ADXL345_setActivityXYZ(bool stateX, bool stateY, bool stateZ) {
    ADXL345_setActivityX(stateX);
    ADXL345_setActivityY(stateY);
    ADXL345_setActivityZ(stateZ);
}

void ADXL345_setInactivityX(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 2, state);
}

void ADXL345_setInactivityY(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 1, state);
}

void ADXL345_setInactivityZ(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 0, state);
}

void ADXL345_setInactivityXYZ(bool stateX, bool stateY, bool stateZ) {
    ADXL345_setInactivityX(stateX);
    ADXL345_setInactivityY(stateY);
    ADXL345_setInactivityZ(stateZ);
}

bool ADXL345_isActivityAc() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 7);
}

bool ADXL345_isInactivityAc() {
    return ADXL345_getRegisterBit(ADXL345_ACT_INACT_CTL, 3);
}

void ADXL345_setActivityAc(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 7, state);
}

void ADXL345_setInactivityAc(bool state) {
    ADXL345_setRegisterBit(ADXL345_ACT_INACT_CTL, 3, state);
}

/************************* SUPPRESS BITS ****************************/
/*                                                                  */
bool ADXL345_getSuppressBit() {
    return ADXL345_getRegisterBit(ADXL345_TAP_AXES, 3);
}

void ADXL345_setSuppressBit(bool state) {
    ADXL345_setRegisterBit(ADXL345_TAP_AXES, 3, state);
}

/**************************** TAP BITS ******************************/
/*                                                                  */
bool ADXL345_isTapDetectionOnX() {
    return ADXL345_getRegisterBit(ADXL345_TAP_AXES, 2);
}

void ADXL345_setTapDetectionOnX(bool state) {
    ADXL345_setRegisterBit(ADXL345_TAP_AXES, 2, state);
}

bool ADXL345_isTapDetectionOnY() {
    return ADXL345_getRegisterBit(ADXL345_TAP_AXES, 1);
}

void ADXL345_setTapDetectionOnY(bool state) {
    ADXL345_setRegisterBit(ADXL345_TAP_AXES, 1, state);
}

bool ADXL345_isTapDetectionOnZ() {
    return ADXL345_getRegisterBit(ADXL345_TAP_AXES, 0);
}

void ADXL345_setTapDetectionOnZ(bool state) {
    ADXL345_setRegisterBit(ADXL345_TAP_AXES, 0, state);
}

void ADXL345_setTapDetectionOnXYZ(bool stateX, bool stateY, bool stateZ) {
    ADXL345_setTapDetectionOnX(stateX);
    ADXL345_setTapDetectionOnY(stateY);
    ADXL345_setTapDetectionOnZ(stateZ);
}

bool ADXL345_isActivitySourceOnX() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 6);
}

bool ADXL345_isActivitySourceOnY() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 5);
}

bool ADXL345_isActivitySourceOnZ() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 4);
}

bool ADXL345_isTapSourceOnX() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 2);
}

bool ADXL345_isTapSourceOnY() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 1);
}

bool ADXL345_isTapSourceOnZ() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 0);
}

/*************************** ASLEEP BIT *****************************/
/*                                                                  */
bool ADXL345_isAsleep() {
    return ADXL345_getRegisterBit(ADXL345_ACT_TAP_STATUS, 3);
}

/************************** LOW POWER BIT ***************************/
/*                                                                  */
bool ADXL345_isLowPower() {
    return ADXL345_getRegisterBit(ADXL345_BW_RATE, 4);
}

void ADXL345_setLowPower(bool state) {
    ADXL345_setRegisterBit(ADXL345_BW_RATE, 4, state);
}

/*************************** RATE BITS ******************************/
/*                                                                  */
double ADXL345_getRate() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_BW_RATE, 1, &_b);
    _b &= 0x0F;
    return (pow(2, ((int) _b) - 6)) * 6.25;
}

void ADXL345_setRate(double rate) {
    uint8_t _b, _s;
    int v = (int) (rate / 6.25);
    int r = 0;
    while (v >>= 1) {
        r++;
    }
    if (r <= 9) {
        ADXL345_readFrom(ADXL345_BW_RATE, 1, &_b);
        _s = (uint8_t) (r + 6) | (_b & 0xF0);
        ADXL345_writeTo(ADXL345_BW_RATE, _s);
    }
}

/*************************** BANDWIDTH ******************************/
/*                          ~ SET & GET                             */
void ADXL345_set_bw(uint8_t bw_code) {
    if ((bw_code < ADXL345_BW_0_05) || (bw_code > ADXL345_BW_1600)) {
        status = false;
        error_code = ADXL345_BAD_ARG;
    } else {
        ADXL345_writeTo(ADXL345_BW_RATE, bw_code);
    }
}

uint8_t ADXL345_get_bw_code() {
    uint8_t bw_code;
    ADXL345_readFrom(ADXL345_BW_RATE, 1, &bw_code);
    return bw_code;
}




/************************* TRIGGER CHECK  ***************************/
/*                                                                  */
// Check if Action was Triggered in Interrupts
// Example triggered(interrupts, ADXL345_SINGLE_TAP);
inline bool ADXL345_triggered(uint8_t interrupts, uint8_t mask) {
    return ((interrupts >> mask) & 1);
}

/*
 ADXL345_DATA_READY
 ADXL345_SINGLE_TAP
 ADXL345_DOUBLE_TAP
 ADXL345_ACTIVITY
 ADXL345_INACTIVITY
 ADXL345_FREE_FALL
 ADXL345_WATERMARK
 ADXL345_OVERRUNY
 */


uint8_t ADXL345_getInterruptSource() {
    uint8_t _b;
    ADXL345_readFrom(ADXL345_INT_SOURCE, 1, &_b);
    return _b;
}

bool ADXL345_getInterruptSourceFromBit(uint8_t interruptBit) {
    return ADXL345_getRegisterBit(ADXL345_INT_SOURCE, interruptBit);
}

bool ADXL345_getInterruptMapping(uint8_t interruptBit) {
    return ADXL345_getRegisterBit(ADXL345_INT_MAP, interruptBit) == 1;
}

/*********************** INTERRUPT MAPPING **************************/
/*         Set the Mapping of an Interrupt to pin1 or pin2          */
// eg: setInterruptMapping(ADXL345_INT_DOUBLE_TAP_BIT,ADXL345_INT2_PIN);
void ADXL345_setInterruptMapping(uint8_t interruptBit, bool interruptPin) {
    ADXL345_setRegisterBit(ADXL345_INT_MAP, interruptBit, interruptPin);
}

void ADXL345_setImportantInterruptMapping(uint8_t single_tap, uint8_t double_tap,
                                          uint8_t free_fall, uint8_t activity, uint8_t inactivity) {
    if (single_tap == 1) {
        ADXL345_setInterruptMapping(ADXL345_INT_SINGLE_TAP_BIT, ADXL345_INT1_PIN);
    } else if (single_tap == 2) {
        ADXL345_setInterruptMapping(ADXL345_INT_SINGLE_TAP_BIT, ADXL345_INT2_PIN);
    }

    if (double_tap == 1) {
        ADXL345_setInterruptMapping(ADXL345_INT_DOUBLE_TAP_BIT, ADXL345_INT1_PIN);
    } else if (double_tap == 2) {
        ADXL345_setInterruptMapping(ADXL345_INT_DOUBLE_TAP_BIT, ADXL345_INT2_PIN);
    }

    if (free_fall == 1) {
        ADXL345_setInterruptMapping(ADXL345_INT_FREE_FALL_BIT, ADXL345_INT1_PIN);
    } else if (free_fall == 2) {
        ADXL345_setInterruptMapping(ADXL345_INT_FREE_FALL_BIT, ADXL345_INT2_PIN);
    }

    if (activity == 1) {
        ADXL345_setInterruptMapping(ADXL345_INT_ACTIVITY_BIT, ADXL345_INT1_PIN);
    } else if (activity == 2) {
        ADXL345_setInterruptMapping(ADXL345_INT_ACTIVITY_BIT, ADXL345_INT2_PIN);
    }

    if (inactivity == 1) {
        ADXL345_setInterruptMapping(ADXL345_INT_INACTIVITY_BIT, ADXL345_INT1_PIN);
    } else if (inactivity == 2) {
        ADXL345_setInterruptMapping(ADXL345_INT_INACTIVITY_BIT, ADXL345_INT2_PIN);
    }
}

bool ADXL345_isInterruptEnabled(uint8_t interruptBit) {
    return ADXL345_getRegisterBit(ADXL345_INT_ENABLE, interruptBit);
}

void ADXL345_setInterrupt(uint8_t interruptBit, bool state) {
    ADXL345_setRegisterBit(ADXL345_INT_ENABLE, interruptBit, state);
}

void ADXL345_singleTapINT(bool status) {
    ADXL345_setInterrupt(ADXL345_INT_SINGLE_TAP_BIT, status ? 1 : 0);
}

void ADXL345_doubleTapINT(bool status) {
    ADXL345_setInterrupt(ADXL345_INT_DOUBLE_TAP_BIT, status ? 1 : 0);
}

void ADXL345_FreeFallINT(bool status) {
    ADXL345_setInterrupt(ADXL345_INT_FREE_FALL_BIT, status ? 1 : 0);
}

void ADXL345_ActivityINT(bool status) {
    ADXL345_setInterrupt(ADXL345_INT_ACTIVITY_BIT, status ? 1 : 0);
}

void ADXL345_InactivityINT(bool status) {
    ADXL345_setInterrupt(ADXL345_INT_INACTIVITY_BIT, status ? 1 : 0);
}

void ADXL345_setRegisterBit(uint8_t regAdress, uint8_t bitPos, bool state) {
    uint8_t _b;
    ADXL345_readFrom(regAdress, 1, &_b);
    if (state) {
        _b |= (1 << bitPos);  // Forces nth Bit of _b to 1. Other Bits Unchanged.
    } else {
        _b &= ~(1 << bitPos); // Forces nth Bit of _b to 0. Other Bits Unchanged.
    }
    ADXL345_writeTo(regAdress, _b);
}

bool ADXL345_getRegisterBit(uint8_t regAdress, uint8_t bitPos) {
    uint8_t _b;
    ADXL345_readFrom(regAdress, 1, &_b);
    return ((_b >> bitPos) & 1);
}

/********************************************************************/
/*                                                                  */
// Print Register Values to Serial Output =
// Can be used to Manually Check the Current Configuration of Device
/*void ADXL345_printAllRegister() {
    byte _b;
    Serial.print("0x00: ");
    readFrom(0x00, 1, &_b);
    print_byte(_b);
    Serial.println("");
    int i;
    for (i = 29; i <= 57; i++) {
        Serial.print("0x");
        Serial.print(i, HEX);
        Serial.print(": ");
        readFrom(i, 1, &_b);
        print_byte(_b);
        Serial.println("");
    }
}*/
/*
void print_byte(byte val) {
    int i;
    Serial.print("B");
    for (i = 7; i >= 0; i--) {
        Serial.print(val >> i & 1, BIN);
    }
}*/