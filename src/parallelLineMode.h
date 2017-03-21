
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
