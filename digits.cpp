/**
 * @file digits.cpp
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

#include "include/digits.h"

#include "include/config.h"
#include "include/pins.h"

// Preinstantiate
Digits digits;

void Digits::init(void) {
    // Initialize latch pin for using with fast digital write
    latch_pin_mask = digitalPinToBitMask(PIN_LATCH);
    latch_pin_p_out = portOutputRegister(digitalPinToPort(PIN_LATCH));
    pinMode(PIN_LATCH, OUTPUT);

    // Initialize SPI
    spi_settings = SPISettings(SPI_CLOCK_DIV16, MSBFIRST, SPI_MODE0);
    SPI.begin();

    // CTC mode
    // See "Table 14-8. Waveform Generation Mode Bit Description" in Atmega328P datasheet for more info
    TCCR0A |= _BV(WGM01);

    // Set prescaler
    TCCR0B |= TIMER0_PRESCALER;

    // Output Compare Match A Interrupt Enable
    TIMSK0 |= _BV(OCIE0A);
    OCR0A = OCR0A_VALUE;

    // Enable interrupts
    sei();

    // Disable everything
    set();
}

/**
 * @brief Sets all digits
 *
 * @param digit_1 nixie 1 (0-9). If larger will be turned OFF
 * @param digit_2 nixie 2 (0-9). If larger will be turned OFF
 * @param digit_3 nixie 3 (0-9). If larger will be turned OFF
 * @param digit_4 nixie 4 (0-9). If larger will be turned OFF
 */
void Digits::set(uint8_t digit_1, uint8_t digit_2, uint8_t digit_3, uint8_t digit_4) {
    current_numbers[0] = digit_1;
    current_numbers[1] = digit_2;
    current_numbers[2] = digit_3;
    current_numbers[3] = digit_4;
}

/**
 * @brief Sets separator state
 *
 * @param state true to turn separator ON
 */
void Digits::set_separator(boolean state) { current_separator_state = state; }

/**
 * @brief Handles interrupt (writes current_numbers and current_separator_state to each nixie tube)
 */
void Digits::isr_callback_handler(void) {
    write(digit_counter, current_numbers[digit_counter], current_separator_state);
    digit_counter++;
    if (digit_counter == DIGITS_NUM)
        digit_counter = 0;
}

/**
 * @brief Calculates final bit mask and writes it into shift registers
 *
 * @param anode 0-N of nixies - 1 (3). If larger will not be used
 * @param number 0-9. If larger will not be used
 * @param separator true to also turn on the separator
 */
void Digits::write(uint8_t anode, uint8_t number, boolean separator) {
    // Calculate which anodes to activate
#ifdef ANODES_INVERTED
    uint16_t anode_mask = 0U;
    for (uint8_t i = 0; i < DIGITS_NUM; ++i)
        anode_mask |= pgm_read_word(&PINS_ANODES[i]);
    if (anode < DIGITS_NUM)
        anode_mask &= ~pgm_read_word(&PINS_ANODES[anode]);
#else
    uint16_t anode_mask = anode < anodes_num ? pgm_read_word(&PINS_ANODES[anode]) : 0U;
#endif

        // Calculate which numbers (cathodes) to activate
#ifdef NUMBERS_INVERTED
    uint16_t numbers_mask = 0U;
    for (uint8_t i = 0; i < 10U; ++i)
        numbers_mask |= pgm_read_word(&PINS_NUMBERS[i]);
    if (number < 10U)
        numbers_mask &= ~pgm_read_word(&PINS_NUMBERS[number]);
#else
    uint16_t numbers_mask = number < 10U ? pgm_read_word(&PINS_NUMBERS[number]) : 0U;
#endif

    // Build final mask
    uint16_t mask = anode_mask | numbers_mask;

    // Add separator
#ifdef SEPARATOR_INVERTED
    if (separator)
        mask &= ~PIN_SEPARATOR;
    else
        mask |= PIN_SEPARATOR;
#endif
    if (separator)
        mask |= PIN_SEPARATOR;
    else
        mask &= ~PIN_SEPARATOR;

    // Write to the shift registers
    SPI.beginTransaction(spi_settings);
    *latch_pin_p_out &= ~latch_pin_mask;
    SPI.transfer(mask & 0xFF);
    SPI.transfer((mask >> 8) & 0xFF);
    *latch_pin_p_out |= latch_pin_mask;
    SPI.endTransaction();
}

/**
 * @brief Redirects interrupt to static member of Digits instance
 */
ISR(TIMER0_COMPA_vect) { digits._isr_callback(); }

/**
 * @brief Redirects static to a non-static isr_callback_handler()
 */
void Digits::_isr_callback(void) { digits.isr_callback_handler(); }
