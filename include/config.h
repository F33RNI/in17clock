/**
 * @file config.h
 * @author Fern Lane
 * @brief Main configuration file. For pins please see pins.h
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

#ifndef CONFIG_H__
#define CONFIG_H__

#include <Arduino.h>

// ----------------------- //
// DC-DC Step-up converter //
// ----------------------- //

// Frequency (in KHz) of the PWM
#define CONVERTER_FREQUENCY 40000UL

// Don't touch the code below unless you know what you are doing
// -------------------------------------------------------------
// Convert to cycles (see Timer1 inside Atmega328P datasheet for more info) without prescaler
#define _CONVERTER_CYCLES (F_CPU / 100000UL * ((1000UL * 1000UL) / CONVERTER_FREQUENCY)) / 20UL
#if (_CONVERTER_CYCLES >= 65536UL)
#error CONVERTER_FREQUENCY is too low
#endif
const uint32_t CONVERTER_PERIOD_CYCLES PROGMEM = _CONVERTER_CYCLES;
// -------------------------------------------------------------

// Target output voltage min-max (in Volts) 255 max. Final voltage will be edited using buttons within these margins
const uint8_t CONVERTER_SETPOINT_MIN PROGMEM = 140.f;
const uint8_t CONVERTER_SETPOINT_MAX PROGMEM = 180.f;

// 0 to CONVERTER_SETPOINT time in milliseconds
const uint64_t CONVERTER_SOFT_START_TIME PROGMEM = 1000UL;

// Measured real 1.1V reference (in Volts)
const float VREF_ACTUAL_MV PROGMEM = 1.106f;

// Measured resistances of voltage divider (in Ohms)
const float CONVERTER_R_HIGH PROGMEM = 986000.f;
const float CONVERTER_R_LOW PROGMEM = 4270.f;

// Uncomment PID_AUTO_TUNE to perform PID auto-tuning on startup
// Connect your serial converter to TX pin of Atmega and listen on PID_AUTO_TUNE_BAUD_RATE
// (Result will be print to the serial port)
// #define PID_AUTO_TUNE
#ifdef PID_AUTO_TUNE
#define PID_AUTO_TUNE_TYPE      TYPE_PI
#define PID_AUTO_TUNE_N_CYCLES  1000UL
#define PID_AUTO_TUNE_SERIAL    Serial
#define PID_AUTO_TUNE_BAUD_RATE 9600UL
const float PID_P_GAIN PROGMEM = 0.f;
const float PID_I_GAIN PROGMEM = 0.f;
const float PID_D_GAIN PROGMEM = 0.f;
#else
// PID-controller gains
const float PID_P_GAIN PROGMEM = 85.f;
const float PID_I_GAIN PROGMEM = 11.4f;
const float PID_D_GAIN PROGMEM = 0.f;
#endif

// Limit PID output to 0% - 50% power
const float PID_MIN_OUT PROGMEM = 0.f;
const float PID_MAX_OUT PROGMEM = 512.f;

// Limit to prevent integral windup
const float PID_MIN_INTEGRAL PROGMEM = -1000.f;
const float PID_MAX_INTEGRAL PROGMEM = 1000.f;

// ------------------ //
// Nixie multiplexing //
// ------------------ //

// Interrupt (single digit show time) frequency (in Hz)
#define MULTIPLEXING_FREQUENCY 1000UL

// Don't touch the code below unless you know what you are doing
// -------------------------------------------------------------
// Convert to compare register value with 1/64 prescaler
#define OCR0A_VALUE ((F_CPU / 64UL) / MULTIPLEXING_FREQUENCY) - 1U
#if (OCR0A_VALUE > 255UL)
#error MULTIPLEXING_FREQUENCY is too low or Timer 0 prescaler is too small
#endif
// 1/64 prescaler
// See "Table 14-9. Clock Select Bit Description" in Atmega328P datasheet for more info
#define TIMER0_PRESCALER _BV(CS01) | _BV(CS00)
// -------------------------------------------------------------

// ------ //
// Digits //
// ------ //

// How long to keep separator ON after a new second
const uint16_t SEPARATOR_TIME PROGMEM = 250U;

// How long to show alarm preview after turning switch ON
const uint16_t ALARM_PREVIEW_TIME PROGMEM = 1000U;

// Hours and minutes will blink with this rate (in milliseconds) if alarm is active
const uint16_t ALARM_BLINK_RATE PROGMEM = 100U;

// Hours or minutes will blink in set mode with this rate (in milliseconds)
const uint16_t SET_BLINK_RATE PROGMEM = 250U;

// ------- //
// Buttons //
// ------- //

// Time between increments / decrements
const uint16_t BTN_INC_DEC_DELAY_LOW PROGMEM = 250U;
const uint16_t BTN_INC_DEC_DELAY_HIGH PROGMEM = 70U;

// Transition time from pressing button (BTN_INC_DEC_DELAY_LOW) to BTN_INC_DEC_DELAY_HIGH
const uint16_t BTN_INC_DEC_DELAY_TRANS_TIME PROGMEM = 2000U;

// ------ //
// Buzzer //
// ------ //

// Base initial PWM value (attack) (velocity)
const uint8_t BUZZER_PWM_START PROGMEM = 50U;

// How much can initial PWM value (attack) (velocity) deviate from BUZZER_PWM_START (+/-)
// NOTE: BUZZER_PWM_START + BUZZER_PWM_DEVIATION must be less then 255
// NOTE: BUZZER_PWM_START - BUZZER_PWM_DEVIATION must be greater then 0
const uint8_t BUZZER_PWM_DEVIATION PROGMEM = 40U;

// Note decay to 0 time (in milliseconds)
const uint16_t DECAY_TIME PROGMEM = 400U;

// Tuning frequency (in Hz)
const float A_BASE PROGMEM = 440.f;

// 1/4, 1/8, 1/16, 1/32 (will be select randomly)
const uint8_t NOTE_DURATION_DIVIDERS[] PROGMEM = {1U, 2U, 2U, 2U, 4U, 8U};
const uint8_t NOTE_DURATION_DIVIDERS_N PROGMEM = 6U;

// All the notes will be played randomly
// D Major
// const uint8_t ALARM_CHIME_NOTES[] PROGMEM = {0U,  0U,  74U, 78U, 81U, 74U, 78U, 81U,
//                                             86U, 88U, 90U, 91U, 93U, 95U, 97U, 98U};
// D Minor
const uint8_t ALARM_CHIME_NOTES[] PROGMEM = {0U,  0U,  62U, 65U, 69U, 62U, 65U, 69U,
                                             86U, 88U, 89U, 91U, 93U, 94U, 96U, 98U};
const uint8_t ALARM_CHIME_NOTES_N PROGMEM = 16U;

// Main BPM (length of 1/4 note)
const float ALARM_CHIME_BPM PROGMEM = 90.f;

// Button sounds velocity
const uint8_t BUTTON_NOTE_PWM = 10U;

// Button sounds (MIDI notes)
const uint8_t NOTE_INCREMENT = 91U;
const uint8_t NOTE_DECREMENT = 88U;
const uint8_t NOTE_TIME_MODE = 86U;
const uint8_t NOTE_SET_MODE = 81U;
const uint8_t NOTE_WEATHER_MODE PROGMEM = 93U;
const uint8_t NOTE_ALARM_ON PROGMEM = 81U;

#endif
