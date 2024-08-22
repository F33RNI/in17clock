/**
 * @file temp_humid.cpp
 * @author Fern Lane
 * @brief Handles DHT3X temperature + humidity sensor
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

#include "include/temp_humid.h"

#include "include/pins.h"

// Preinstantiate
TempHumid temp_humid;

/**
 * @brief Initializes I2C bus (if not yet initialized) and initializes internal variables
 */
void TempHumid::init(void) {
    // Initialize IC2 and wait a bit
#ifndef _WIRE_INITIALIZED
#define _WIRE_INITIALIZED
    Wire.begin();
    delay(100UL);
#endif

    // Initialize sensor
    Wire.beginTransmission(SHT_ADDRESS);
    Wire.endTransmission();

    // For proper filter initialization
    temperature_last = INFINITY;
    temperature_filtered = INFINITY;
    humidity_last = INFINITY;
    humidity_filtered = INFINITY;
}

/**
 * @brief Reads, parses and filters temperature and humidity
 * NOTE: Must be called in a main loop without any delays (has internal timer)
 */
void TempHumid::read(void) {
    // Read only after a delay
    if (millis() - read_timer < READ_INTERVAL)
        return;
    read_timer = millis();

    // Request data and check for transmission error
    Wire.beginTransmission(SHT_ADDRESS);
    Wire.write(READ_TEMP_HUMID_0);
    Wire.write(READ_TEMP_HUMID_1);
    if (Wire.endTransmission())
        return;

    // Request and read 6 bytes
    Wire.requestFrom(SHT_ADDRESS, 6);
    uint8_t temp_raw_0 = Wire.read();
    uint8_t temp_raw_1 = Wire.read();
    uint8_t temp_crc = Wire.read();
    uint8_t humid_raw_0 = Wire.read();
    uint8_t humid_raw_1 = Wire.read();
    uint8_t humid_crc = Wire.read();

    // Verify checksums
    if (crc_8(temp_raw_0, temp_raw_1) != temp_crc || crc_8(humid_raw_0, humid_raw_1) != humid_crc)
        return;

    // Parse temperature
    int32_t temp_raw = (int32_t) (((uint32_t) temp_raw_0 << 8) | temp_raw_1);
    temp_raw = ((4375 * temp_raw) >> 14UL) - 4500;
    float temperature = temp_raw / 100.f;

    // Filter temperature
    if (temperature_last == INFINITY)
        temperature_last = temperature;
    if (temperature_filtered == INFINITY)
        temperature_filtered = temperature;
    else
        temperature_filtered = temperature_filtered * TEMP_HUMID_FILTER_K +
                               temperature * (1.f - TEMP_HUMID_FILTER_K) / 2.f +
                               temperature_last * (1.f - TEMP_HUMID_FILTER_K) / 2.f;
    temperature_last = temperature;

    // Parse humidity
    uint32_t humid_raw = ((uint32_t) humid_raw_0 << 8) | humid_raw_1;
    humid_raw = (625 * humid_raw) >> 12UL;
    float humidity = humid_raw / 100.f;

    // Filter humidity
    if (humidity_last == INFINITY)
        humidity_last = humidity;
    if (humidity_filtered == INFINITY)
        humidity_filtered = humidity;
    else
        humidity_filtered = humidity_filtered * TEMP_HUMID_FILTER_K + humidity * (1.f - TEMP_HUMID_FILTER_K) / 2.f +
                            humidity_last * (1.f - TEMP_HUMID_FILTER_K) / 2.f;
    humidity_last = humidity;
}

/**
 * @return float filtered temperature in degrees Celsius
 */
float TempHumid::get_temperature(void) { return temperature_filtered == NAN ? 0.f : temperature_filtered; }

/**
 * @return float filtered humidity in %
 */
float TempHumid::get_humidity(void) { return humidity_filtered == NAN ? 0.f : humidity_filtered; }

/**
 * @brief Calculates CRC checksum. Read "4.12 Checksum Calculation" section for more info
 * <https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf>
 *
 * @param byte_1 first data byte
 * @param byte_2 second data byte
 * @return uint8_t result
 */
uint8_t TempHumid::crc_8(uint8_t byte_1, uint8_t byte_2) {
    uint8_t crc = 0xFFU;

    crc ^= byte_1++;
    for (uint8_t i = 8; i; --i)
        crc = (crc & 0x80U) ? (crc << 1U) ^ _POLYNOMIAL : (crc << 1U);

    crc ^= byte_2++;
    for (uint8_t i = 8; i; --i)
        crc = (crc & 0x80U) ? (crc << 1U) ^ _POLYNOMIAL : (crc << 1U);

    return crc;
}
