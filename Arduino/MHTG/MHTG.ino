/*
 * ModularHydroponicTowerGarden (https://github.com/laszlodaniel/ModularHydroponicTowerGarden)
 * Copyright (C) 2021, Daniel Laszlo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published bypr
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <math.h>
#include <util/atomic.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h> // https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#include <TimeLib.h> // https://github.com/PaulStoffregen/Time
#include <TimerOne.h> // https://github.com/PaulStoffregen/TimerOne
#include <Wire.h>


// Firmware date/time of compilation in 64-bit UNIX time
// https://www.epochconverter.com/hex
#define FW_DATE 0x0000000060BA2885

#define TEMP_EXT      A0 // external 10k NTC thermistor is connected to this analog pin
#define TEMP_INT      A1 // internal 10k NTC thermistor is connected to this analog pin
#define RED_LED_PIN   4  // red LED
#define WATERPUMP_PIN 9  // waterpump pwm output

// Set (1), clear (0) and invert (1->0; 0->1) bit in a register or variable easily
#define sbi(reg, bit) (reg) |=  (1 << (bit))
#define cbi(reg, bit) (reg) &= ~(1 << (bit))
#define ibi(reg, bit) (reg) ^=  (1 << (bit))

#define to_uint16(hb, lb) (uint16_t)(((uint8_t)hb << 8) | (uint8_t)lb)
#define to_uint32(msb, hb, lb, lsb) (uint32_t)(((uint32_t)msb << 24) | ((uint32_t)hb << 16) | ((uint32_t)lb << 8) | (uint32_t)lsb)

// Packet related stuff
// DATA CODE byte building blocks
// Commands (low nibble (4 bits))
#define reset               0x00
#define handshake           0x01
#define status              0x02
#define settings            0x03
#define request             0x04
#define response            0x05
// 0x06-0x0D reserved
#define debug               0x0E
#define ok_error            0x0F

// SUB-DATA CODE byte
// Command 0x0F (ok_error)
#define ok                                      0x00
#define error_length_invalid_value              0x01
#define error_datacode_invalid_command          0x02
#define error_subdatacode_invalid_value         0x03
#define error_payload_invalid_values            0x04
#define error_packet_checksum_invalid_value     0x05
#define error_packet_timeout_occured            0x06
// 0x06-0xFD reserved
#define error_eeprom_checksum_mismatch          0xFC
#define error_not_enough_mcu_ram                0xFD
#define error_internal                          0xFE
#define error_fatal                             0xFF

// Construct an object called "lcd" for the external display (optional)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variables
uint32_t Ve, Vi; // raw analog voltage readings (external, internal temperatures)
uint16_t beta = 3950; // thermistor specific value
float inv_beta = 1.00 / (float)beta; // reciprocal of beta used in calculations
float inv_t0 = 1.00 / 298.15; // reciprocal of t0 = normal room temperature
uint16_t update_interval = 1000; // milliseconds

float T_ext[3]; // calculated external temperatures (0: Kelvin, 1: Celsius, 2: Fahrenheit)
float T_int[3]; // calculated internal temperatures (0: Kelvin, 1: Celsius, 2: Fahrenheit)
uint8_t temperature_payload[8];

uint32_t current_millis = 0;
uint8_t current_timestamp[4]; // current time is stored here when "update_timestamp" is called
uint32_t last_update_millis = 0;
uint32_t last_wpump_millis = 0;
uint32_t remaining_time = 0;
uint8_t percent_left = 0;
uint32_t t = 0; // remaining time is seconds
uint8_t h, m, s; // calculated from "t": hours component (h), minutes component (m), seconds component (s)

char rxBuffer[32]; // buffer for text-based communication
uint8_t rxBufferPtr = 0;

// EEPROM settings
uint8_t eeprom_content[64];
uint8_t default_eeprom_content[64] =
{
    0x00, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7F, 0x00, 0x0D, 0xBB, 0xA0, 0x00, 0x0D, 0xBB, 0xA0, 0x00, 0x01, 0x0F, 0x6E, 0x01,
    0xEA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t hw_version[2] = { 0x00, 0x00 };
uint8_t hw_date[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t assembly_date[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t old_wpump_pwm = 0;
uint8_t wpump_pwm = 0; // 0: 0% - 255: 100%
uint32_t wpump_ontime = 0; // 900000 milliseconds = 900 seconds = 15 minutes
uint32_t wpump_offtime = 0; // 900000 milliseconds = 900 seconds = 15 minutes
uint8_t enabled_temperature_sensors = 0;
uint8_t temperature_unit = 0;
uint16_t pwm_frequency = 490; // Hz
uint8_t calculated_checksum = 0;
bool checksum_ok = false;

bool wpump_on = true;
uint32_t wpump_interval = 0;
uint8_t avr_signature[3];

volatile bool mode_button_pressed = false;
bool autoupdate = true;
bool ascii_mode = true;
bool service_mode = false;
uint16_t service_mode_blinking_interval = 500; // ms
uint16_t led_blink_duration = 50; // ms
uint32_t previous_led_blink = 0; // ms
uint32_t red_led_ontime = 0; // ms

// Packet related variables
// Timeout values for packets
uint8_t command_timeout = 100; // milliseconds
uint16_t command_purge_timeout = 200; // milliseconds, if a command isn't complete within this time then delete the usb receive buffer
uint8_t ack[1] = { 0x00 }; // acknowledge payload array
uint8_t err[1] = { 0xFF }; // error payload array
uint8_t ret[1]; // general array to store arbitrary bytes

// LCD related variables
// Custom LCD-characters
// https://maxpromer.github.io/LCD-Character-Creator/
uint8_t degree_symbol[8] = { 0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00 }; // °
uint8_t progress_bar_1[8] = { 0x00, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00 };
uint8_t progress_bar_2[8] = { 0x00, 0x00, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00 };
uint8_t progress_bar_3[8] = { 0x00, 0x00, 0x1C, 0x1C, 0x1C, 0x00, 0x00, 0x00 };
uint8_t progress_bar_4[8] = { 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00 };
uint8_t progress_bar_5[8] = { 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00 };

typedef union {
    float number;
    uint8_t bytes[4];
} FLOATUNION_t;


/*****************************************************************************
Function: get_temps()
Purpose:  measures and converts the analog voltage present on A0 and A1 pins
Note:     -
******************************************************************************/
void get_temps(float *te, float *ti)
{
    float Ke, Ce, Fe; // external temperature variables
    float Ki, Ci, Fi; // internal temperature variables
    uint16_t sampleCount = 512;

    Ve = 0; // start with zero
    Vi = 0; // start with zero

    for (uint16_t i = 0; i < sampleCount; i++) // measure analog voltage quickly 500 times and add results together
    {
        Ve += analogRead(TEMP_EXT);
    }

    for (uint16_t i = 0; i < sampleCount; i++) // measure analog voltage quickly 500 times and add results together
    {
        Vi += analogRead(TEMP_INT);
    }

    Ve /= sampleCount; // calculate average value
    Vi /= sampleCount; // calculate average value

    // Calculate external temperature using this equation involving the thermistor's beta property
    Ke = 1.0 / (inv_t0 + inv_beta * (log(1023.0 / (float)Ve - 1.0))); // Kelvin
    Ce = Ke - 273.15; // Celsius
    Fe = Ce * 1.8 + 32.0; // Fahrenheit

    // Calculate internal temperature using this equation involving the thermistor's beta property
    Ki = 1.0 / (inv_t0 + inv_beta * (log(1023.0 / (float)Vi - 1.0))); // Kelvin
    Ci = Ki - 273.15; // Celsius
    Fi = Ci * 1.8 + 32.0; // Fahrenheit

    // Save values in the input arrays
    te[0] = Ke;
    te[1] = Ce;
    te[2] = Fe;

    ti[0] = Ki;
    ti[1] = Ci;
    ti[2] = Fi;

    // Save float values as their 4-byte constituents
    switch (temperature_unit)
    {
        case 1: // Celsius
        {
            FLOATUNION_t ext_celsius;
            FLOATUNION_t int_celsius;
            ext_celsius.number = te[1];
            int_celsius.number = ti[1];
            temperature_payload[0] = ext_celsius.bytes[0];
            temperature_payload[1] = ext_celsius.bytes[1];
            temperature_payload[2] = ext_celsius.bytes[2];
            temperature_payload[3] = ext_celsius.bytes[3];
            temperature_payload[4] = int_celsius.bytes[0];
            temperature_payload[5] = int_celsius.bytes[1];
            temperature_payload[6] = int_celsius.bytes[2];
            temperature_payload[7] = int_celsius.bytes[3];
            break;
        }
        case 2: // Fahrenheit
        {
            FLOATUNION_t ext_fahrenheit;
            FLOATUNION_t int_fahrenheit;
            ext_fahrenheit.number = te[2];
            int_fahrenheit.number = ti[2];
            temperature_payload[0] = ext_fahrenheit.bytes[0];
            temperature_payload[1] = ext_fahrenheit.bytes[1];
            temperature_payload[2] = ext_fahrenheit.bytes[2];
            temperature_payload[3] = ext_fahrenheit.bytes[3];
            temperature_payload[4] = int_fahrenheit.bytes[0];
            temperature_payload[5] = int_fahrenheit.bytes[1];
            temperature_payload[6] = int_fahrenheit.bytes[2];
            temperature_payload[7] = int_fahrenheit.bytes[3];
            break;
        }
        case 4: // Kelvin
        {
            FLOATUNION_t ext_kelvin;
            FLOATUNION_t int_kelvin;
            ext_kelvin.number = te[0];
            int_kelvin.number = ti[0];
            temperature_payload[0] = ext_kelvin.bytes[0];
            temperature_payload[1] = ext_kelvin.bytes[1];
            temperature_payload[2] = ext_kelvin.bytes[2];
            temperature_payload[3] = ext_kelvin.bytes[3];
            temperature_payload[4] = int_kelvin.bytes[0];
            temperature_payload[5] = int_kelvin.bytes[1];
            temperature_payload[6] = int_kelvin.bytes[2];
            temperature_payload[7] = int_kelvin.bytes[3];
            break;
        }
        default: // Celsius
        {
            FLOATUNION_t ext_celsius;
            FLOATUNION_t int_celsius;
            ext_celsius.number = te[1];
            int_celsius.number = ti[1];
            temperature_payload[0] = ext_celsius.bytes[0];
            temperature_payload[1] = ext_celsius.bytes[1];
            temperature_payload[2] = ext_celsius.bytes[2];
            temperature_payload[3] = ext_celsius.bytes[3];
            temperature_payload[4] = int_celsius.bytes[0];
            temperature_payload[5] = int_celsius.bytes[1];
            temperature_payload[6] = int_celsius.bytes[2];
            temperature_payload[7] = int_celsius.bytes[3];
            break;
        }
    }
    
} // end of get_temps


/*****************************************************************************
Function: handle_mode_button()
Purpose:  sets a flag when the mode button is pressed
Note:     the loop-function processes this flag
******************************************************************************/
void handle_mode_button(void)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        mode_button_pressed = true;
    }
    
} // end of handle_mode_button


/*****************************************************************************
Function: change_wpump_speed()
Purpose:  gradually changes water pump speed from current to target value
Note:     it's easier on the power supply if the speed is changed slowly
******************************************************************************/
void change_wpump_speed(uint8_t new_wpump_pwm)
{
    if (wpump_on || service_mode)
    {
        if (new_wpump_pwm == 0)
        {
            Timer1.pwm(WATERPUMP_PIN, 0); // turn off water pump
            digitalWrite(RED_LED_PIN, HIGH); // turn off indicator LED
            old_wpump_pwm = 0;
            return;
        }
        
        // Gradually change from old speed to new speed
        if (new_wpump_pwm > old_wpump_pwm) // increase speed
        {
            for (uint16_t i = old_wpump_pwm; i <= new_wpump_pwm; i++)
            {
                Timer1.pwm(WATERPUMP_PIN, map(i, 0, 255, 0, 1023)); // 8-bit to 10-bit conversion is necessary for this library
                delay(15);
                wdt_reset();
            }
        }
        else if (new_wpump_pwm < old_wpump_pwm) // decrease speed
        {
            for (uint16_t i = old_wpump_pwm; i >= new_wpump_pwm; i--)
            {
                Timer1.pwm(WATERPUMP_PIN, map(i, 0, 255, 0, 1023)); // 8-bit to 10-bit conversion is necessary for this library
                delay(15);
                wdt_reset();
            }
        }
    
        digitalWrite(RED_LED_PIN, LOW); // turn on indicator LED
        old_wpump_pwm = new_wpump_pwm; // save last set pwm-level
    }
    
} // end of change_wpump_speed


/*****************************************************************************
Function: update_timestamp()
Purpose:  saves the current elapes milliseconds in the target byte-array
Note:     -
******************************************************************************/
void update_timestamp(uint8_t *target)
{
    uint32_t mcu_current_millis = millis();
    target[0] = (mcu_current_millis >> 24) & 0xFF;
    target[1] = (mcu_current_millis >> 16) & 0xFF;
    target[2] = (mcu_current_millis >> 8) & 0xFF;
    target[3] = mcu_current_millis & 0xFF;
    
} // end of update_timestamp


/*****************************************************************************
Function: read_avr_signature()
Purpose:  read AVR device signature bytes into an array given in the parameter
Note:     ATmega328P: 1E 95 0F
******************************************************************************/
void read_avr_signature(uint8_t *target)
{
    target[0] = boot_signature_byte_get(0x0000);
    target[1] = boot_signature_byte_get(0x0002);
    target[2] = boot_signature_byte_get(0x0004);

} // end of read_avr_signature


/*************************************************************************
Function: update_pwm_frequency()
Purpose:  change water pump driver PWM-frequency
Note:     frequency changing while the water pump is running may cause glitches
**************************************************************************/
void update_pwm_frequency(uint16_t frequency)
{
    uint16_t period = 1000000 / frequency; // microseconds
    Timer1.setPeriod(period);
    
} // end of update_pwm_frequency


/*****************************************************************************
Function: update_settings()
Purpose:  takes values from the temporary eeprom_content array and updates variables
Note:     -
******************************************************************************/
void update_settings(void)
{
    hw_version[0] = eeprom_content[0x00];
    hw_version[1] = eeprom_content[0x01];
    hw_date[0] = eeprom_content[0x02];
    hw_date[1] = eeprom_content[0x03];
    hw_date[2] = eeprom_content[0x04];
    hw_date[3] = eeprom_content[0x05];
    hw_date[4] = eeprom_content[0x06];
    hw_date[5] = eeprom_content[0x07];
    hw_date[6] = eeprom_content[0x08];
    hw_date[7] = eeprom_content[0x09];
    assembly_date[0] = eeprom_content[0x0A];
    assembly_date[1] = eeprom_content[0x0B];
    assembly_date[2] = eeprom_content[0x0C];
    assembly_date[3] = eeprom_content[0x0D];
    assembly_date[4] = eeprom_content[0x0E];
    assembly_date[5] = eeprom_content[0x0F];
    assembly_date[6] = eeprom_content[0x10];
    assembly_date[7] = eeprom_content[0x11];
    wpump_pwm = eeprom_content[0x12];
    wpump_ontime = to_uint32(eeprom_content[0x13], eeprom_content[0x14], eeprom_content[0x15], eeprom_content[0x16]);
    wpump_offtime = to_uint32(eeprom_content[0x17], eeprom_content[0x18], eeprom_content[0x19], eeprom_content[0x1A]);
    enabled_temperature_sensors = eeprom_content[0x1B];
    temperature_unit = eeprom_content[0x1C];
    beta = to_uint16(eeprom_content[0x1D], eeprom_content[0x1E]);
    inv_beta = 1.00 / (float)beta;
    pwm_frequency = to_uint16(eeprom_content[0x1F], eeprom_content[0x20]);
    update_pwm_frequency(pwm_frequency);

    if (!service_mode)
    {
        if ((wpump_ontime == 0) && (wpump_offtime == 0))
        {
            wpump_interval = 0;
            change_wpump_speed(0);
            wpump_on = false;
        }
        else
        {
            if (wpump_offtime == 0) // permanently turn on water pump
            {
                wpump_interval = 0;
                wpump_on = true;
                change_wpump_speed(wpump_pwm);
            }
    
            if (wpump_ontime == 0) // permanently turn off water pump
            {
                wpump_interval = 0;
                change_wpump_speed(0);
                wpump_on = false;
            }
    
            if ((wpump_ontime != 0) && (wpump_offtime != 0))
            {
                if (wpump_on)
                {
                    wpump_interval = wpump_ontime;
                    wpump_on = true;
                    update_pwm_frequency(pwm_frequency);
                    change_wpump_speed(wpump_pwm);
                }
                else
                {
                    wpump_interval = wpump_offtime;
                    change_wpump_speed(0);
                    wpump_on = false;
                }
            }
        }
    }
    else
    {
        change_wpump_speed(0);
        wpump_on = false;
    }
    
} // end of update_settings


/*************************************************************************
Function: default_eeprom_settings()
Purpose:  sets default values in RAM when the EEPROM checksum doesn't match
Note:     -
**************************************************************************/
void default_eeprom_settings(void)
{
    // Calculate checksum for the default EEPROM content array
    default_eeprom_content[0x3F] = calculate_checksum(default_eeprom_content, 0, 63);

    // Copy all values
    for (uint8_t i = 0; i < 64; i++)
    {
        eeprom_content[i] = default_eeprom_content[i];
    }
    
} // end of default_eeprom_settings


/*************************************************************************
Function: read_eeprom_settings()
Purpose:  read internal EEPROM into RAM
Note:     -
**************************************************************************/
void read_eeprom_settings(void)
{
    // Read first 64 bytes of the internal EEPROM into RAM
    for (uint8_t i = 0; i < 64; i++)
    {
        eeprom_content[i] = EEPROM.read(i);
    }
    
    // Calculate expected checkum
    calculated_checksum = calculate_checksum(eeprom_content, 0, 63);

    if (calculated_checksum == eeprom_content[0x3F])
    {
        checksum_ok = true;
        update_settings(); // load and apply stored settings
    }
    else
    {
        checksum_ok = false;
        default_eeprom_settings(); // load default settings
        update_settings(); // apply default settings
        send_usb_packet(ok_error, error_eeprom_checksum_mismatch, err, 1);
    }
    
} // end of read_eeprom_settings


/*************************************************************************
Function: write_eeprom_settings()
Purpose:  write settings stored in RAM to internal EEPROM
Note:     -
**************************************************************************/
void write_eeprom_settings(void)
{
    eeprom_content[0x3F] = calculate_checksum(eeprom_content, 0, 63); // re-calculate checksum

    for (uint8_t i = 0; i < 64; i++) // update EEPROM (write value if different)
    {
        EEPROM.update(i, eeprom_content[i]);
    }
    
} // end of write_eeprom_settings


/*************************************************************************
Function: handle_remaining_time()
Purpose:  calculate and convert remaining seconds to time
Note:     sprintf formatting idea commented out at the end
**************************************************************************/
void handle_remaining_time(void)
{
    remaining_time = last_wpump_millis + wpump_interval - current_millis; // milliseconds

    t = remaining_time / 1000; // remaining time in seconds

    if (t <= 359999) // less or equal to 99:59:59 show countdown timer 
    {
        h = t / 3600; // how many integer hours are in these seconds
        t = t % 3600; // remove integer hours, keep remainder
        m = t / 60; // how many integer minutes are in these seconds
        s = t % 60; // remove minutes, keep remainder for final seconds value
    }
    else // show fixed time until it becomes less or equal to 99:59:59
    {
        h = 99;
        m = 59;
        s = 59;
    }

/*
char buf[21];
sprintf(buf, "%02d:%02d:%02d", h, m ,s);
Serial.println(buf);
*/
    
} // end of handle_remaining_time


/*************************************************************************
Function: lcd_init()
Purpose:  initializes LCD
Note:     -
**************************************************************************/
void lcd_init(void)
{
    lcd.begin();     // start LCD
    lcd.backlight(); // backlight on
    lcd.clear();     // clear display
    lcd.home();      // set cursor in home position (0, 0)
    lcd.print(F("--------------------")); // F(" ") makes the compiler store the string inside flash memory instead of RAM, good practice if system is low on RAM
    lcd.setCursor(0, 1);
    lcd.print(F(" MODULAR HYDROPONIC "));
    lcd.setCursor(0, 2);
    lcd.print(F("    TOWER GARDEN    "));
    lcd.setCursor(0, 3);
    lcd.print(F("--------------------"));
    lcd.createChar(0, degree_symbol); // custom character from "degree_symbol" variable with id number 0
    lcd.createChar(1, progress_bar_1); // custom character from "progress_bar_1" variable with id number 1
    lcd.createChar(2, progress_bar_2); // custom character from "progress_bar_2" variable with id number 2
    lcd.createChar(3, progress_bar_3); // custom character from "progress_bar_3" variable with id number 3
    lcd.createChar(4, progress_bar_4); // custom character from "progress_bar_1" variable with id number 4
    lcd.createChar(5, progress_bar_5); // custom character from "progress_bar_1" variable with id number 5
    
} // end of lcd_init


/*************************************************************************
Function: lcd_print_default_layout()
Purpose:  print default text on the LCD
Note:     -
**************************************************************************/
void lcd_print_default_layout(void)
{
    lcd.backlight();  // backlight on
    lcd.clear();      // clear display
    lcd.home();       // set cursor in home position (0, 0)
    lcd.print(F("Water pump: --------")); // F(" ") makes the compiler store the string inside flash memory instead of RAM, good practice if system is low on RAM
    lcd.setCursor(0, 1);
    lcd.print(F("Te:------  Ti:------"));
    lcd.setCursor(0, 2);
    lcd.print(F("Timer: --:--:-- left"));
    lcd.setCursor(0, 3);
    lcd.print(F("----progress bar----"));
    
} // end of lcd_print_default_layout


/*************************************************************************
Function: lcd_print_progress_bar()
Purpose:  prints a progress bar in the third line related to time remaining
Note:     -
**************************************************************************/
void lcd_print_progress_bar(uint8_t percent)
{
    uint8_t value = percent;
    uint8_t num_full_blocks = value / 5; // should be 0...20
    uint8_t remainder = value % 5; // integer division by 5, keep the remainder in this variable, should be 0, 1, 2, 3 or 4
    uint8_t counter = 0; // keep count on how many full blocks are printed

    lcd.setCursor(0, 3);
    for (uint8_t i = 0; i < num_full_blocks; i++)
    {
        lcd.write((uint8_t)5); // print full blocks
        counter++;
    }

    if (percent < 100)
    {
        switch (remainder)
        {
            case 0:
            {
                lcd.print(F(" "));
                break;
            }
            case 1:
            {
                lcd.write((uint8_t)1); // print 1/5th block
                break;
            }
            case 2:
            {
                lcd.write((uint8_t)2); // print 2/5th block
                break;
            }
            case 3:
            {
                lcd.write((uint8_t)3); // print 3/5th block
                break;
            }
            case 4:
            {
                lcd.write((uint8_t)4); // print 4/5th block
                break;
            }
        }

        // Blank space to the end of the line
        for (uint8_t i = 0; i < (19 - counter); i++)
        {
            lcd.print(F(" "));
        }
    }
    
} // end of lcd_print_progress_bar


/*************************************************************************
Function: lcd_print_external_temperature()
Purpose:  prints external temperature to LCD
Note:     -
**************************************************************************/
void lcd_print_external_temperature(void)
{
    switch (temperature_unit)
    {
        case 1: // Celsius
        {
            lcd.setCursor(3, 1);
            
            float TR = roundf(T_ext[1] * 100.0); // float rounded to 2 decimal places
            TR = TR / 100.0;

            if ((TR > -10.0) && (TR < 0.0))
            {
               lcd.print(TR, 1);
            }
            else if ((TR >= 0.0) && (TR < 10.0))
            {
                lcd.print(TR, 2);
            }
            else if ((TR >= 10.0) && (TR < 100.0))
            {
                lcd.print(TR, 1);
            }
            else lcd.print(F("----")); // temperature is out of normal range
            
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("C"));
            break;
        }
        case 2: // Fahrenheit
        {
            lcd.setCursor(3, 1);
            
            float TR = roundf(T_ext[2] * 100.0);  // float rounded to 2 decimal places
            TR = TR / 100.0;

            if ((TR > -10.0) && (TR < 0.0))
            {
               lcd.print(TR, 1);
            }
            else if ((TR >= 0.0) && (TR < 10.0))
            {
                lcd.print(TR, 2);
            }
            else if ((TR >= 10.0) && (TR < 100.0))
            {
                lcd.print(TR, 1);
            }
            else lcd.print(F("----")); // temperature is out of normal range
            
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("F"));
            break;
        }
        case 4: // Kelvin
        {
            lcd.setCursor(3, 1);
            
            float TR = roundf(T_ext[0] * 100.0);  // float rounded to 2 decimal places
            TR = TR / 100.0;
            
            if (TR < 10.0) lcd.print(TR, 3);
            else if ((TR >= 10.0) && (TR < 100.0)) lcd.print(TR, 2);
            else if (TR >= 100.0) lcd.print(TR, 1);
            else lcd.print(F("-----"));
            lcd.print(F("K"));
            break;
        }
    }
    
} // end of lcd_print_external_temperature


/*************************************************************************
Function: lcd_blank_external_temperature()
Purpose:  deletes external temperature from LCD
Note:     -
**************************************************************************/
void lcd_blank_external_temperature(void)
{
    lcd.setCursor(3, 1);
    lcd.print(F("----"));
    
    switch (temperature_unit)
    {
        case 1: // Celsius
        {
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("C"));
            break;
        }
        case 2: // Fahrenheit
        {
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("F"));
            break;
        }
        case 4: // Kelvin
        {
            lcd.print(F("-K"));
            break;
        }
    }
    
} // end of lcd_blank_external_temperature


/*************************************************************************
Function: lcd_print_internal_temperature()
Purpose:  prints internal temperature to LCD
Note:     -
**************************************************************************/
void lcd_print_internal_temperature(void)
{
    switch (temperature_unit)
    {
        case 1: // Celsius
        {
            lcd.setCursor(14, 1);
            
            float TR = roundf(T_int[1] * 100.0);  // float rounded to 2 decimal places
            TR = TR / 100.0;

            if ((TR > -10.0) && (TR < 0.0))
            {
               lcd.print(TR, 1);
            }
            else if ((TR >= 0.0) && (TR < 10.0))
            {
                lcd.print(TR, 2);
            }
            else if ((TR >= 10.0) && (TR < 100.0))
            {
                lcd.print(TR, 1);
            }
            else lcd.print(F("----")); // temperature is out of normal range

            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("C"));
            break;
        }
        case 2: // Fahrenheit
        {
            lcd.setCursor(14, 1);
            
            float TR = roundf(T_int[2] * 100.0);  // float rounded to 2 decimal places
            TR = TR / 100.0;

            if ((TR > -10.0) && (TR < 0.0))
            {
               lcd.print(TR, 1);
            }
            else if ((TR >= 0.0) && (TR < 10.0))
            {
                lcd.print(TR, 2);
            }
            else if ((TR >= 10.0) && (TR < 100.0))
            {
                lcd.print(TR, 1);
            }
            else lcd.print(F("----")); // temperature is out of normal range
    
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("F"));
            break;
        }
        case 4: // Kelvin
        {
            lcd.setCursor(14, 1);
            
            float TR = roundf(T_int[0] * 100.0);  // float rounded to 2 decimal places
            TR = TR / 100.0;
            
            if (TR < 10.0) lcd.print(TR, 3);
            else if ((TR >= 10.0) && (TR < 100.0)) lcd.print(TR, 2);
            else if (TR >= 100.0) lcd.print(TR, 1);
            else lcd.print(F("-----"));
            lcd.print(F("K"));
            break;
        }
    }
    
} // end of lcd_print_internal_temperature


/*************************************************************************
Function: lcd_blank_internal_temperature()
Purpose:  deletes external temperature from LCD
Note:     -
**************************************************************************/
void lcd_blank_internal_temperature(void)
{
    lcd.setCursor(14, 1);
    lcd.print(F("----"));
    
    switch (temperature_unit)
    {
        case 1: // Celsius
        {
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("C"));
            break;
        }
        case 2: // Fahrenheit
        {
            lcd.write((uint8_t)0); // print degree-symbol
            lcd.print(F("F"));
            break;
        }
        case 4: // Kelvin
        {
            lcd.print(F("-K"));
            break;
        }
    }
    
} // end of lcd_blank_internal_temperature


/*****************************************************************************
Function: lcd_update()
Purpose:  printc current information on the LCD
Note:     -
******************************************************************************/
void lcd_update(void)
{
    // Print water pump state in the first line
    if (wpump_on)
    {
        lcd.setCursor(12, 0);
        
        if (!service_mode)
        {
            lcd.print(F("on @"));
            uint8_t percent = (uint8_t)(round(((float)wpump_pwm + 1.0) * 100.0 / 256.0));
            if (percent < 10)
            {
                lcd.print(F("  "));
                lcd.print(percent);
                lcd.print(F("%"));
            }
            else if (percent < 100)
            {
                lcd.print(F(" "));
                lcd.print(percent);
                lcd.print(F("%"));
            }
            else lcd.print(F("100%"));
        }
        else lcd.print(F("off     "));
    }
    else
    {
        lcd.setCursor(12, 0);
        lcd.print(F("off     "));
    }

    // Print temperatures in the second line
    if (enabled_temperature_sensors != 0)
    {
        if ((enabled_temperature_sensors & 0x03) == 3) // both external and internal temperature
        {
            lcd_print_external_temperature();
            lcd_print_internal_temperature();
        }
        else if ((enabled_temperature_sensors & 0x03) == 2) // internal temperature only
        {
            lcd_blank_external_temperature();
            lcd_print_internal_temperature();
        }
        else if ((enabled_temperature_sensors & 0x03) == 1) // external temperature only
        {
            lcd_print_external_temperature();
            lcd_blank_internal_temperature();
        }
    }
    else // there's no enabled temperature sensor
    {
        lcd.setCursor(0, 1);
        lcd.print(F("Te:------  Ti:------"));
    }

    // Print timer information
    if (!service_mode)
    {
        if (wpump_interval != 0)
        {
            handle_remaining_time();

            lcd.setCursor(7, 2);
            if (h < 10) lcd.print(F("0"));
            lcd.print(h);
            lcd.print(F(":"));
            if (m < 10) lcd.print(F("0"));
            lcd.print(m);
            lcd.print(F(":"));
            if (s < 10) lcd.print(F("0"));
            lcd.print(s);
            lcd.print(F(" left"));
        }
        else // show static zero time
        {
            lcd.setCursor(7, 2);
            lcd.print(F("--:--:--"));
        }
    }
    else
    {
        lcd.setCursor(7, 2);
        lcd.print(F("service mode!"));
    }

    // Calculate the ratio of remaining_time and wpump_interval and print a progress bar
    if (wpump_interval != 0) percent_left = (uint8_t)(round(((float)remaining_time / (float)wpump_interval) * 100.0));
    else percent_left = 100;
    lcd_print_progress_bar(percent_left);
    
} // end of lcd_update


/*****************************************************************************
Function: calculate_checksum()
Purpose:  calculates checksum in a given buffer with specified length
Note:     index = first byte in the array to include in calculation
          bufflen = full length of the buffer
******************************************************************************/
uint8_t calculate_checksum(uint8_t *buff, uint16_t index, uint16_t bufflen)
{
    uint8_t a = 0;
    for (uint16_t i = index ; i < bufflen; i++)
    {
        a += buff[i]; // add bytes together
    }
    return a;
    
} // end of calculate_checksum


/*************************************************************************
Function: free_ram()
Purpose:  returns how many bytes exists between the end of the heap and 
          the last allocated memory on the stack, so it is effectively 
          how much the stack/heap can grow before they collide.
**************************************************************************/
uint16_t free_ram(void)
{
    extern int  __bss_end; 
    extern int  *__brkval; 
    uint16_t free_memory; 
    
    if((int)__brkval == 0)
    {
        free_memory = ((int)&free_memory) - ((int)&__bss_end); 
    }
    else 
    {
        free_memory = ((int)&free_memory) - ((int)__brkval); 
    }
    return free_memory; 

} // end of free_ram


/*************************************************************************
Function: send_usb_packet()
Purpose:  assembles and sends data packet through serial link (UART0)
Inputs:   - one source byte,
          - one target byte,
          - one datacode command value byte, these three are used to calculate the DATA CODE byte
          - one SUB-DATA CODE byte,
          - name of the PAYLOAD array (it must be previously filled with data),
          - PAYLOAD length
Returns:  none
Note:     SYNC, LENGTH and CHECKSUM bytes are calculated automatically;
          Payload can be omitted if a (uint8_t*)0x00 value is used in conjunction with 0 length
**************************************************************************/
void send_usb_packet(uint8_t command, uint8_t subdatacode, uint8_t *payloadbuff, uint16_t payloadbufflen)
{
    // Calculate the length of the full packet:
    // PAYLOAD length + 1 SYNC byte + 2 LENGTH bytes + 1 DATA CODE byte + 1 SUB-DATA CODE byte + 1 CHECKSUM byte
    uint16_t packet_length = payloadbufflen + 6;
    bool payload_bytes = true;
    uint8_t datacode = 0;

    // Check if there's enough RAM to store the whole packet
    if (free_ram() < (packet_length + 50)) // require +50 free bytes to be safe
    {
        uint8_t error[7] = { 0x3D, 0x00, 0x03, 0x8F, error_not_enough_mcu_ram, 0xFF, 0x8E }; // prepare the "not enough MCU RAM" error message, error_not_enough_mcu_ram = 0xFD
        for (uint16_t i = 0; i < 7; i++)
        {
            Serial.write(error[i]);
        }
        return;
    }

    uint8_t packet[packet_length]; // create a temporary byte-array

    if (payloadbufflen <= 0) payload_bytes = false;
    else payload_bytes = true;

    // Assemble datacode from input parameter
    datacode = command;
    sbi(datacode, 7); // set highest bit to indicate the packet is coming from the device

    // Start assembling the packet by manually filling the first few slots
    packet[0] = 0x3D; // add SYNC byte
    packet[1] = ((packet_length - 4) >> 8) & 0xFF; // add LENGTH high byte
    packet[2] = (packet_length - 4) & 0xFF; // add LENGTH low byte
    packet[3] = datacode; // add DATA CODE byte
    packet[4] = subdatacode; // add SUB-DATA CODE byte
    
    // If there are payload bytes add them too after subdatacode
    if (payload_bytes)
    {
        for (uint16_t i = 0; i < payloadbufflen; i++)
        {
            packet[5 + i] = payloadbuff[i]; // Add message bytes to the PAYLOAD bytes
        }
    }

    // Calculate checksum
    calculated_checksum = calculate_checksum(packet, 1, packet_length - 1);

    // Place checksum byte
    packet[packet_length - 1] = calculated_checksum;

    // Send the prepared packet through serial link
    for (uint16_t i = 0; i < packet_length; i++)
    {
        Serial.write(packet[i]);
    }

} // end of send_usb_packet


/*****************************************************************************
Function: update_status()
Purpose:  sends a status packet through serial link
Note:     -
******************************************************************************/
void update_status(void)
{
    if (ascii_mode)
    {
        Serial.println();
        Serial.println();
        Serial.println(F("-----------STATUS-----------"));
        
        if (service_mode)
        {
            Serial.println(F("Service mode : enabled"));
        }
        else
        {
            Serial.println(F("Service mode : disabled"));
            Serial.flush();
        }

        if (wpump_on && !service_mode)
        {
            uint8_t wpump_pwm_percent = roundf((float)((wpump_pwm + 1.0) * 100.0 / 256.0));

            Serial.print(F("  Water pump : on @ "));
            Serial.print(wpump_pwm_percent);
            Serial.println(F("%"));
            Serial.flush();
        }
        else
        {
            Serial.println(F("  Water pump : off"));
        }

        Serial.flush();
        Serial.print(F("       Timer : "));

        if (!service_mode)
        {
            if (wpump_interval != 0)
            {
                handle_remaining_time();
                
                if (h < 10) Serial.print(F("0"));
                Serial.print(h);
                Serial.print(F(":"));
                if (m < 10) Serial.print(F("0"));
                Serial.print(m);
                Serial.flush();
                Serial.print(F(":"));
                if (s < 10) Serial.print(F("0"));
                Serial.print(s);
                Serial.println(F(" left"));
            }
            else // show static zero time
            {
                Serial.println(F("--:--:--"));
            }
        }
        else
        {
            Serial.println(F("--:--:--"));
        }
        Serial.flush();

        if (enabled_temperature_sensors & 0x03)
        {
            Serial.println(F("Temperatures"));

            if (enabled_temperature_sensors & 0x01)
            {
                Serial.print(F("    external : "));
                Serial.flush();

                switch (temperature_unit)
                {
                    case 1: // Celsius
                    {
                        float TR = roundf(T_ext[1] * 100.0);  // float rounded to 2 decimal places
                        TR = TR / 100.0;
                        
                        Serial.print(TR, 1);
                        Serial.print(F("°C"));
                        break;
                    }
                    case 2: // Fahrenheit
                    {
                        float TR = roundf(T_ext[2] * 100.0);  // float rounded to 2 decimal places
                        TR = TR / 100.0;
                        
                        Serial.print(TR, 1);
                        Serial.print(F("°F"));
                        break;
                    }
                    case 4: // Kelvin
                    {
                        float TR = roundf(T_ext[0] * 100.0);  // float rounded to 2 decimal places
                        TR = TR / 100.0;
                        
                        Serial.print(TR, 1);
                        Serial.print(F("K"));
                        break;
                    }
                }

                Serial.println();
                Serial.flush();
            }

            if (enabled_temperature_sensors & 0x02)
            {
                Serial.print(F("    internal : "));
                Serial.flush();

                switch (temperature_unit)
                {
                    case 1: // Celsius
                    {
                        float TR = roundf(T_int[1] * 100.0);  // float rounded to 2 decimal places
                        TR = TR / 100.0;
                        
                        Serial.print(TR, 1);
                        Serial.print(F("°C"));
                        break;
                    }
                    case 2: // Fahrenheit
                    {
                        float TR = roundf(T_int[2] * 100.0);  // float rounded to 2 decimal places
                        TR = TR / 100.0;
                        
                        Serial.print(TR, 1);
                        Serial.print(F("°F"));
                        break;
                    }
                    case 4: // Kelvin
                    {
                        float TR = roundf(T_int[0] * 100.0);  // float rounded to 2 decimal places
                        TR = TR / 100.0;
                        
                        Serial.print(TR, 1);
                        Serial.print(F("K"));
                        break;
                    }
                }

                Serial.println();
                Serial.flush();
            }
        }

        Serial.print(F("----------------------------"));
        Serial.flush();
    }
    else
    {
        // Gather data and send back to laptop (water pump state, water pump pwm-level, remaining time until state change, temperatures)
        uint8_t status_payload[14];
        uint8_t remaining_time_array[4];

        handle_remaining_time();

        if (!service_mode)
        {
            if (wpump_interval > 0)
            {
                remaining_time_array[0] = (remaining_time >> 24) & 0xFF;
                remaining_time_array[1] = (remaining_time >> 16) & 0xFF;
                remaining_time_array[2] = (remaining_time >> 8) & 0xFF;
                remaining_time_array[3] = remaining_time & 0xFF;
            }
            else
            {
                remaining_time_array[0] = 0;
                remaining_time_array[1] = 0;
                remaining_time_array[2] = 0;
                remaining_time_array[3] = 0;
            }
        }
        else if (service_mode)
        {
            remaining_time_array[0] = 0;
            remaining_time_array[1] = 0;
            remaining_time_array[2] = 0;
            remaining_time_array[3] = 0;
        }
        
        status_payload[0] = 0;
        if (wpump_on) sbi(status_payload[0], 0);
        else cbi(status_payload[0], 0);
        if (service_mode) sbi(status_payload[0], 1);
        else cbi(status_payload[0], 1);
        
        status_payload[1] = wpump_pwm;
        
        status_payload[2] = remaining_time_array[0];
        status_payload[3] = remaining_time_array[1];
        status_payload[4] = remaining_time_array[2];
        status_payload[5] = remaining_time_array[3];
        
        if (enabled_temperature_sensors & 0x01) // external temperature sensor
        {
            status_payload[6] = temperature_payload[0];
            status_payload[7] = temperature_payload[1];
            status_payload[8] = temperature_payload[2];
            status_payload[9] = temperature_payload[3];
        }
        else
        {
            status_payload[6] = 0;
            status_payload[7] = 0;
            status_payload[8] = 0;
            status_payload[9] = 0;
        }
    
        if (enabled_temperature_sensors & 0x02) // internal temperature sensor
        {
            status_payload[10] = temperature_payload[4];
            status_payload[11] = temperature_payload[5];
            status_payload[12] = temperature_payload[6];
            status_payload[13] = temperature_payload[7];
        }
        else
        {
            status_payload[10] = 0;
            status_payload[11] = 0;
            status_payload[12] = 0;
            status_payload[13] = 0;
        }
        
        send_usb_packet(status, ok, status_payload, 14);
    }
    
} // end of update_status


/*************************************************************************
Function: send_hwfw_info()
Purpose:  gathers hardware version/date, assembly date and firmware date
          into an array and send through serial link
Note:     -
**************************************************************************/
void send_hwfw_info(void)
{
    uint8_t ret[26];
                                    
    ret[0] = hw_version[0];
    ret[1] = hw_version[1];
    
    ret[2] = hw_date[0];
    ret[3] = hw_date[1];
    ret[4] = hw_date[2];
    ret[5] = hw_date[3];
    ret[6] = hw_date[4];
    ret[7] = hw_date[5];
    ret[8] = hw_date[6];
    ret[9] = hw_date[7];

    ret[10] = assembly_date[0];
    ret[11] = assembly_date[1];
    ret[12] = assembly_date[2];
    ret[13] = assembly_date[3];
    ret[14] = assembly_date[4];
    ret[15] = assembly_date[5];
    ret[16] = assembly_date[6];
    ret[17] = assembly_date[7];
    
    ret[18] = (FW_DATE >> 56) & 0xFF;
    ret[19] = (FW_DATE >> 48) & 0xFF;
    ret[20] = (FW_DATE >> 40) & 0xFF;
    ret[21] = (FW_DATE >> 32) & 0xFF;
    ret[22] = (FW_DATE >> 24) & 0xFF;
    ret[23] = (FW_DATE >> 16) & 0xFF;
    ret[24] = (FW_DATE >> 8) & 0xFF;
    ret[25] = FW_DATE & 0xFF;

    send_usb_packet(response, 0x02, ret, 26);
    
} // end of evaluate_eep_checksum


/*************************************************************************
Function: handle_usb_data()
Purpose:  handles USB commands coming from an external computer
Note:     [ SYNC | LENGTH_HB | LENGTH_LB | DATACODE | SUBDATACODE | <?PAYLOAD?> | CHECKSUM ]
**************************************************************************/
void handle_usb_data(void)
{
    if (Serial.available() > 0) // proceed only if the receive buffer contains at least 1 byte
    {
        uint8_t sync = Serial.read(); // read the next available byte in the USB receive buffer, it's supposed to be the first byte of a message

        if (sync == 0x3D) // packet-based communication
        {
            ascii_mode = false;
            
            uint8_t length_hb, length_lb, datacode, subdatacode, checksum;
            bool payload_bytes = false;
            uint16_t bytes_to_read = 0;
            uint16_t payload_length = 0;
            uint8_t calculated_checksum = 0;
    
            uint32_t command_timeout_start = 0;
            bool command_timeout_reached = false;
    
            // Wait for the length bytes to arrive (2 bytes)
            command_timeout_start = millis(); // save current time
            while ((Serial.available() < 2) && !command_timeout_reached)
            {
                if (millis() - command_timeout_start > command_purge_timeout) command_timeout_reached = true;
            }
            if (command_timeout_reached)
            {
                send_usb_packet(ok_error, error_packet_timeout_occured, err, 1);
                return;
            }
    
            length_hb = Serial.read(); // read first length byte
            length_lb = Serial.read(); // read second length byte
    
            // Calculate how much more bytes should we read by combining the two length bytes into a word.
            bytes_to_read = to_uint16(length_hb, length_lb) + 1; // +1 CHECKSUM byte
            
            // Calculate the exact size of the payload.
            payload_length = bytes_to_read - 3; // in this case we have to be careful not to count data code byte, sub-data code byte and checksum byte
    
            // Do not let this variable sink below zero.
            if (payload_length < 0) payload_length = 0; // !!!
    
            // Maximum packet length is 1024 bytes; can't accept larger packets 
            // and can't accept packet without datacode and subdatacode.
            if ((payload_length > 1018) || ((bytes_to_read - 1) < 2))
            {
                send_usb_packet(ok_error, error_length_invalid_value, err, 1);
                return; // exit, let the loop call this function again
            }
    
            // Wait here until all of the expected bytes are received or timeout occurs.
            command_timeout_start = millis();
            while ((Serial.available() < bytes_to_read) && !command_timeout_reached) 
            {
                if (millis() - command_timeout_start > command_timeout) command_timeout_reached = true;
            }
            if (command_timeout_reached)
            {
                send_usb_packet(ok_error, error_packet_timeout_occured, err, 1);
                return; // exit, let the loop call this function again
            }
    
            // There's at least one full command in the buffer now.
            // Go ahead and read one DATA CODE byte (next in the row).
            datacode = Serial.read();
    
            // Read one SUB-DATA CODE byte that's following.
            subdatacode = Serial.read();
    
            // Make some space for the payload bytes (even if there is none).
            uint8_t cmd_payload[payload_length];
    
            // If the payload length is greater than zero then read those bytes too.
            if (payload_length > 0)
            {
                // Read all the PAYLOAD bytes
                for (uint16_t i = 0; i < payload_length; i++)
                {
                    cmd_payload[i] = Serial.read();
                }
                // And set flag so the rest of the code knows.
                payload_bytes = true;
            }
            // Set flag if there are no PAYLOAD bytes available.
            else payload_bytes = false;
    
            // Read last CHECKSUM byte.
            checksum = Serial.read();
    
            // Verify the received packet by calculating what the checksum byte should be.
            calculated_checksum = length_hb + length_lb + datacode + subdatacode; // add the first few bytes together manually
    
            // Add payload bytes here together if present
            if (payload_bytes)
            {
                for (uint16_t j = 0; j < payload_length; j++)
                {
                    calculated_checksum += cmd_payload[j];
                }
            }

            // Compare calculated checksum to the received CHECKSUM byte
            if (calculated_checksum != checksum) // if they are not the same
            {
                send_usb_packet(ok_error, error_packet_checksum_invalid_value, err, 1);
                return; // exit, let the loop call this function again
            }

            // Extract command value from the low nibble (lower 4 bits).
            uint8_t command = datacode & 0x0F;
        
            // Source is ignored, the packet must come from an external computer through USB
            switch (command) // evaluate command
            {
                case reset: // 0x00 - reset device request
                {
                    send_usb_packet(reset, 0x00, ack, 1); // confirm action
                    while (true); // enter into an infinite loop; watchdog timer doesn't get reset this way so it restarts the program eventually
                    break; // not necessary but every case needs a break
                }
                case handshake: // 0x01 - handshake request coming from an external computer
                {
                    uint8_t handshake_payload[4] = { 0x4D, 0x48, 0x54, 0x47 }; // MHTG
                    send_usb_packet(handshake, 0x00, handshake_payload, 4);
                    break;
                }
                case status: // 0x02 - status report request
                {
                    update_status();
                    break;
                }
                case settings: // 0x03 - change device settings
                {
                    switch (subdatacode) // evaluate SUB-DATA CODE byte
                    {
                        case 0x01: // read settings, a proportion of the internal EEPROM
                        {
                            uint8_t settings_payload[15];

                            for (uint8_t i = 0; i < 15; i++) // copy the last 14 bytes from the current eeprom_content array
                            {
                                settings_payload[i] = eeprom_content[0x12 + i];
                            }

                            send_usb_packet(settings, 0x01, settings_payload, 15);
                            break;
                        }
                        case 0x02: // write settings
                        {
                            if (payload_length < 15)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            for (uint8_t i = 0; i < 15; i++) // overwrite settings in the temporary EEPROM array
                            {
                                eeprom_content[0x12 + i] = cmd_payload[i];
                            }

                            write_eeprom_settings();
                            update_settings(); // actualize settings
                            send_usb_packet(settings, 0x02, ack, 1);
                            break;
                        }
                        case 0x03: // autoupdate enable/disable
                        {
                            if (!payload_bytes)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            if (cmd_payload[0] == 0x00) autoupdate = false;
                            else autoupdate = true;
                            send_usb_packet(settings, 0x03, cmd_payload, 1);
                            break;
                        }
                        case 0x04: // service mode on/off
                        {
                            if (!payload_bytes)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            if (cmd_payload[0] & 0x01) // request to enter into service mode
                            {
                                if (!service_mode)
                                {
                                    service_mode = true; // enter service mode
                                    
                                    if (wpump_on)
                                    {
                                        change_wpump_speed(0); // turn off water pump
                                    }
                                }
                            }
                            else // exit service mode
                            {
                                service_mode = false; // exit service mode

                                if (wpump_on) // if water pump is on according to the timer
                                {
                                    wpump_on = true;
                                    change_wpump_speed(wpump_pwm); // resume water pump activity
                                }
                    
                                if (!wpump_on) digitalWrite(RED_LED_PIN, HIGH); // turn off red LED is this condition catches it during blinking
                            }
                            
                            send_usb_packet(settings, 0x04, cmd_payload, 1);
                            break;
                        }
                        case 0x05: // water pump on/off
                        {
                            if (!payload_bytes)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            if (service_mode)
                            {
                                send_usb_packet(settings, 0x05, cmd_payload, 1);
                                
                                if (cmd_payload[0] == 0x00) wpump_on = false;
                                else wpump_on = true;
                                
                                change_wpump_speed(cmd_payload[0]);
                            }
                            else
                            {
                                send_usb_packet(ok_error, error_internal, err, 1);
                            }
                            break;
                        }
                        default: // other values are not used
                        {
                            send_usb_packet(ok_error, error_subdatacode_invalid_value, err, 1);
                            break;
                        }
                    }
                    break;
                }
                case request: // 0x04 - request data from the device
                {
                    switch (subdatacode) // evaluate SUB-DATA CODE byte
                    {
                        case 0x01: // EEPROM content (the first 32 bytes are relevant to us)
                        {
                            send_usb_packet(response, 0x01, eeprom_content, 32);
                            break;
                        }
                        case 0x02: // hardware/firmware version
                        {
                            send_hwfw_info();
                            break;
                        }
                        case 0x03: // timestamp
                        {
                            update_timestamp(current_timestamp); // this function updates the global byte array "current_timestamp" with the current time
                            send_usb_packet(response, 0x03, current_timestamp, 4);
                            break;
                        }
                        case 0x04: // temperatures
                        {
                            send_usb_packet(response, 0x04, temperature_payload, 8);
                            break;
                        }
                        default: // other values are not used
                        {
                            send_usb_packet(ok_error, error_subdatacode_invalid_value, err, 1);
                            break;
                        }
                    }
                    break;
                }
                case debug: // 0x0E - experimental stuff are first tested here
                {
                    switch (subdatacode) // evaluate SUB-DATA CODE byte
                    {
                        case 0x01: // read EEPROM
                        {
                            // BUG: can't read the whole 1024 bytes in one go...
                            
                            if (payload_length < 4)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            uint16_t index = to_uint16(cmd_payload[0], cmd_payload[1]); // start index in EEPROM to read
                            uint16_t count = to_uint16(cmd_payload[2], cmd_payload[3]); // number of bytes to read starting at index

                            if ((index + count - 1) < 1024) // ATmega328P has 1024 bytes of EEPROM
                            {
                                uint8_t eeprom_values[count + 4]; // temporary array to store values + 2 address bytes and 2 count bytes at the beginning
                                eeprom_values[0] = cmd_payload[0];
                                eeprom_values[1] = cmd_payload[1];
                                eeprom_values[2] = cmd_payload[2];
                                eeprom_values[3] = cmd_payload[3];

                                for (uint16_t i = 0; i < count; i++)
                                {
                                    eeprom_values[4 + i] = EEPROM.read(index + i);
                                }

                                send_usb_packet(debug, 0x01, eeprom_values, count + 4);
                            }
                            else // otherwise at some point the read address will exceed EEPROM size
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            break;
                        }
                        case 0x02: // write EEPROM
                        {
                            if (payload_length < 3)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            uint16_t index = to_uint16(cmd_payload[0], cmd_payload[1]); // start index in EEPROM to write
                            uint16_t count = payload_length - 2;

                            if ((index + count - 1) < 1024) // ATmega328P has 1024 bytes of EEPROM
                            {
                                for (uint16_t i = 0; i < count; i++)
                                {
                                    EEPROM.update(index + i, cmd_payload[2 + i]);
                                }

                                // Checksum at 0x3F is not re-calculated automatically here! Make sure to write the correct value with the data above or with a second go!

                                send_usb_packet(debug, 0x02, ack, 1);
                            }
                            else
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            break;
                        }
                        case 0x03: // update water pump driver PWM-frequency
                        {
                            if (payload_length < 2)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                break;
                            }

                            // Update the local RAM-copy of the EEPROM
                            eeprom_content[0x1F] = cmd_payload[0];
                            eeprom_content[0x20] = cmd_payload[1];
                            
                            write_eeprom_settings(); // write settings to internal EEPROM
                            update_settings(); // actualize settings

                            send_usb_packet(debug, 0x03, ack, 1);
                            break;
                        }
                        default: // other values are not used
                        {
                            send_usb_packet(ok_error, error_subdatacode_invalid_value, err, 1);
                            break;
                        }
                    }
                    break;
                }
                default: // other values are not used
                {
                    send_usb_packet(ok_error, error_datacode_invalid_command, err, 1);
                    break;
                }
            }
        }
        else // text-based communication
        {
            if ((sync != '\n') && (sync != '\r')) // filter out blank lines
            {
                ascii_mode = true;
                
                rxBuffer[0] = sync; // store first character before further reading
                rxBuffer[1] = '\0';
                rxBufferPtr = 1;
    
                delay(50); // wait a little for the rest of the characters to arrive
                
                while (Serial.available())
                {
                    char c = Serial.read(); // read next character
                    
                    if ((c != '\n') && (c != '\r') && (rxBufferPtr < (sizeof(rxBuffer) - 1)))
                    {
                        rxBuffer[rxBufferPtr] = c; // store character
                        rxBufferPtr++; // increment pointer value
                        rxBuffer[rxBufferPtr] = '\0'; // insert string termination character after the current character
                    }
                    else
                    {
                        while (Serial.available()) Serial.read(); // read remaining characters into oblivion
                    }
                }

                char command[32];
                char value[32];
                uint32_t valueNumeric = 0;
                uint8_t delimiterPos = 0;
                bool valuePresent = false;
    
                for (uint8_t i = 0; i < rxBufferPtr; i++) // find '=' symbol
                {
                    if (rxBuffer[i] == '=')
                    {
                        delimiterPos = i;
                        break;
                    }
                }
    
                if (delimiterPos != 0) // command and value are both present
                {
                    valuePresent = true;
                    uint8_t valuePos = delimiterPos + 1;
                    uint8_t valueLen = rxBufferPtr - valuePos;

                    for (uint8_t i = 0; i < delimiterPos; i++)
                    {
                        command[i] = rxBuffer[i];
                    }

                    command[delimiterPos] = '\0';

                    for (uint8_t i = 0; i < valueLen; i++)
                    {
                        value[i] = rxBuffer[valuePos + i];
                    }

                    value[valueLen] = '\0';
                    valueNumeric = atoi(value);
                }
                else // only command is present
                {
                    valuePresent = false;
    
                    for (uint8_t i = 0; i < rxBufferPtr ; i++)
                    {
                        command[i] = rxBuffer[i];
                    }
    
                    command[rxBufferPtr] = '\0';
                }

                Serial.println();
                Serial.println();
                Serial.print(F("Input: "));
                Serial.println(rxBuffer);
                Serial.flush();
                Serial.print(F("Command: "));
                Serial.print(command);
    
                if (valuePresent)
                {
                    Serial.println();
                    Serial.print(F("Value: "));
                    Serial.print(valueNumeric);
                }

                Serial.flush();

                if ((strcmp(command, "help") == 0) || (strcmp(command, "HELP") == 0)) // print command descriptions to the Serial Monitor
                {
                    Serial.println();
                    Serial.println();
                    Serial.flush();
                    Serial.println(F("----------------------------------HELP---------------------------------"));
                    Serial.println(F("         help : print this description."));
                    Serial.println(F("      restart : restart controller."));
                    Serial.flush();
                    Serial.println(F("         info : print controller information."));
                    Serial.println(F("servicemode=X : set service mode,"));
                    Serial.println(F("                X=0: disabled (default),"));
                    Serial.flush();
                    Serial.println(F("                X=1: enabled."));
                    Serial.println(F("     status=X : print regular status updates,"));
                    Serial.println(F("                X=0: disabled,"));
                    Serial.flush();
                    Serial.println(F("                X=1: enabled (default)."));
                    Serial.println(F("  pumpspeed=X : set water pump speed in percents,"));
                    Serial.println(F("                X=0...100 [%]."));
                    Serial.flush();
                    Serial.println(F("     ontime=X : set water pump on-time in minutes,"));
                    Serial.println(F("                X=0...71582 [min],"));
                    Serial.println(F("                when set to 0 the water pump is permanently turned off."));
                    Serial.flush();
                    Serial.println(F("    offtime=X : set water pump off-time in minutes,"));
                    Serial.println(F("                X=0...71582 [min],"));
                    Serial.println(F("                when set to 0 the water pump is permanently turned on."));
                    Serial.flush();
                    Serial.println(F("tempsensors=X : set temperature sensor configuration,"));
                    Serial.println(F("                X=0: none (default),"));
                    Serial.println(F("                X=1: external,"));
                    Serial.flush();
                    Serial.println(F("                X=2: internal,"));
                    Serial.println(F("                X=3: both external and internal."));
                    Serial.println(F("   tempunit=X : set temperature unit,"));
                    Serial.flush();
                    Serial.println(F("                X=1: Celsius (default),"));
                    Serial.println(F("                X=2: Fahrenheit,"));
                    Serial.println(F("                X=4: Kelvin."));
                    Serial.flush();
                    Serial.println(F("    ntcbeta=X : set temperature sensor beta value in Kelvin,"));
                    Serial.println(F("                X=3950 [K] by default."));
                    Serial.println(F("    pwmfreq=X : set water pump driver PWM frequency in Hertz,"));
                    Serial.flush();
                    Serial.println(F("                X=490 [Hz] by default."));
                    Serial.println();
                    Serial.println(F("Note: settings are automatically saved to the internal EEPROM."));
                    Serial.print(F("-----------------------------------------------------------------------"));
                    Serial.flush();
                    
                }
                else if ((strcmp(command, "restart") == 0) || (strcmp(command, "RESTART") == 0)) // restart controller
                {
                    Serial.println();
                    Serial.print(F("Result: restarting controller now, please wait."));
                    while(true); // enter into infinite loop, wait for watchdog reset
                }
                else if ((strcmp(command, "info") == 0) || (strcmp(command, "INFO") == 0)) // print controller information to the Serial Monitor
                {
                    Serial.println();
                    Serial.println();
                    Serial.flush();
                    Serial.println(F("--------------------INFO---------------------"));
                    Serial.print(F("        AVR signature : "));
                    Serial.flush();
                    
                    for (uint8_t i = 0; i < sizeof(avr_signature); i++)
                    {
                        if (avr_signature[i] < 16) Serial.print(F("0")); // print leading zero
                        Serial.print(avr_signature[i], HEX); // print signature byte in hexadecimal format
                        Serial.print(F(" ")); // insert whitespace between bytes
                    }

                    Serial.flush();

                    if ((avr_signature[0] == 0x1E) && (avr_signature[1] == 0x95) && (avr_signature[2] == 0x0F))
                    {
                        Serial.println(F("(ATmega328P)"));
                    }
                    else
                    {
                        Serial.println(F("(unknown microcontroller)"));
                    }

                    Serial.print(F("     Hardware version : V"));
                    float HWVersion = to_uint16(hw_version[0], hw_version[1]) / 100.0;
                    Serial.println(HWVersion, 2);
                    Serial.flush();

                    time_t hardwareDate = to_uint32(hw_date[4], hw_date[5], hw_date[6], hw_date[7]);
                    setTime(hardwareDate);
                    Serial.print(F("        Hardware date : "));
                    Serial.print(year());
                    Serial.print(F("."));
                    Serial.flush();
                    if (month() < 10) Serial.print(F("0"));
                    Serial.print(month());
                    Serial.print(F("."));
                    Serial.flush();
                    if (day() < 10) Serial.print(F("0"));
                    Serial.print(day());
                    Serial.print(F(" "));
                    Serial.flush();
                    if (hour() < 10) Serial.print(F("0"));
                    Serial.print(hour());
                    Serial.print(F(":"));
                    Serial.flush();
                    if (minute() < 10) Serial.print(F("0"));
                    Serial.print(minute());
                    Serial.print(F(":"));
                    Serial.flush();
                    if (second() < 10) Serial.print(F("0"));
                    Serial.println(second());

                    time_t assemblyDate = to_uint32(assembly_date[4], assembly_date[5], assembly_date[6], assembly_date[7]);
                    setTime(assemblyDate);
                    Serial.print(F("        Assembly date : "));
                    Serial.print(year());
                    Serial.print(F("."));
                    Serial.flush();
                    if (month() < 10) Serial.print(F("0"));
                    Serial.print(month());
                    Serial.print(F("."));
                    Serial.flush();
                    if (day() < 10) Serial.print(F("0"));
                    Serial.print(day());
                    Serial.print(F(" "));
                    Serial.flush();
                    if (hour() < 10) Serial.print(F("0"));
                    Serial.print(hour());
                    Serial.print(F(":"));
                    Serial.flush();
                    if (minute() < 10) Serial.print(F("0"));
                    Serial.print(minute());
                    Serial.print(F(":"));
                    Serial.flush();
                    if (second() < 10) Serial.print(F("0"));
                    Serial.println(second());

                    time_t firmwareDate = FW_DATE & 0xFFFFFFFF;
                    setTime(firmwareDate);
                    Serial.print(F("        Firmware date : "));
                    Serial.print(year());
                    Serial.print(F("."));
                    Serial.flush();
                    if (month() < 10) Serial.print(F("0"));
                    Serial.print(month());
                    Serial.print(F("."));
                    Serial.flush();
                    if (day() < 10) Serial.print(F("0"));
                    Serial.print(day());
                    Serial.print(F(" "));
                    Serial.flush();
                    if (hour() < 10) Serial.print(F("0"));
                    Serial.print(hour());
                    Serial.print(F(":"));
                    Serial.flush();
                    if (minute() < 10) Serial.print(F("0"));
                    Serial.print(minute());
                    Serial.print(F(":"));
                    Serial.flush();
                    if (second() < 10) Serial.print(F("0"));
                    Serial.println(second());

                    if (service_mode)
                    {
                        Serial.println(F("         Service mode : enabled"));
                    }
                    else
                    {
                        Serial.println(F("         Service mode : disabled"));
                        Serial.flush();
                    }

                    Serial.flush();
            
                    if (wpump_on && !service_mode)
                    {
                        uint8_t wpump_pwm_percent = roundf((float)((wpump_pwm + 1.0) * 100.0 / 256.0));
            
                        Serial.print(F("     Water pump state : on @ "));
                        Serial.print(wpump_pwm_percent);
                        Serial.println(F("%"));
                        Serial.flush();
                    }
                    else
                    {
                        Serial.println(F("     Water pump state : off"));
                    }

                    Serial.flush();

                    Serial.print(F("   Water pump on-time : "));
                    Serial.print(wpump_ontime / 60000);
                    Serial.println(F(" minute(s)"));
                    Serial.flush();
                    Serial.print(F("  Water pump off-time : "));
                    Serial.print(wpump_offtime / 60000);
                    Serial.println(F(" minute(s)"));
                    Serial.flush();
                    
                    Serial.print(F("Temperature sensor(s) : "));

                    if ((enabled_temperature_sensors & 0x03) == 0) Serial.println(F("none"));
                    else if ((enabled_temperature_sensors & 0x03) == 1) Serial.println(F("external"));
                    else if ((enabled_temperature_sensors & 0x03) == 2) Serial.println(F("internal"));
                    else if ((enabled_temperature_sensors & 0x03) == 3) Serial.println(F("external and internal"));
                    else Serial.println(F("N/A"));
                    Serial.flush();
                    
                    Serial.print(F("     Temperature unit : "));

                    if ((temperature_unit & 0x07) == 1) Serial.println(F("Celsius"));
                    else if ((temperature_unit & 0x07) == 2) Serial.println(F("Fahrenheit"));
                    else if ((temperature_unit & 0x07) == 4) Serial.println(F("Kelvin"));
                    else Serial.println(F("N/A"));
                    Serial.flush();

                    Serial.print(F("       NTC beta value : "));
                    Serial.print(beta);
                    Serial.println(F(" K"));
                    Serial.flush();

                    Serial.print(F("        PWM frequency : "));
                    Serial.print(pwm_frequency);
                    Serial.println(F(" Hz"));
                    Serial.flush();
                    Serial.print(F("---------------------------------------------"));
                    Serial.flush();
                }
                else if ((strcmp(command, "servicemode") == 0) || (strcmp(command, "SERVICEMODE") == 0)) // set service mode
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if (valueNumeric == 0)
                    {
                        service_mode = false;
                        Serial.println();
                        Serial.print(F("Result: service mode disabled, water pump is controlled by the timer."));
                    }
                    else if (valueNumeric == 1)
                    {
                        service_mode = true;
                        Serial.println();
                        Serial.print(F("Result: service mode enabled, water pump is temporary turned off."));
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value!"));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "status") == 0) || (strcmp(command, "STATUS") == 0)) // print regular status updates on the Serial Monitor
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if (valueNumeric == 0)
                    {
                        autoupdate = false;
                        Serial.println();
                        Serial.print(F("Result: regular status updates disabled."));
                    }
                    else if (valueNumeric == 1)
                    {
                        autoupdate = true;
                        Serial.println();
                        Serial.print(F("Result: regular status update enabled."));
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value!"));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "pumpspeed") == 0) || (strcmp(command, "PUMPSPEED") == 0)) // set water pump speed (0-100%)
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if ((valueNumeric >=0) && (valueNumeric <= 100))
                    {
                        Serial.println();
                        Serial.print(F("Note: please wait while the water pump speed is changing."));
                        
                        if (valueNumeric > 0 ) eeprom_content[0x12] = (uint8_t)(valueNumeric * 256 / 100 - 1);
                        else eeprom_content[0x12] = 0;
                        write_eeprom_settings();
                        update_settings(); // actualize settings, ramp up or down the water pump driver

                        Serial.println();
                        Serial.print(F("Result: water pump speed changed to "));
                        Serial.print(valueNumeric);
                        Serial.print(F("%."));
                    }
                    else
                    {
                        uint8_t wpump_pwm_percent = roundf((float)((wpump_pwm + 1.0) * 100.0 / 256.0));

                        Serial.println();
                        Serial.print(F("Result: invalid value! Water pump speed remains at "));
                        Serial.print(wpump_pwm_percent);
                        Serial.print(F("%."));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "ontime") == 0) || (strcmp(command, "ONTIME") == 0)) // set water pump on-time
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if ((valueNumeric >=0) && (valueNumeric <= 71582))
                    {
                        uint32_t t = valueNumeric * 60000; // milliseconds
                        eeprom_content[0x13] = (t >> 24) & 0xFF;
                        eeprom_content[0x14] = (t >> 16) & 0xFF;
                        eeprom_content[0x15] = (t >> 8) & 0xFF;
                        eeprom_content[0x16] = t & 0xFF;
                        write_eeprom_settings();
                        update_settings(); // actualize settings

                        Serial.println();
                        Serial.print(F("Result: water pump on-time changed to "));
                        Serial.print(valueNumeric);
                        Serial.print(F(" minute(s)."));
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value! Water pump on-time must be less than 71582 minutes."));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "offtime") == 0) || (strcmp(command, "OFFTIME") == 0)) // set water pump off-time
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if ((valueNumeric >=0) && (valueNumeric <= 71582))
                    {
                        uint32_t t = valueNumeric * 60000; // milliseconds
                        eeprom_content[0x17] = (t >> 24) & 0xFF;
                        eeprom_content[0x18] = (t >> 16) & 0xFF;
                        eeprom_content[0x19] = (t >> 8) & 0xFF;
                        eeprom_content[0x1A] = t & 0xFF;
                        write_eeprom_settings();
                        update_settings(); // actualize settings

                        Serial.println();
                        Serial.print(F("Result: water pump off-time changed to "));
                        Serial.print(valueNumeric);
                        Serial.print(F(" minute(s)."));
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value! Water pump off-time must be less than 71582 minutes."));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "tempsensors") == 0) || (strcmp(command, "TEMPSENSORS") == 0)) // set temperature sensor configuration
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if ((valueNumeric >= 0) && (valueNumeric <= 3))
                    {
                        eeprom_content[0x1B] = valueNumeric;
                        write_eeprom_settings();
                        update_settings(); // actualize settings

                        if (valueNumeric == 0)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature sensor configuration is set to none."));
                        }
                        else if (valueNumeric == 1)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature sensor configuration is set to external sensor only."));
                        }
                        else if (valueNumeric == 2)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature sensor configuration is set to internal sensor only."));
                        }
                        else if (valueNumeric == 3)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature sensor configuration is set to both external and internal sensors."));
                        }
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value!"));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "tempunit") == 0) || (strcmp(command, "TEMPUNIT") == 0)) // set temperature unit
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if (((valueNumeric & 0x07) >= 1) && ((valueNumeric & 0x07) <= 4) && ((valueNumeric & 0x07) != 3))
                    {
                        eeprom_content[0x1C] = valueNumeric;
                        write_eeprom_settings();
                        update_settings(); // actualize settings

                        if (valueNumeric == 1)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature unit set to Celsius."));
                        }
                        else if (valueNumeric == 2)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature unit set to Fahrenheit."));
                        }
                        else if (valueNumeric == 4)
                        {
                            Serial.println();
                            Serial.print(F("Result: temperature unit set to Kelvin."));
                        }
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value!"));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "ntcbeta") == 0) || (strcmp(command, "NTCBETA") == 0)) // set NTC beta value
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if (valueNumeric > 0)
                    {
                        eeprom_content[0x1D] = (valueNumeric >> 8) & 0xFF;
                        eeprom_content[0x1E] = valueNumeric & 0xFF;
                        write_eeprom_settings();
                        update_settings(); // actualize settings

                        Serial.println();
                        Serial.print(F("Result: NTC beta value changed to "));
                        Serial.print(valueNumeric);
                        Serial.print(F(" K."));
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value! NTC beta value must be greater than 0."));
                    }

                    Serial.flush();
                }
                else if ((strcmp(command, "pwmfreq") == 0) || (strcmp(command, "PWMFREQ") == 0)) // set PWM frequency
                {
                    if (!valuePresent)
                    {
                        Serial.println();
                        Serial.print(F("Result: error!"));
                        Serial.flush();
                        return;
                    }

                    if (valueNumeric > 0)
                    {
                        eeprom_content[0x1F] = (valueNumeric >> 8) & 0xFF;
                        eeprom_content[0x20] = valueNumeric & 0xFF;
                        write_eeprom_settings();
                        update_settings(); // actualize settings

                        Serial.println();
                        Serial.print(F("Result: PWM frequency changed to "));
                        Serial.print(valueNumeric);
                        Serial.print(F(" Hz."));
                    }
                    else
                    {
                        Serial.println();
                        Serial.print(F("Result: invalid value! PWM frequency must be greater than 0."));
                    }

                    Serial.flush();
                }
                else
                {
                    Serial.println();
                    Serial.print(F("Result: unknown command!"));
                }

                Serial.flush();
            }
        }
    }
    else
    {
        // what TODO if nothing is in the serial receive buffer
    }

} // end of handle_usb_data


void setup()
{
    sei(); // enable interrupts
    Serial.begin(250000);

    analogReference(DEFAULT);

    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, HIGH); // turn off red LED
    pinMode(WATERPUMP_PIN, OUTPUT);
    digitalWrite(WATERPUMP_PIN, LOW); // turn off water pump

    attachInterrupt(digitalPinToInterrupt(2), handle_mode_button, FALLING);

    Timer1.initialize(2040); // 2040 us ~= 490 Hz
    Timer1.pwm(WATERPUMP_PIN, 0); // turn off water pump, duty cycle: 0-1023

    read_avr_signature(avr_signature); // read AVR signature bytes that identifies the microcontroller

    lcd_init(); // init LCD

    delay(2000);

    lcd_print_default_layout();
    read_eeprom_settings(); // read internal EEPROM for stored settings
    wdt_enable(WDTO_4S); // setup watchdog timer to reset program if it hangs for more than 4 seconds
    send_usb_packet(reset, 0x01, ack, 1); // confirm device readiness
}


void loop()
{
    wdt_reset(); // reset watchdog timer to 0 seconds so no accidental restart occurs
    handle_usb_data();
    current_millis = millis(); // check current time

    if (((uint32_t)(current_millis - last_wpump_millis) >= wpump_interval) && (wpump_ontime != 0) && (wpump_offtime != 0)) // casting the time difference to uint32_t is a workaround for millis timer overflow that occurs every 50 days or so
    {
        last_wpump_millis = current_millis; // save the last time we were here
        
        if (wpump_on)
        {
            wpump_interval = wpump_offtime;
            change_wpump_speed(0); // set zero speed to turn off
            wpump_on = false;
        }
        else
        {
            wpump_on = true;
            wpump_interval = wpump_ontime;
            if (!service_mode) change_wpump_speed(wpump_pwm);
        }
    }

    if ((uint32_t)(current_millis - last_update_millis) >= update_interval) // casting the time difference to uint32_t is a workaround for millis timer overflow that occurs every 50 days or so
    {     
        last_update_millis = current_millis; // save the last time we were here
        get_temps(T_ext, T_int); // measure temperatures and save them in these arrays
        
        if (autoupdate) // update external devices regularly (USB, LCD)
        {
            update_status();
            lcd_update();
        }
    }

    if (mode_button_pressed) // handle mode-button press
    {
        mode_button_pressed = false; // re-arm

        if (!service_mode)
        {
            service_mode = true; // enter service mode
            
            if (wpump_on)
            {
                change_wpump_speed(0); // turn off water pump
            }
        }
        else
        {
            service_mode = false; // exit service mode

            if (wpump_on) // if water pump is on according to the timer
            {
                wpump_on = true;
                change_wpump_speed(wpump_pwm); // resume water pump activity
            }

            if (!wpump_on) digitalWrite(RED_LED_PIN, HIGH); // turn off red LED is this condition catches it during blinking
        }
    }

    if (service_mode) // blink red LED when in service mode
    {   
        if ((uint32_t)(current_millis - previous_led_blink) >= service_mode_blinking_interval)
        {
            if ((previous_led_blink - last_update_millis) != service_mode_blinking_interval) previous_led_blink = last_update_millis + service_mode_blinking_interval; // avoid blinking issues by shifting the blink start after Serial/LCD update
            else previous_led_blink = current_millis; // save current time
            digitalWrite(RED_LED_PIN, LOW); // turn on red LED
            red_led_ontime = current_millis; // save time when red LED was turned on
        }

        if ((uint32_t)(current_millis - red_led_ontime) >= led_blink_duration)
        {
            digitalWrite(RED_LED_PIN, HIGH); // turn off red LED
        }
    }
}
