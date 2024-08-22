/**
 * @file rtc.h
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

#ifndef RTC_H__
#define RTC_H__

#include <Arduino.h>
#include <Wire.h>
#include <util/atomic.h>

#define REGISTER_TIME    0x00
#define REGISTER_CONTROL 0x0E

class RTC {
  public:
    void init(void);
    void set(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void read(void);
    uint8_t get_hours(void), get_minutes(void), get_seconds(void);
    boolean get_interrupt(void);
    void clear_interrupt(void);
    static inline uint8_t bcd_to_dec(uint8_t bcd);
    static inline uint8_t dec_to_bcd(uint8_t dec);

  private:
    volatile uint8_t hours_raw, minutes_raw, seconds_raw;
    volatile boolean interrupt;

    static void sqw_callback(void);
};

extern RTC rtc;

#endif
