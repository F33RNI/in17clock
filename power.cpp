/**
 * @file power.cpp
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

#include "include/power.h"

// For official Arduino IDE compatibility. Don't need to include .cpp for PlatformIO
#ifndef PLATFORMIO_STYLE_IMPORTS
#include "lib/PetalPID/src/PetalPID.cpp"
#endif
#include "lib/PetalPID/src/PetalPID.h"

#include "include/config.h"
#include "include/pins.h"

// Preinstantiate
Power power;

/**
 * @brief Configures analog reference, Timer 1 and PWM on pin 9
 */
void Power::init(void) {
    // Initialize PID class instance
    pid = PetalPID(PID_P_GAIN, PID_I_GAIN, PID_D_GAIN, PID_MIN_OUT, PID_MAX_OUT);
    pid.set_min_max_integral(PID_MIN_INTEGRAL, PID_MAX_INTEGRAL);

    // Initialize Serial port for auto-tune output
#ifdef PID_AUTO_TUNE
    PID_AUTO_TUNE_SERIAL.begin(PID_AUTO_TUNE_BAUD_RATE);
    PID_AUTO_TUNE_SERIAL.println(F("--- in17clock  PID Auto-tune ---"));
    PID_AUTO_TUNE_SERIAL.println();
#endif

    // Phase and frequency correct mode, ICR1 as top counter value
    // See "Table 15-5. Waveform Generation Mode Bit Description" in Atmega328P datasheet for more info
    TCCR1B = _BV(WGM13);
    ICR1 = CONVERTER_PERIOD_CYCLES;

    // No prescaler
    TCCR1B |= _BV(CS10);

    // Reset control register A
    TCCR1A = 0;

    // Enable PWM
    pinMode(_TIMER_1_A_PIN, OUTPUT);
    TCCR1A |= _BV(COM1A1);

    // Enable inverted mode if needed
#ifdef CONVERTER_PWM_INVERTED
    TCCR1A |= _BV(COM1A0);
#endif

    // Initially set output to the lowest value (disable output)
    set_duty_cycle(0U);

    // Set analog reference to internal and wait for it to settle
    analogReference(INTERNAL);
    analogRead(CONVERTER_SENSE_PIN);
    delay(100UL);

    // Begin auto-tuning
#ifdef PID_AUTO_TUNE
    PID_AUTO_TUNE_SERIAL.println(F("Tuning... Please wait"));
    pid.start_auto_tune(PID_AUTO_TUNE_TYPE, PID_AUTO_TUNE_N_CYCLES);
#endif
}

/**
 * @brief Sets voltage setpoint
 *
 * @param voltage target output voltage in Volts
 */
void Power::set_voltage(uint8_t voltage) { setpoint = voltage; }

/**
 * @return uint8_t target output voltage in Volts (from set_voltage())
 */
uint8_t Power::get_voltage(void) { return setpoint; }

/**
 * @brief Measures and calculates output voltage, calculates PID controller and writes PWM
 * NOTE: This must called in a main loop without any delays!
 */
void Power::regulate(void) {
    measure_voltage();

    // Ignore soft-start in PID auto-tuning mode
#ifdef PID_AUTO_TUNE
    setpoint_temp = setpoint;
#else
    uint64_t millis_current = millis();
    // Record initial time for soft start
    if (time_started == 0)
        time_started = millis_current;

    // Gradually increase setpoint (soft start)
    if (millis_current - time_started > CONVERTER_SOFT_START_TIME)
        setpoint_temp = setpoint;
    else
        setpoint_temp = (float) (millis_current - time_started) / (float) CONVERTER_SOFT_START_TIME * setpoint;
#endif

    // Calculate and write PID controller
    set_duty_cycle(pid.calculate(voltage, setpoint_temp, micros()));

    // Print auto-tune result
#ifdef PID_AUTO_TUNE
    if (!auto_tune_reported && !pid.is_tuning()) {
        PID_AUTO_TUNE_SERIAL.println(F(""));
        PID_AUTO_TUNE_SERIAL.print(F("PID_P_GAIN = "));
        PID_AUTO_TUNE_SERIAL.println(pid.get_p(), 4);
        PID_AUTO_TUNE_SERIAL.print(F("PID_I_GAIN = "));
        PID_AUTO_TUNE_SERIAL.println(pid.get_i(), 4);
        PID_AUTO_TUNE_SERIAL.print(F("PID_D_GAIN = "));
        PID_AUTO_TUNE_SERIAL.println(pid.get_d(), 4);
        PID_AUTO_TUNE_SERIAL.println(F(""));
        PID_AUTO_TUNE_SERIAL.println(F("Done! Edit config and re-upload the code"));
        auto_tune_reported = true;
    }
#endif
}

/**
 * @brief Measures and calculates output voltage
 * (Result will be in private voltage variable)
 */
void Power::measure_voltage(void) {
    voltage = analogRead(CONVERTER_SENSE_PIN) / 1023.f * VREF_ACTUAL_MV;
    voltage /= (CONVERTER_R_LOW / (CONVERTER_R_LOW + CONVERTER_R_HIGH));
}

/**
 * @brief Sets duty cycle of PWM on pin 9
 *
 * @param duty_cycle 0 to 1023 (1024 - always HIGH)
 */
void Power::set_duty_cycle(uint16_t duty_cycle) { OCR1A = (CONVERTER_PERIOD_CYCLES * (uint32_t) duty_cycle) >> 10UL; }
