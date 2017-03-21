#pragma once

void loopWaterLevel(bool, int8_t, int8_t, int8_t);
void displayLine(int, int);
void calculateCorrectionAngle(double * angleAround, double * angleTilt, int16_t x, int16_t y, int16_t z);

#define PI_OVER_180 (0.01745329251) // = * PI/180
#define DEG180_OVER_PI (57.2957795) // = * 180/PI
#define numb_between(a,b,c) ((a) < (b) && (b) < (c))
#define RETURN_UNKNOWN_ANGLE (-100)

void loopWaterLevel(bool tiltModeEnabled, int8_t x, int8_t y, int8_t z) {
    double angle_around, angle_tilt;
    // Calculate angles
    calculateCorrectionAngle(&angle_around, tiltModeEnabled ? (&angle_tilt) : 0, x, y, z);
    if (angle_around == RETURN_UNKNOWN_ANGLE) { // don't update as we don't know 0 or 180 deg.
        // do nothing
    } else if (angle_around != -1) {
        if (tiltModeEnabled) {
            displayLine(angle_around + 0.5, 4.5 + (angle_tilt / 90.0 * 3));
            // Add 0.5 for rounding
        } else {
            displayLine(angle_around + 0.5, 4);
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
}


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
void calculateCorrectionAngle(double * angleAround, double * angleTilt, int16_t x, int16_t y, int16_t z) {
    double angle_around = atan(x*1.0/y) * DEG180_OVER_PI;

    if (numb_between(-5, y, 5) && numb_between(-5, x, 5)) { // lying flat on table, show X
        angle_around = -1;
    } else if (y == 0) { // skip, unknown if 0 deg or 180 deg || not connected
        angle_around = RETURN_UNKNOWN_ANGLE;
    } else if (y > 0) { // y (+)
        angle_around += 90; // because upper ranger is +/- 90, so it will offset to 0~180
    } else { // y (-)
        angle_around += 270; // because upper ranger is +/- 90, so it will offset to 180~360
    }
    *angleAround = angle_around;

    if (angleTilt) { // if angleTile is 0, then don't waste CPU time to calculate
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
    }
}