/**
 * @file power.h
 * @author Fern Lane
 * @brief DC-DC step up converter with feedback
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

#ifndef POWER_H__
#define POWER_H__

#include <Arduino.h>

#include "../lib/PetalPID/src/PetalPID.h"

#include "config.h"

// Timer 1 channel A pin on Atmega328P
#define _TIMER_1_A_PIN 9U

class Power {
  public:
    void init(void);
    void set_voltage(uint8_t voltage);
    uint8_t get_voltage(void);
    void regulate(void);

  private:
    PetalPID pid;
    float voltage;
    uint8_t setpoint, setpoint_temp;
    uint64_t time_started;
    void measure_voltage();
    void set_duty_cycle(uint16_t duty_cycle);
#ifdef PID_AUTO_TUNE
    boolean auto_tune_reported;
#endif
};

extern Power power;

#endif
