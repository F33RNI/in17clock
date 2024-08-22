/**
 * @file digits.h
 * @author Fern Lane
 * @brief Digits multiplexing
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

#ifndef DIGITS_H__
#define DIGITS_H__

#include <Arduino.h>
#include <SPI.h>

#include "config.h"

// Number of nixies
#define DIGITS_NUM 4U

class Digits {
  public:
    void init(void);
    void set(uint8_t digit_1 = 255U, uint8_t digit_2 = 255U, uint8_t digit_3 = 255U, uint8_t digit_4 = 255U);
    void set_separator(boolean state);
    static void _isr_callback(void);

  private:
    SPISettings spi_settings;
    volatile uint8_t current_numbers[4], digit_counter;
    volatile boolean current_separator_state;
    volatile uint8_t latch_pin_mask, *latch_pin_p_out;

    void write(uint8_t anode, uint8_t number, boolean separator = false);
    void isr_callback_handler(void);
};

extern Digits digits;

#endif
