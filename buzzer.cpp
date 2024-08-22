/**
 * @file buzzer.cpp
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

#include "include/buzzer.h"

#include "include/config.h"
#include "include/pins.h"

// Preinstantiate
Buzzer buzzer;

void Buzzer::init(void) {

    // Mode 5 "PWM phase correct", OCRA as top counter value
    // See "17.11.1" in Atmega328P datasheet for more info
    TCCR2A = _BV(WGM20);
    TCCR2B = _BV(WGM22);

    // Set prescaler and TOP counter
    set_frequency(1000.f);

    // Enable PWM
    // See "Table 17-4. Compare Output Mode, Phase Correct PWM Mode" for more info
    pinMode(_TIMER_2_B_PIN, OUTPUT);
    TCCR2A |= _BV(COM2B1);

    // Enable inverted mode if needed
#ifdef BUZZER_PWM_INVERTED
    TCCR2A |= _BV(COM2B0);
#endif

    // Turn buzzer OFF
    set_duty_cycle(0);
}

/**
 * @brief Starts playing note and resets decay timer
 *
 * @param note_number MIDI note number (69 = 440Hz). 0 = silence
 * @param pwm attack PWM value (0-255)
 */
void Buzzer::play_note(uint8_t note_number, uint8_t pwm) {
    if (note_number != note_last) {
        if (note_number != 0)
            set_frequency((A_BASE / 32.f) * powf(2.f, ((note_number - 9U) / 12.f)));
        note_last = note_number;
    }
    attack_pwm_value = note_number != 0 ? pwm : 0U;
    set_duty_cycle(attack_pwm_value);
    decay_timer = millis();
}

void Buzzer::play_chime(void) {
    uint64_t millis_current = millis();
    if (chime_timer > millis_current)
        chime_timer = millis_current;

    // New note
    if (millis_current - chime_timer >= chime_note_duration) {
        chime_timer = millis_current;

        // Select random velocity
        attack_pwm_value =
            BUZZER_PWM_START + ((int16_t) (random() % BUZZER_PWM_DEVIATION * 2) - ((int16_t) BUZZER_PWM_DEVIATION / 2));

        // Select random note
        play_note(pgm_read_byte(&ALARM_CHIME_NOTES[random() % ALARM_CHIME_NOTES_N]), attack_pwm_value);

        note_counter++;

        // Select new note duration
        if (note_counter > note_duration_divider) {
            note_duration_divider = pgm_read_byte(&NOTE_DURATION_DIVIDERS[random() % NOTE_DURATION_DIVIDERS_N]);
            // chime_note_duration = (60000.f / (float)map(millis() > 60000 ? 60000 : millis(), 0, 60000, 45, 800)) /
            // (float) note_duration_divider;
            chime_note_duration = 60000.f / ALARM_CHIME_BPM / (float) note_duration_divider;
            note_counter = 0;
        }
    }
}

/**
 * @brief Processes note decaying
 * NOTE: Must be called in a main loop without any delays (has internal timer)
 */
void Buzzer::decay(void) {
    uint64_t millis_current = millis();
    if (decay_timer > millis_current)
        decay_timer = millis_current;

    // Fully decayed
    if (decay_timer != 0 && millis_current - decay_timer >= DECAY_TIME) {
        decay_timer = 0;
        set_duty_cycle(0);
    }

    // Decaying
    else if (millis_current - decay_timer < DECAY_TIME) {
        uint8_t duty_cycle = map(millis_current - decay_timer, 0UL, DECAY_TIME, attack_pwm_value, 0UL);
        set_duty_cycle(duty_cycle);
    }
}

/**
 * @brief Sets PWM frequency
 *
 * @param frequency in Hz
 */
void Buzzer::set_frequency(float frequency) {
    // Convert frequency to cycles
    uint32_t cycles = (F_CPU / 2000000UL) * (1.e6f / frequency);

    // Calculate prescaler
    if (cycles < _RESOLUTION)
        prescaler_bits = _PRESCALER_1;
    else if ((cycles >>= 3U) < _RESOLUTION)
        prescaler_bits = _PRESCALER_8;
    else if ((cycles >>= 2U) < _RESOLUTION)
        prescaler_bits = _PRESCALER_32;
    else if ((cycles >>= 1U) < _RESOLUTION)
        prescaler_bits = _PRESCALER_64;
    else if ((cycles >>= 1U) < _RESOLUTION)
        prescaler_bits = _PRESCALER_128;
    else if ((cycles >>= 1U) < _RESOLUTION)
        prescaler_bits = _PRESCALER_256;
    else if ((cycles >>= 2U) < _RESOLUTION)
        prescaler_bits = _PRESCALER_1024;

    // Set prescaler and TOP counter
    TCCR2B = _BV(WGM22) | prescaler_bits;
    OCR2A = cycles;
}

/**
 * @brief Sets duty cycle of the PWM on pin 3
 *
 * @param duty_cycle 0 to 255 (255 - always HIGH)
 */
void Buzzer::set_duty_cycle(uint8_t duty_cycle) { OCR2B = (OCR2A * duty_cycle) >> 8U; }
