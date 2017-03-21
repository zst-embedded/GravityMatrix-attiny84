#pragma once

void loopMovingDots(int8_t x, int8_t y, int8_t z) {
    double rad_angle_tilt_vert = atan( x * 1.0 / z ); // x axis is pointing down
    double rad_angle_tilt_hori = atan( y * 1.0 / z ); // y axis is pointing right

    uint8_t vert_center_col = 0b00011000;
    int8_t vert_shift = fmin(4, fabs(rad_angle_tilt_vert / M_PI_4 * LED_HEIGHT_HALF) ); // -> angle/45deg * 4LEDs

    if (0 == vert_shift) {
        // do nothing
    } else if (rad_angle_tilt_vert > 0) {
        vert_center_col <<= vert_shift;
    } else { // if (rad_angle_tilt_vert < 0) {
        vert_center_col >>= vert_shift;
    }

    int8_t hori_shift = rad_angle_tilt_hori / M_PI_4 * LED_WIDTH_HALF;
    int8_t hori_col = hori_shift + 4.5;

    // hori_shift will be between -45deg (-4shifts) ~ 0deg (0 shifts) ~ 45deg (4 shifts)
    // hori_col will be 0.5 shifts (col 0, 1) ~ 4.5 shifts (col 4, 5) ~ 8.5 shifts (col 8, 9)
    // hori_col is floored (casted to int) and result, result+1 is used as the columns
    // col 0 and 9 are intentionally outside the 8x8 matrix.

    /* If shifts are below 0 or above 8, then fix it */
    if (hori_col <= 0)
        hori_col = 0;
    else if (hori_col >= 8)
        hori_col = 8;

    for (uint8_t col = 1; col <= 8; col++) {
        if (col == hori_col || col == (hori_col+1)) {
            MAX7219_Shift2Bytes(col, vert_center_col);
        } else {
            MAX7219_Shift2Bytes(col, 0);
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
