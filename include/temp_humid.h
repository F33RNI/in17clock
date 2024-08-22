/**
 * @file temp_humid.h
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

#ifndef TEMP_HUMID_H__
#define TEMP_HUMID_H__

#include <Arduino.h>
#include <Wire.h>

#define _POLYNOMIAL       0x31U
#define READ_TEMP_HUMID_0 0x24
#define READ_TEMP_HUMID_1 0x00

// In milliseconds
#define READ_INTERVAL 20UL

// Temperature and humidity low-pass filter (0-1). Closer to 1 -> smoother and slower
const float TEMP_HUMID_FILTER_K PROGMEM = .994f;

class TempHumid {
  public:
    void init(void);
    void read(void);
    float get_temperature(void), get_humidity(void);

    static inline uint8_t crc_8(uint8_t byte_1, uint8_t byte_2);

  private:
    uint64_t read_timer;
    float temperature_last, temperature_filtered;
    float humidity_last, humidity_filtered;
};

extern TempHumid temp_humid;

#endif
