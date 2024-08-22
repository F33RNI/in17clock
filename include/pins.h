/**
 * @file pins.h
 * @author Fern Lane
 * @brief Hardware pins configuration
 *
 * @copyright Copyright (c) 2024 Fern Lane
 *
 * This file is part of the in17clock distribution.
 * See <https://github.com/F33RNI/in17clock> for more info.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * long with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PINS_H__
#define PINS_H__

#include <Arduino.h>

// --------------- //
// Shift registers //
// --------------- //

// Both HC595 are connected in a chain to the SPI, but we need to specify latch pin separately. Use can use any pin
const uint8_t PIN_LATCH PROGMEM = 10U;

// Common pins (anodes) of the nixies (MSBFIRST, big endian) (bits looks the same as on the board)
const uint16_t PINS_ANODES[] PROGMEM = {0b0010000000000000, 0b0000100000000000, 0b0000000010000000, 0b0000000000000100};

// Uncomment if HIGH on anode pin means nixie is OFF and LOW means nixie is ON
#define ANODES_INVERTED

// Number pins (cathodes) of the nixies (MSBFIRST, big endian) (bits looks the same as on the board)
const uint16_t PINS_NUMBERS[] PROGMEM = {0b0100000000000000, 0b0000000000000001, 0b1000000000000000, 0b0001000000000000,
                                         0b0000010000000000, 0b0000001000000000, 0b0000000001000000, 0b0000000000001000,
                                         0b0000000000100000, 0b0000000000000010};

// Uncomment if HIGH on cathode pin means number is OFF and LOW means number is ON
// #define NUMBERS_INVERTED

// Hours : minutes separator
const uint16_t PIN_SEPARATOR PROGMEM = 0b0000000000010000;

// Uncomment if HIGH on separator pin means OFF and LOW means ON
// #define SEPARATOR_INVERTED

// Numbers from bottom to the top (for wave effect)
const uint8_t POSITION_TO_NUMBER[] PROGMEM = {1U, 7U, 2U, 8U, 0U, 6U, 3U, 9U, 4U, 5U};
const uint8_t NUMBER_TO_POSITION[] PROGMEM = {4U, 0U, 2U, 6U, 8U, 9U, 5U, 1U, 3U, 7U};

// ----------------------- //
// DC-DC Step-up converter //
// ----------------------- //

// Uncomment if larger duty cycles means lower output and smaller duty cycler means higher output
// For other settings see config.h
#define CONVERTER_PWM_INVERTED

// Voltage divider feedback pin
const uint8_t CONVERTER_SENSE_PIN PROGMEM = A0;

// ---------- //
// DS3231 RTC //
// ---------- //

// DS3231 I2C address
#define RTC_ADDRESS 0x68

// DS3231 Square wave output interrupt pin (2 or 3)
const uint8_t PIN_SQW PROGMEM = 2U;

// ----------------------------------- //
// SHT31 Temperature & humidity sensor //
// ----------------------------------- //

// SHT31 I2C address
#define SHT_ADDRESS 0x44

// ------- //
// Buttons //
// ------- //

// Brightness + / hours button (any pin with pullup resistor and PCINT available)
const uint8_t PIN_BTN_UP PROGMEM = 5;

// Brightness - / minutes button (any pin with pullup resistor and PCINT available)
const uint8_t PIN_BTN_DOWN PROGMEM = A3;

// Show temperature and humidity button (any pin with pullup resistor and PCINT available)
const uint8_t PIN_BTN_WEATHER PROGMEM = A2;

// Set time / alarm (if switch is also ON) (any pin with pullup resistor and PCINT available)
const uint8_t PIN_BTN_SET PROGMEM = 6;

// Alarm switch (any pin with pullup resistor and PCINT available)
const uint8_t PIN_SW_ALARM PROGMEM = A1;

#endif
