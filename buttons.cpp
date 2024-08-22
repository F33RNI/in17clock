/**
 * @file buttons.cpp
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

#include "include/buttons.h"

#include "include/pins.h"

// Preinstantiate
Buttons buttons;

/**
 * @brief Initializes pins and enables interrupts on all of them
 */
void Buttons::init(void) {
    // Setup all pins with pullup resistors enabled
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_WEATHER, INPUT_PULLUP);
    pinMode(PIN_BTN_SET, INPUT_PULLUP);
    pinMode(PIN_SW_ALARM, INPUT_PULLUP);

    // Convert pins into input registers and masks for faster access
    btn_up_pin_mask = digitalPinToBitMask(PIN_BTN_UP);
    btn_up_port = portInputRegister(digitalPinToPort(PIN_BTN_UP));
    btn_down_pin_mask = digitalPinToBitMask(PIN_BTN_DOWN);
    btn_down_port = portInputRegister(digitalPinToPort(PIN_BTN_DOWN));
    btn_weather_pin_mask = digitalPinToBitMask(PIN_BTN_WEATHER);
    btn_weather_port = portInputRegister(digitalPinToPort(PIN_BTN_WEATHER));
    btn_set_pin_mask = digitalPinToBitMask(PIN_BTN_SET);
    btn_set_port = portInputRegister(digitalPinToPort(PIN_BTN_SET));
    sw_alarm_pin_mask = digitalPinToBitMask(PIN_SW_ALARM);
    sw_alarm_port = portInputRegister(digitalPinToPort(PIN_SW_ALARM));

    // Read all buttons at startup (because PCINT only fires on change)
    read(255U);

    // Enable interrupts on all pins
    *digitalPinToPCMSK(PIN_BTN_UP) |= (1 << digitalPinToPCMSKbit(PIN_BTN_UP));
    *digitalPinToPCICR(PIN_BTN_UP) |= (1 << digitalPinToPCICRbit(PIN_BTN_UP));
    *digitalPinToPCMSK(PIN_BTN_DOWN) |= (1 << digitalPinToPCMSKbit(PIN_BTN_DOWN));
    *digitalPinToPCICR(PIN_BTN_DOWN) |= (1 << digitalPinToPCICRbit(PIN_BTN_DOWN));
    *digitalPinToPCMSK(PIN_BTN_WEATHER) |= (1 << digitalPinToPCMSKbit(PIN_BTN_WEATHER));
    *digitalPinToPCICR(PIN_BTN_WEATHER) |= (1 << digitalPinToPCICRbit(PIN_BTN_WEATHER));
    *digitalPinToPCMSK(PIN_BTN_SET) |= (1 << digitalPinToPCMSKbit(PIN_BTN_SET));
    *digitalPinToPCICR(PIN_BTN_SET) |= (1 << digitalPinToPCICRbit(PIN_BTN_SET));
    *digitalPinToPCMSK(PIN_SW_ALARM) |= (1 << digitalPinToPCMSKbit(PIN_SW_ALARM));
    *digitalPinToPCICR(PIN_SW_ALARM) |= (1 << digitalPinToPCICRbit(PIN_SW_ALARM));
}

/**
 * @return boolean true if UP button is pressed
 */
boolean Buttons::get_up(void) {
    boolean up_;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { up_ = up; }
    DEBOUNCING_RETURN(up_, up_state, up_last)
}

/**
 * @return boolean true if DOWN button is pressed
 */
boolean Buttons::get_down(void) {
    boolean down_;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { down_ = down; }
    DEBOUNCING_RETURN(down_, down_state, down_last)
}

/**
 * @return boolean true if WEATHER button is pressed
 */
boolean Buttons::get_weather(void) {
    boolean weather_;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { weather_ = weather; }
    DEBOUNCING_RETURN(weather_, weather_state, weather_last)
}

/**
 * @return boolean true if SET button is pressed
 */
boolean Buttons::get_set(void) {
    boolean set_;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { set_ = set; }
    DEBOUNCING_RETURN(set_, set_state, set_last)
}

/**
 * @return boolean true if alarm switch is ON
 */
boolean Buttons::get_alarm(void) {
    boolean alarm_;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { alarm_ = alarm; }
    DEBOUNCING_RETURN(alarm_, alarm_state, alarm_last)
}

/**
 * @brief Reads states of all buttons and alarm switch
 *
 * @param pcicr_bit index of fired PCINT vector to prevent multiple reads of the same pin or 255 to read all
 */
void Buttons::read(uint8_t pcicr_bit) {
    if (pcicr_bit == digitalPinToPCICRbit(PIN_BTN_UP) || pcicr_bit == 255U)
        up = !(*btn_up_port & btn_up_pin_mask);
    if (pcicr_bit == digitalPinToPCICRbit(PIN_BTN_DOWN) || pcicr_bit == 255U)
        down = !(*btn_down_port & btn_down_pin_mask);
    if (pcicr_bit == digitalPinToPCICRbit(PIN_BTN_WEATHER) || pcicr_bit == 255U)
        weather = !(*btn_weather_port & btn_weather_pin_mask);
    if (pcicr_bit == digitalPinToPCICRbit(PIN_BTN_SET) || pcicr_bit == 255U)
        set = !(*btn_set_port & btn_set_pin_mask);
    if (pcicr_bit == digitalPinToPCICRbit(PIN_SW_ALARM) || pcicr_bit == 255U)
        alarm = !(*sw_alarm_port & sw_alarm_pin_mask);
}

/**
 * @brief Redirects interrupts to non-static read()
 *
 * @param pcicr_bit index of fired PCINT vector to prevent multiple reads of the same pin
 */
void Buttons::_isr(uint8_t pcicr_bit) { buttons.read(pcicr_bit); }

#ifdef PCINT0_vect
ISR(PCINT0_vect) { buttons._isr(0U); }
#endif
#ifdef PCINT1_vect
ISR(PCINT1_vect) { buttons._isr(1U); }
#endif
#ifdef PCINT2_vect
ISR(PCINT2_vect) { buttons._isr(2U); }
#endif
#ifdef PCINT3_vect
ISR(PCINT3_vect) { buttons._isr(3U); }
#endif
#ifdef PCINT4_vect
ISR(PCINT4_vect) { buttons._isr(4U); }
#endif
#ifdef PCINT5_vect
ISR(PCINT5_vect) { buttons._isr(5U); }
#endif
#ifdef PCINT6_vect
ISR(PCINT6_vect) { buttons._isr(6U); }
#endif
#ifdef PCINT7_vect
ISR(PCINT7_vect) { buttons._isr(7U); }
#endif
