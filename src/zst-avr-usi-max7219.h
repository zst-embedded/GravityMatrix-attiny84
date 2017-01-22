#define _MAX7219_DDR_CS DDRB
#define _MAX7219_PORT_CS PORTB
#define _MAX7219_CS PB1

#define _MAX7219_DDR_SPI DDRA
#define _MAX7219_PORT_SPI PORTA
#define _MAX7219_DD_DI PA6       // ISP_MOSI
#define _MAX7219_DD_DO PA5       // ISP_MISO
#define _MAX7219_DD_USCK PA4     // ISP_CLK

#define MAX7219_MODE_DECODE       0x09
#define MAX7219_MODE_INTENSITY    0x0A
#define MAX7219_MODE_SCAN_LIMIT   0x0B
#define MAX7219_MODE_POWER        0x0C
#define MAX7219_MODE_TEST         0x0F
#define MAX7219_MODE_NOOP         0x00

void MAX7219_USI_SPI_Init(void) {
    _MAX7219_DDR_SPI |= _BV(_MAX7219_DD_DO) | _BV(_MAX7219_DD_USCK);
    _MAX7219_DDR_SPI &= ~_BV(_MAX7219_DD_DI); // as input (DI) - data in
    _MAX7219_DDR_CS |= _BV(_MAX7219_CS);
}

uint8_t MAX7219_USI_SPI_Transmit(uint8_t cData) {
    USIDR = cData; // Put data into register
    USISR |= _BV(USIOIF); // Clear Counter Overflow Interrupt Flag flag by writing 1
    // USIOIF: used to determine when a transfer is completed, incremented every USCK pulse

    /* Data transferred MSB first */
    while ( (USISR & _BV(USIOIF)) == 0 ) { // while counter not overflowed, meaning transfer not complete
        USICR = (1 << USIWM0) | // 3 wire mode
                (1 << USICS1) | // Produce an External, positive edge Clock Source on USCK
                (1 << USICLK) | // Software clock strobe (Controlled by USITC)
                (1 << USITC); // Write 1 to toggle/pulse USCK
    }
    USICR = 0; // Disable USI
    return USIDR;
}

void MAX7219_Shift2Bytes(uint8_t address, uint8_t data) {
    _MAX7219_PORT_CS &= ~_BV(_MAX7219_CS); // Active low control select
    MAX7219_USI_SPI_Transmit(address);
    MAX7219_USI_SPI_Transmit(data);
    _MAX7219_PORT_CS |= _BV(_MAX7219_CS); // Inactive high control select
}