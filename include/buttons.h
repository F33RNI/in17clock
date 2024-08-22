/**
 * @file buttons.h
 * @author Fern Lane
 * @brief Interrupt-based buttons and alarm switch handler with debouncing
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

#ifndef BUTTONS_H__
#define BUTTONS_H__

#include <Arduino.h>
#include <util/atomic.h>

#define DEBOUNCING_RETURN(name, name_state, name_last)                                                                 \
    name_state <<= 1;                                                                                                  \
    name_state |= name & 0x01;                                                                                         \
    name_last = name_state == 0xFFFF ? true : !name_state ? false : name_last;                                         \
    return name_last;

class Buttons {
  public:
    void init(void);
    boolean get_up(void), get_down(void), get_weather(void), get_set(void), get_alarm(void);

    static void _isr(uint8_t pcicr_bit);

  private:
    volatile uint8_t btn_up_pin_mask, *btn_up_port;
    volatile uint8_t btn_down_pin_mask, *btn_down_port;
    volatile uint8_t btn_weather_pin_mask, *btn_weather_port;
    volatile uint8_t btn_set_pin_mask, *btn_set_port;
    volatile uint8_t sw_alarm_pin_mask, *sw_alarm_port;
    volatile boolean up, down, weather, set, alarm;
    boolean up_last, down_last, weather_last, set_last, alarm_last;
    uint16_t up_state, down_state, weather_state, set_state, alarm_state;

    void read(uint8_t pcicr_bit);
};

extern Buttons buttons;

#endif
