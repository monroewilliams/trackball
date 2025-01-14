#pragma once

// LED (N/A)
#define PIN_LED        (31u)

// NeoPixel
#define PIN_NEOPIXEL   (11u)
// NeoPixel is always powered on this board.
// #define NEOPIXEL_POWER (31u)

// Serial (N/A)
#define PIN_SERIAL1_TX (31u)
#define PIN_SERIAL1_RX (31u)
#define PIN_SERIAL2_TX (31u)
#define PIN_SERIAL2_RX (31u)

// SPI
#define PIN_SPI0_MISO  (4u)
#define PIN_SPI0_MOSI  (7u)
#define PIN_SPI0_SCK   (2u)
// No SS pin specified to the library, code will set it manually.
// Pins routed to the connectors are GPIO8 and GPIO9 
#define PIN_SPI0_SS    (31u)
#define PIN_SPI0_SS08  (8u)
#define PIN_SPI0_SS09  (9u)

#define PIN_SPI1_MISO  (12u)
#define PIN_SPI1_MOSI  (15u)
#define PIN_SPI1_SCK   (10u)
// Pin routed to the connector is GPIO13, as labelled on the board.
#define PIN_SPI1_SS    (13u)

// Wire0 (connected to STEMMA QT connector)
#define PIN_WIRE0_SDA (0u)
#define PIN_WIRE0_SCL (1u)
// Wire1 (N/A)
#define PIN_WIRE1_SDA (31u)
#define PIN_WIRE1_SCL (31u)

// on-board piezo speaker
#define PIN_PIEZO (24u)

// Button connectors
#define PIN_B16       (16u)
#define PIN_B17       (17u)
#define PIN_B18       (18u)
#define PIN_B19       (19u)
#define PIN_B20       (20u)
#define PIN_B21       (21u)

// pins routed to the breakout connector
#define PIN_BREAKOUT1 (26u)
#define PIN_BREAKOUT2 (27u)
#define PIN_BREAKOUT3 (28u)
#define PIN_BREAKOUT4 (29u)

// parameters for the common header and library code
#define SERIAL_HOWMANY (0u)
#define SPI_HOWMANY    (2u)
#define WIRE_HOWMANY   (1u)

// Include variants/generic/common.h from framework-arduinopico 
// This should be the correct relative path off the existing framework include path 
//     "[...]/framework-arduinopico/include"
#include "../variants/generic/common.h"
