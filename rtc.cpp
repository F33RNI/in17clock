/**
 * @file rtc.cpp
 * @author Fern Lane
 * @brief Handles DS3231 RTC
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

#include "include/rtc.h"

#include "include/pins.h"

// Preinstantiate
RTC rtc;

void RTC::init(void) {
    // Initialize IC2 and wait a bit
#ifndef _WIRE_INITIALIZED
#define _WIRE_INITIALIZED
    Wire.begin();
    delay(100UL);
#endif

    // Attach interrupt to SQW pin
    pinMode(PIN_SQW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_SQW), sqw_callback, FALLING);

    // Enable 1Hz SQW output (See "SQUARE-WAVE OUTPUT FREQUENCY" table in DS3231 datasheet)
    // <https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf>
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(REGISTER_CONTROL);
    Wire.write(0x00);
    Wire.endTransmission();
}

/**
 * @brief Sets new time (24-hours format) and resets date
 *
 * @param hours 0-23
 * @param minutes 0-59
 * @param seconds 0-59
 */
void RTC::set(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(REGISTER_TIME);

    // Seconds, minutes, hours
    Wire.write(dec_to_bcd(seconds));
    Wire.write(dec_to_bcd(minutes));
    Wire.write(dec_to_bcd(hours));

    // DOW, DOM, moth, year
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();
}

/**
 * @brief Retrieves data from DS3231. Call get_hours(), get_minutes(), get_seconds() to get parsed data
 */
void RTC::read(void) {
    // Request from time register and check for transmission error
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(REGISTER_TIME);
    if (Wire.endTransmission())
        return;

    // Request and read 3 bytes (hours, minutes and seconds)
    Wire.requestFrom(RTC_ADDRESS, 3);
    seconds_raw = Wire.read();
    minutes_raw = Wire.read();
    hours_raw = Wire.read();
}

/**
 * @return uint8_t current hours (24-hours format). Call read() before to retrieve new data
 */
uint8_t RTC::get_hours(void) { return rtc.bcd_to_dec(hours_raw & 0x3F); }

/**
 * @return uint8_t current minutes. Call read() before to retrieve new data
 */
uint8_t RTC::get_minutes(void) { return rtc.bcd_to_dec(minutes_raw & 0x7F); }

/**
 * @return uint8_t current seconds. Call read() before to retrieve new data
 */
uint8_t RTC::get_seconds(void) { return rtc.bcd_to_dec(seconds_raw & 0x7F); }

/**
 * @brief Checks if SQW interrupt has been arrived atomically
 * Call clear_interrupt() to reset this flag after handling it
 *
 * @return boolean true if interrupt arrived
 */
boolean RTC::get_interrupt(void) {
    boolean interrupt_;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { interrupt_ = interrupt; }
    return interrupt_;
}

/**
 * @brief Clears SQW interrupt. Call this after handling get_interrupt()
 */
void RTC::clear_interrupt(void) {
    // Ignore if false to prevent errors
    if (!get_interrupt())
        return;
    interrupt = false;
}

/**
 * @brief Converts BCD numbers (from DS3231) into decimals
 *
 * @param bcd data from registers
 * @return uint8_t decimal number
 */
uint8_t RTC::bcd_to_dec(uint8_t bcd) { return (bcd & 0x0F) + 10 * ((bcd & 0xF0) >> 4); }

/**
 * @brief Converts decimal number into BCD (DS3231 registers format)
 *
 * @param dec decimal number
 * @return uint8_t bcd data
 */
uint8_t RTC::dec_to_bcd(uint8_t dec) { return ((dec % 10) & 0x0F) | (((dec / 10) << 4) & 0xF0); };

/**
 * @brief Sets internal non-static variable. Call get_interrupt() to read it atomically
 */
void RTC::sqw_callback(void) { rtc.interrupt = true; }
