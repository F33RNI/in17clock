/**
 * @file main.cpp
 * @author Fern Lane
 * @brief IN17 Nixie tube clock with internal DC-DC converter, random melody alarm and temperature/humidity sensor
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

#include <Arduino.h>
#include <EEPROM.h>

#include "include/config.h"
#include "include/pins.h"

#include "include/buttons.h"
#include "include/buzzer.h"
#include "include/digits.h"
#include "include/power.h"
#include "include/rtc.h"
#include "include/temp_humid.h"

#define MODE_TIME        0U
#define MODE_VOLTAGE     1U
#define MODE_SET_HOURS   2U
#define MODE_SET_MINUTES 3U
#define MODE_WEATHER     4U

uint8_t mode;
uint64_t separator_timer, blink_timer, wave_timer, btn_timer, inc_dec_timer, alarm_preview_timer;
uint8_t set_hours, set_minutes, alarm_hours, alarm_minutes, alarm_disabled_hours, alarm_disabled_minutes;
uint8_t wave_positions[4], wave_counter;
uint16_t inc_dec_delay;
boolean separator, blink_state, set_last, wave_started, alarm_active;

void alarm(void);
void mode_clock(boolean sqw_interrupt);
void mode_voltage(void);
void mode_set(boolean sqw_interrupt);
void mode_weather(void);
boolean inc_dec(void);
void increment(void);
void decrement(void);
void return_to_main(void);

void setup() {
    // Initialize everything
    power.init();
    digits.init();
    rtc.init();
    temp_humid.init();
    buzzer.init();
    buttons.init();
    EEPROM.begin();

    // Rotate random seed
    uint32_t seed = (uint32_t) EEPROM.read(0) | ((uint32_t) EEPROM.read(1) << 8) | ((uint32_t) EEPROM.read(2) << 16) |
                    ((uint32_t) EEPROM.read(3) << 24);
    randomSeed(seed);
    seed = random();
    EEPROM.write(0, seed & 0xFF);
    EEPROM.write(1, (seed >> 8) & 0xFF);
    EEPROM.write(2, (seed >> 16) & 0xFF);
    EEPROM.write(3, (seed >> 24) & 0xFF);

    // Restore converter voltage
    uint8_t voltage = EEPROM.read(4);
    if (voltage > CONVERTER_SETPOINT_MAX || voltage < CONVERTER_SETPOINT_MIN)
        voltage = ((uint16_t) CONVERTER_SETPOINT_MAX + (uint16_t) CONVERTER_SETPOINT_MIN) / 2;
    power.set_voltage(voltage);

    // Restore alarm
    alarm_hours = EEPROM.read(5);
    if (alarm_hours > 23)
        alarm_hours = 0;
    alarm_minutes = EEPROM.read(6);
    if (alarm_minutes > 59)
        alarm_minutes = 0;
    alarm_active = !EEPROM.read(7);
    alarm_disabled_hours = 255U;
    alarm_disabled_minutes = 255U;

    // Initiate wave at the start
    rtc.read();
    wave_started = true;
    wave_counter = 0;
    wave_positions[0] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_hours() / 10]);
    wave_positions[1] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_hours() % 10]);
    wave_positions[2] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_minutes() / 10]);
    wave_positions[3] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_minutes() % 10]);
}

void loop() {
    power.regulate();
    temp_humid.read();

    // Handle 1Hz RTC interrupts (SQW)
    boolean sqw_interrupt = false;
    if (rtc.get_interrupt()) {
        sqw_interrupt = true;
        rtc.clear_interrupt();
        rtc.read();
    }

    if (mode == MODE_TIME) {
        alarm();
        mode_clock(sqw_interrupt);
    } else if (mode == MODE_VOLTAGE)
        mode_voltage();
    else if (mode == MODE_SET_HOURS || mode == MODE_SET_MINUTES)
        mode_set(sqw_interrupt);
    else if (mode == MODE_WEATHER)
        mode_weather();

    buzzer.decay();
}

/**
 * @brief Handles alarm, alarm switch and buzzer chime
 */
void alarm(void) {
    // Clear turned off time
    if (rtc.get_hours() != alarm_disabled_hours || rtc.get_minutes() != alarm_disabled_minutes) {
        alarm_disabled_hours = 255U;
        alarm_disabled_minutes = 255U;
    }

    // Alarm is switch is ON
    if (buttons.get_alarm()) {
        // Start alarm preview
        if (alarm_preview_timer == 0) {
            alarm_preview_timer = millis();
            buzzer.play_note(NOTE_ALARM_ON, BUTTON_NOTE_PWM);
        }

        // Activate alarm
        if (rtc.get_hours() == alarm_hours && rtc.get_minutes() == alarm_minutes &&
            rtc.get_hours() != alarm_disabled_hours && rtc.get_minutes() != alarm_disabled_minutes && !alarm_active) {
            alarm_active = true;
            EEPROM.write(7, !alarm_active);
        }
    }

    // Alarm switch is OFF
    else {
        alarm_preview_timer = 0;

        // Alarm has been turned off
        if (alarm_active) {
            alarm_active = false;
            alarm_disabled_hours = rtc.get_hours();
            alarm_disabled_minutes = rtc.get_minutes();
            EEPROM.write(7, !alarm_active);
            buzzer.play_note(NOTE_TIME_MODE, BUTTON_NOTE_PWM);
        }
    }

    // Pi pi pi...
    if (alarm_active)
        buzzer.play_chime();
}

/**
 * @brief Main mode (shows hours : minutes) + alarm + wave
 *
 * @param sqw_interrupt true if RTC interrupt arrived
 */
void mode_clock(boolean sqw_interrupt) {
    if (wave_started) {
        // Update wave each 2s / (10numbers * 2cycles) = 100ms
        if (millis() - wave_timer >= 100U) {
            wave_timer = millis();
            for (uint8_t i = 0; i < 4; ++i)
                wave_positions[i] = wave_positions[i] == 9 ? 0 : wave_positions[i] + 1;
            digits.set(pgm_read_byte(&POSITION_TO_NUMBER[wave_positions[0]]),
                       pgm_read_byte(&POSITION_TO_NUMBER[wave_positions[1]]),
                       pgm_read_byte(&POSITION_TO_NUMBER[wave_positions[2]]),
                       pgm_read_byte(&POSITION_TO_NUMBER[wave_positions[3]]));
            wave_counter++;

            // Turn wave OFF after 20 cycles
            if (wave_counter == 21) {
                wave_started = false;
                digits.set(rtc.get_hours() / 10, rtc.get_hours() % 10, rtc.get_minutes() / 10, rtc.get_minutes() % 10);
            }
        }
    }

    // Blink with time if alarm is active
    if (alarm_active) {
        if (millis() - blink_timer >= ALARM_BLINK_RATE) {
            blink_timer = millis();
            blink_state = !blink_state;
        }
        if (blink_state)
            digits.set(rtc.get_hours() / 10, rtc.get_hours() % 10, rtc.get_minutes() / 10, rtc.get_minutes() % 10);
        else
            digits.set(255U, 255U, 255U, 255U);
    }

    // Briefly show alarm setpoint
    else if (alarm_preview_timer != 0 && millis() - alarm_preview_timer <= ALARM_PREVIEW_TIME)
        digits.set(alarm_hours / 10, alarm_hours % 10, alarm_minutes / 10, alarm_minutes % 10);

    // New second
    if (sqw_interrupt) {
        // Normal mode
        if (!alarm_active && !wave_started && millis() - alarm_preview_timer > ALARM_PREVIEW_TIME)
            digits.set(rtc.get_hours() / 10, rtc.get_hours() % 10, rtc.get_minutes() / 10, rtc.get_minutes() % 10);

        // Turn separator ON and reset it's timer
        digits.set_separator(true);
        separator_timer = millis();

        // Start wave 2 seconds before new minute
        if (rtc.get_seconds() == 58U && !wave_started) {
            wave_started = true;
            wave_counter = 0;
            wave_positions[0] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_hours() / 10]);
            wave_positions[1] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_hours() % 10]);
            wave_positions[2] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_minutes() / 10]);
            wave_positions[3] = pgm_read_byte(&NUMBER_TO_POSITION[rtc.get_minutes() % 10]);
        }
    }

    // Clear separator
    if (separator_timer != 0 && millis() - separator_timer >= SEPARATOR_TIME) {
        digits.set_separator(false);
        separator_timer = 0;
    }

    // Set button pressed -> enter set mode
    if (buttons.get_set()) {
        if (!set_last) {
            mode = MODE_SET_HOURS;
            set_last = true;
            if (!buttons.get_alarm()) {
                set_hours = rtc.get_hours();
                set_minutes = rtc.get_minutes();
            }
            buzzer.play_note(NOTE_SET_MODE, BUTTON_NOTE_PWM);
        }

    } else
        set_last = false;

    // Up / down button pressed -> enter voltage select mode and reset timers
    if (buttons.get_down() || buttons.get_up()) {
        mode = MODE_VOLTAGE;
        btn_timer = millis();
        inc_dec_timer = btn_timer;
        inc_dec_delay = BTN_INC_DEC_DELAY_LOW;
    }

    // Weather button pressed -> switch to weather mode
    else if (buttons.get_weather()) {
        mode = MODE_WEATHER;
        buzzer.play_note(NOTE_WEATHER_MODE, BUZZER_PWM_START);
    }
}

/**
 * @brief Allows to edit nixie supply voltage
 * (Shows supply voltage in Volts)
 */
void mode_voltage(void) {
    // Set without separator
    digits.set(255U, power.get_voltage() / 100U, (power.get_voltage() - ((power.get_voltage() / 100U) * 100U)) / 10U,
               power.get_voltage() % 10U);
    digits.set_separator(false);

    // Edit voltage and return to main (time) mode if no more buttons pressed
    if (!inc_dec())
        return_to_main();
}

/**
 * @brief Allows to edit current time / alarm
 *
 * @param sqw_interrupt true if RTC interrupt arrived
 */
void mode_set(boolean sqw_interrupt) {
    // Blink with minutes or seconds every SET_BLINK_RATE milliseconds
    if (millis() - blink_timer >= SET_BLINK_RATE) {
        blink_timer = millis();
        blink_state = !blink_state;
    }

    // Show alarm time
    if (buttons.get_alarm()) {
        digits.set(blink_state || mode == MODE_SET_MINUTES ? alarm_hours / 10 : 255U,
                   blink_state || mode == MODE_SET_MINUTES ? alarm_hours % 10 : 255U,
                   blink_state || mode == MODE_SET_HOURS ? alarm_minutes / 10 : 255U,
                   blink_state || mode == MODE_SET_HOURS ? alarm_minutes % 10 : 255U);
        digits.set_separator(true);
    }

    // Show main time
    else {
        digits.set(blink_state || mode == MODE_SET_MINUTES ? set_hours / 10 : 255U,
                   blink_state || mode == MODE_SET_MINUTES ? set_hours % 10 : 255U,
                   blink_state || mode == MODE_SET_HOURS ? set_minutes / 10 : 255U,
                   blink_state || mode == MODE_SET_HOURS ? set_minutes % 10 : 255U);
        digits.set_separator(false);
    }

    // Edit time / alarm and handle RTC interrupts (and update hours / minutes) if no up/down buttons pressed
    if (!inc_dec()) {
        if (sqw_interrupt) {
            set_hours = rtc.get_hours();
            set_minutes = rtc.get_minutes();
        }
    }

    // Set button pressed again -> edit minutes or return to main (time) mode
    if (buttons.get_set()) {
        if (!set_last) {
            set_last = true;
            if (mode == MODE_SET_HOURS) {
                mode = MODE_SET_MINUTES;
                buzzer.play_note(NOTE_SET_MODE, BUTTON_NOTE_PWM);
            } else
                return_to_main();
        }
    } else
        set_last = false;
}

/**
 * @brief Shows temperature (in degC) : humidity (in %)
 * NOTE: temperature will be absolute (-10degC -> 10degC)
 *
 */
void mode_weather(void) {
    // Limit temperature and humidity to 0-99
    uint16_t temperature_short = fabsf(temp_humid.get_temperature());
    if (temperature_short > 99U)
        temperature_short = 99U;
    uint16_t humidity_short = temp_humid.get_humidity();
    if (humidity_short > 99U)
        humidity_short = 99U;

    // Set with active separator
    digits.set(temperature_short / 10, temperature_short % 10, humidity_short / 10, humidity_short % 10);
    digits.set_separator(true);

    // Weather button released -> return to main (time) mode
    if (!buttons.get_weather())
        return_to_main();
}

/**
 * @brief Increment or decrements voltage / hours / minutes / alarm
 *
 * @return boolean true if one of the up or down buttons was pressed
 */
boolean inc_dec(void) {
    if (buttons.get_down() || buttons.get_up()) {
        if (millis() - btn_timer >= inc_dec_delay) {
            // Reset timer and calculate new increment / decrement delay based on time passed since mode activation
            btn_timer = millis();
            if (btn_timer - inc_dec_timer <= BTN_INC_DEC_DELAY_TRANS_TIME)
                inc_dec_delay = map(btn_timer - inc_dec_timer, 0UL, BTN_INC_DEC_DELAY_TRANS_TIME, BTN_INC_DEC_DELAY_LOW,
                                    BTN_INC_DEC_DELAY_HIGH);
            else
                inc_dec_delay = BTN_INC_DEC_DELAY_HIGH;

            if (buttons.get_down())
                decrement();
            else
                increment();
        }
        return true;
    }

    // Reset timer because no buttons pressed
    else {
        inc_dec_timer = millis();
        inc_dec_delay = BTN_INC_DEC_DELAY_LOW;
    }
    return false;
}

/**
 * @brief Increments voltage or main time or alarm
 * (depends on current mode)
 */
void increment(void) {
    // Increment voltage
    if (mode == MODE_VOLTAGE) {
        if (power.get_voltage() < CONVERTER_SETPOINT_MAX) {
            power.set_voltage(power.get_voltage() + 1);
            EEPROM.write(4, power.get_voltage());
        }
    }

    // Increment alarm or time
    else if (mode == MODE_SET_HOURS || mode == MODE_SET_MINUTES) {
        boolean alarm = buttons.get_alarm();

        // Increment alarm
        if (alarm) {
            if (mode == MODE_SET_HOURS && alarm_hours < 23)
                alarm_hours++;
            if (mode == MODE_SET_MINUTES && alarm_minutes < 59)
                alarm_minutes++;
            alarm_disabled_hours = 255U;
            alarm_disabled_minutes = 255U;
            EEPROM.write(5, alarm_hours);
            EEPROM.write(6, alarm_minutes);
        }

        // Increment time
        else {
            if (mode == MODE_SET_HOURS && set_hours < 23)
                set_hours++;
            if (mode == MODE_SET_MINUTES && set_minutes < 59)
                set_minutes++;
            rtc.set(set_hours, set_minutes, 0U);
        }
    }

    // Play increment sound
    buzzer.play_note(NOTE_INCREMENT, BUTTON_NOTE_PWM);
}

/**
 * @brief Decrements voltage or main time or alarm
 * (depends on current mode)
 */
void decrement(void) {
    // Decrement voltage
    if (mode == MODE_VOLTAGE) {
        if (power.get_voltage() > CONVERTER_SETPOINT_MIN) {
            power.set_voltage(power.get_voltage() - 1);
            EEPROM.write(4, power.get_voltage());
        }
    }

    // Decrement alarm or time
    else if (mode == MODE_SET_HOURS || mode == MODE_SET_MINUTES) {
        boolean alarm = buttons.get_alarm();

        // Decrement alarm
        if (alarm) {
            if (mode == MODE_SET_HOURS && alarm_hours > 0)
                alarm_hours--;
            if (mode == MODE_SET_MINUTES && alarm_minutes > 0)
                alarm_minutes--;
            alarm_disabled_hours = 255U;
            alarm_disabled_minutes = 255U;
            EEPROM.write(5, alarm_hours);
            EEPROM.write(6, alarm_minutes);
        }

        // Decrement time
        else {
            if (mode == MODE_SET_HOURS && set_hours > 0)
                set_hours--;
            if (mode == MODE_SET_MINUTES && set_minutes > 0)
                set_minutes--;
            rtc.set(set_hours, set_minutes, 0U);
        }
    }

    // Play decrement sound
    buzzer.play_note(NOTE_DECREMENT, BUTTON_NOTE_PWM);
}

/**
 * @brief Returns to main (time) mode
 */
void return_to_main(void) {
    mode = MODE_TIME;
    digits.set(rtc.get_hours() / 10, rtc.get_hours() % 10, rtc.get_minutes() / 10, rtc.get_minutes() % 10);
    digits.set_separator(false);
    rtc.clear_interrupt();
    buzzer.play_note(NOTE_TIME_MODE, BUTTON_NOTE_PWM);
}
