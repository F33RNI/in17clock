/**
 * @file buzzer.h
 * @author Fern Lane
 * @brief Alarm buzzer
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

#ifndef BUZZER_H__
#define BUZZER_H__

#include <Arduino.h>

// Timer 2 channel B pin on Atmega328P
#define _TIMER_2_B_PIN 3U

#define _RESOLUTION 256U

// Timer 2 prescalers from "Table 17-9. Clock Select Bit Description"
#define _PRESCALER_1    _BV(CS20)
#define _PRESCALER_8    _BV(CS21)
#define _PRESCALER_32   _BV(CS21) | _BV(CS20)
#define _PRESCALER_64   _BV(CS22)
#define _PRESCALER_128  _BV(CS22) | _BV(CS20)
#define _PRESCALER_256  _BV(CS22) | _BV(CS21)
#define _PRESCALER_1024 _BV(CS22) | _BV(CS21) | _BV(CS20)

class Buzzer {
  public:
    void init(void);
    void play_note(uint8_t note_number, uint8_t pwm);
    void play_chime(void);
    void decay(void);

  private:
    uint64_t decay_timer, chime_timer;
    uint16_t chime_note_duration;
    uint8_t prescaler_bits;
    uint8_t attack_pwm_value, note_last, note_duration_divider, note_counter;

    void set_frequency(float frequency);
    void set_duty_cycle(uint8_t duty_cycle);
};

extern Buzzer buzzer;

#endif
