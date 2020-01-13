/*
 * ModularHydroponicTowerGarden (https://github.com/laszlodaniel/ModularHydroponicTowerGarden)
 * Copyright (C) 2019, László Dániel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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

// Board: Arduino/Genuino Uno
// Fuse bytes:
// - LF: 0xFF
// - HF: 0xD6
// - EF: 0xFD
// - Lock: 0x3F

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <math.h>
#include <util/atomic.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h> // https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/
#include <Wire.h>


// Firmware date/time of compilation in 64-bit UNIX time
// https://www.epochconverter.com/hex
#define FW_DATE 0x000000005E1CAAF1

#define TEMP_EXT      A0 // external 10k NTC thermistor is connected to this analog pin
#define TEMP_INT      A1 // internal 10k NTC thermistor is connected to this analog pin
#define LED_PIN       4  // red LED
#define WATERPUMP_PIN 9  // waterpump pwm output

// Set (1), clear (0) and invert (1->0; 0->1) bit in a register or variable easily
#define sbi(variable, bit) (variable) |=  (1 << (bit))
#define cbi(variable, bit) (variable) &= ~(1 << (bit))
#define ibi(variable, bit) (variable) ^=  (1 << (bit))

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
#define error_checksum_invalid_value            0x05
#define error_packet_timeout_occured            0x06
// 0x06-0xFD reserved
#define error_internal                          0xFE
#define error_fatal                             0xFF

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

// EEPROM settings
uint8_t eeprom_content[32];
uint8_t hw_version[2] = { 0x00, 0x00 };
uint8_t hw_date[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t assembly_date[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
volatile uint8_t wpump_pwm = 0; // 0: 0% - 255: 100%
uint32_t wpump_ontime = 0; // 900000 milliseconds = 900 seconds = 15 minutes
uint32_t wpump_offtime = 0; // 900000 milliseconds = 900 seconds = 15 minutes
uint8_t enabled_temperature_sensors = 0;
uint8_t temperature_unit = 0;
uint8_t calculated_checksum = 0;
bool checksum_ok = false;

bool wpump_on = true;
uint32_t wpump_interval = 0;
uint8_t avr_signature[3];

volatile bool mode_button_pressed = false;
bool autoupdate = true;
bool service_mode = false;

// Packet related variables
// Timeout values for packets
uint8_t command_timeout = 100; // milliseconds
uint16_t command_purge_timeout = 200; // milliseconds, if a command isn't complete within this time then delete the usb receive buffer
uint8_t ack[1] = { 0x00 }; // acknowledge payload array
uint8_t err[1] = { 0xFF }; // error payload array
uint8_t ret[1]; // general array to store arbitrary bytes

typedef union {
    float number;
    uint8_t bytes[4];
} FLOATUNION_t;


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
******************************************************************************/
void get_temps(float *te, float *ti)
{
    float Ke, Ce, Fe; // external variables
    float Ki, Ci, Fi; // internal variables

    Ve = 0; // start with zero
    Vi = 0; // start with zero

    for (uint16_t i = 0; i < 1000; i++) // measure analog voltage quickly 1000 times and add results together
    {
        Ve += analogRead(TEMP_EXT);
    }

    for (uint16_t i = 0; i < 1000; i++) // measure analog voltage quickly 1000 times and add results together
    {
        Vi += analogRead(TEMP_INT);
    }

    Ve /= 1000; // calculate average value
    Vi /= 1000; // calculate average value

    // Calculate external temperature using this equation involving the thermistor's beta property
    Ke = 1.00 / (inv_t0 + inv_beta * (log(1023.00 / (float)Ve - 1.00))); // Kelvin
    Ce = Ke - 273.15; // Celsius
    Fe = ((9.0 * Ce) / 5.00) + 32.00; // Fahrenheit

    // Calculate external temperature using this equation involving the thermistor's beta property
    Ki = 1.00 / (inv_t0 + inv_beta * (log(1023.00 / (float)Vi - 1.00))); // Kelvin
    Ci = Ki - 273.15; // Celsius
    Fi = ((9.0 * Ci) / 5.00) + 32.00; // Fahrenheit

    // Save values in the input arrays
    te[0] = Ke; te[1] = Ce; te[2] = Fe;
    ti[0] = Ki; ti[1] = Ci; ti[2] = Fi;

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
}


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
******************************************************************************/
void update_lcd(void)
{
    // TODO
}


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
******************************************************************************/
void handle_mode_button(void)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        mode_button_pressed = true;
        //wpump_pwm += 20;
        if (wpump_on) analogWrite(WATERPUMP_PIN, wpump_pwm);
    }
}


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
******************************************************************************/
void wpump_rampup(uint8_t pwm_level)
{
    // It's easier on the power supply if the pump is ramped up slowly
    for (uint8_t i = 0; i < pwm_level; i++)
    {
        analogWrite(WATERPUMP_PIN, i);
        delay(15);
    }
    analogWrite(WATERPUMP_PIN, pwm_level);
}


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
******************************************************************************/
void update_timestamp(uint8_t *target)
{
    uint32_t mcu_current_millis = millis();
    target[0] = (mcu_current_millis >> 24) & 0xFF;
    target[1] = (mcu_current_millis >> 16) & 0xFF;
    target[2] = (mcu_current_millis >> 8) & 0xFF;
    target[3] = mcu_current_millis & 0xFF;
}


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


/*****************************************************************************
Function: -
Purpose:  -
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
    wpump_rampup(wpump_pwm); // change water pump speed
    wpump_ontime =  ((uint32_t)eeprom_content[0x13] << 24) | ((uint32_t)eeprom_content[0x14] << 16) | ((uint32_t)eeprom_content[0x15] << 8) | (uint32_t)eeprom_content[0x16];
    wpump_offtime = ((uint32_t)eeprom_content[0x17] << 24) | ((uint32_t)eeprom_content[0x18] << 16) | ((uint32_t)eeprom_content[0x19] << 8) | (uint32_t)eeprom_content[0x1A];
    
    if (wpump_on) wpump_interval = wpump_ontime;
    else wpump_interval = wpump_offtime;
    
    enabled_temperature_sensors = eeprom_content[0x1B];
    temperature_unit = eeprom_content[0x1C];
    beta = ((uint16_t)eeprom_content[0x1D] << 8) | (uint16_t)eeprom_content[0x1E];
    inv_beta = 1.00 / (float)beta;
}


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
******************************************************************************/
void update_status(void)
{
    // Gather data and send back to laptop (water pump state, water pump pwm-level, remaining time until state change, temperatures)
    uint8_t status_payload[14];
    uint32_t remaining_time = last_wpump_millis + wpump_interval - current_millis;
    uint8_t remaining_time_array[4];

    if (!service_mode)
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
    
    status_payload[6] = temperature_payload[0];
    status_payload[7] = temperature_payload[1];
    status_payload[8] = temperature_payload[2];
    status_payload[9] = temperature_payload[3];
    status_payload[10] = temperature_payload[4];
    status_payload[11] = temperature_payload[5];
    status_payload[12] = temperature_payload[6];
    status_payload[13] = temperature_payload[7];
    
    send_usb_packet(status, ok, status_payload, 14);
}


/*************************************************************************
Function: send_hwfw_info()
Purpose:  gather hardware version/date, assembly date and firmware date
          into an array and send through serial link
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


/*****************************************************************************
Function: -
Purpose:  -
Note:     -
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
Function: send_usb_packet()
Purpose:  assemble and send data packet through serial link (UART0)
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
    uint8_t packet[packet_length]; // create a temporary byte-array
    bool payload_bytes = true;
    uint8_t datacode = 0;

    if (payloadbufflen <= 0) payload_bytes = false;
    else payload_bytes = true;

    // Assemble datacode from input parameter
    datacode = (1 << 7)  + command;
    //          10000000 + 0000zzzz  =  1000zzzz  

    // Start assembling the packet by manually filling the first few slots
    packet[0] = 0x3D; // add SYNC byte
    packet[1] = ((packet_length - 4) >> 8) & 0xFF; // add LENGTH high byte
    packet[2] =  (packet_length - 4) & 0xFF; // add LENGTH low byte
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
    Serial.write(packet, packet_length);
    
} // end of send_usb_packet


/*************************************************************************
Function: handle_usb_data()
Purpose:  handle USB commands coming from an external computer
Note:     [ SYNC | LENGTH_HB | LENGTH_LB | DATACODE | SUBDATACODE | <?PAYLOAD?> | CHECKSUM ]
**************************************************************************/
void handle_usb_data(void)
{
    if (Serial.available() > 0) // proceed only if the receive buffer contains at least 1 byte
    {
        uint8_t sync = Serial.read(); // read the next available byte in the USB receive buffer, it's supposed to be the first byte of a message

        if (sync == 0x3D)
        {
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
            bytes_to_read = (length_hb << 8) + length_lb + 1; // +1 CHECKSUM byte
            
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
                send_usb_packet(ok_error, error_checksum_invalid_value, err, 1);
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
                        case 0x01: // read settings
                        {
                            uint8_t settings_payload[13];
                            for (uint8_t i = 0; i < 13; i++) // copy the last 13 bytes from the current eeprom_content array
                            {
                                settings_payload[i] = eeprom_content[0x12 + i];
                            }
                            
                            send_usb_packet(settings, 0x01, settings_payload, 13);
                            break;
                        }
                        case 0x02: // write settings
                        {
                            if (!payload_bytes || (payload_length < 12))
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            else
                            {
                                for (uint8_t i = 0; i < 13; i++) // overwrite settings in the temporary EEPROM array
                                {
                                    eeprom_content[0x12 + i] = cmd_payload[i];
                                }
    
                                eeprom_content[0x1F] = calculate_checksum(eeprom_content, 0, 31); // re-calculate checksum
    
                                for (uint8_t i = 0; i < 32; i++) // update EEPROM (write value if different)
                                {
                                    EEPROM.update(i, eeprom_content[i]);
                                }
    
                                update_settings(); // update settings in RAM
                                send_usb_packet(settings, 0x02, ack, 1);
                            }
                            break;
                        }
                        case 0x03: // autoupdate enable/disable
                        {
                            if (!payload_bytes)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            else
                            {
                                if (cmd_payload[0] == 0x00) autoupdate = false;
                                else autoupdate = true;
                                send_usb_packet(settings, 0x03, cmd_payload, 1);
                            }
                            break;
                        }
                        case 0x04: // service mode on/off
                        {
                            if (!payload_bytes)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            else
                            {
                                if (cmd_payload[0] == 0x00)
                                {
                                    service_mode = false;
                                    wpump_rampup(wpump_pwm); // restart water pump at saved speed
                                }
                                else service_mode = true;
                                send_usb_packet(settings, 0x04, cmd_payload, 1);
                            }
                            break;
                        }
                        case 0x05: // water pump on/off
                        {
                            if (!payload_bytes)
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            else
                            {
                                if (service_mode)
                                {
                                    send_usb_packet(settings, 0x05, cmd_payload, 1);
                                    
                                    if (cmd_payload[0] == 0x00)
                                    {
                                        wpump_on = false;
                                        digitalWrite(WATERPUMP_PIN, LOW);
                                        digitalWrite(LED_PIN, HIGH); // LOW = ON, HIGH/HIGH-Z = OFF
                                    }
                                    else
                                    {
                                        wpump_on = true;
                                        wpump_rampup(cmd_payload[0]);
                                        digitalWrite(LED_PIN, LOW); // LOW = ON, HIGH/HIGH-Z = OFF
                                    }
                                }
                                else
                                {
                                    send_usb_packet(ok_error, error_internal, err, 1);
                                }
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
                            if (!payload_bytes || (payload_length < 4))
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            else
                            {
                                uint16_t index = (cmd_payload[0] << 8) | cmd_payload[1]; // start index in EEPROM to read
                                uint16_t count = (cmd_payload[2] << 8) | cmd_payload[3]; // number of bytes to read starting at index

                                if ((index + count) <= 1024) // ATmega328P has 1024 bytes of EEPROM
                                {
                                    if (count < 512) // Serial.write buffer limits one transmission below 1 kilobytes so a packet bigger than that has to be split apart
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
                                    else // split packet into 2 separate packets
                                    {
                                        uint8_t eeprom_values[512 + 4]; // temporary array to store values + 2 address bytes and 2 count bytes at the beginning
                                        eeprom_values[0] = cmd_payload[0]; // first round, starting address is given in the payload
                                        eeprom_values[1] = cmd_payload[1];
                                        eeprom_values[2] = 0x02;
                                        eeprom_values[3] = 0x00;
        
                                        for (uint16_t i = 0; i < 512; i++)
                                        {
                                            eeprom_values[4 + i] = EEPROM.read(index + i);
                                        }
        
                                        send_usb_packet(debug, 0x01, eeprom_values, 512 + 4);

                                        eeprom_values[0] = 0x02; // second round, starting address is fixed
                                        eeprom_values[1] = 0x00;
                                        eeprom_values[2] = ((count - 512) >> 8) & 0xFF;
                                        eeprom_values[3] = (count - 512) & 0xFF;
        
                                        for (uint16_t i = 512; i < count; i++)
                                        {
                                            eeprom_values[4 + i - 512] = EEPROM.read(index + i);
                                        }
        
                                        send_usb_packet(debug, 0x01, eeprom_values, (count - 512) + 4);
                                    }
                                }
                                else // otherwise at some point the read address will exceed EEPROM size
                                {
                                    send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                                }
                            }
                            break;
                        }
                        case 0x02: // write EEPROM
                        {
                            if (!payload_bytes || (payload_length < 3))
                            {
                                send_usb_packet(ok_error, error_payload_invalid_values, err, 1);
                            }
                            else
                            {
                                uint16_t index = (cmd_payload[0] << 8) | cmd_payload[1]; // start index in EEPROM to write
                                uint16_t count = payload_length - 2;

                                for (uint16_t i = 0; i < count; i++)
                                {
                                    EEPROM.update(index + i, cmd_payload[2 + i]);
                                }

                                send_usb_packet(debug, 0x02, ack, 1);
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
                default: // other values are not used
                {
                    send_usb_packet(ok_error, error_datacode_invalid_command, err, 1);
                    break;
                }
            }
        }
        else // if it's not a sync byte
        {
            Serial.read(); // remove this byte from buffer and try again in the next loop
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
    pinMode(LED_PIN, OUTPUT);
    pinMode(WATERPUMP_PIN, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(2), handle_mode_button, FALLING);

    read_avr_signature(avr_signature); // read AVR signature bytes that identifies the microcontroller

    // Read first 32 bytes of the internal EEPROM
    for (uint8_t i = 0; i < 32; i++)
    {
        eeprom_content[i] = EEPROM.read(i);
    }
    
    // Verify EEPROM content
    calculated_checksum = calculate_checksum(eeprom_content, 0, 31);

    if (calculated_checksum == eeprom_content[0x1F]) // internal EEPROM checksum verified, load settings
    {
        checksum_ok = true;
        update_settings();
    }
    else // internal EEPROM checksum error, load default settings
    {
        checksum_ok = false;
        hw_version[0] = 0x00; // V1.02
        hw_version[1] = 0x66;
        hw_date[0] = 0x00;
        hw_date[1] = 0x00;
        hw_date[2] = 0x00;
        hw_date[3] = 0x00;
        hw_date[4] = 0x5D;
        hw_date[5] = 0xCA;
        hw_date[6] = 0xBE;
        hw_date[7] = 0x0D;
        assembly_date[0] = 0x00; // 2020.01.12 13:33:31
        assembly_date[1] = 0x00;
        assembly_date[2] = 0x00;
        assembly_date[3] = 0x00;
        assembly_date[4] = 0x5D;
        assembly_date[5] = 0xAC;
        assembly_date[6] = 0x57;
        assembly_date[7] = 0x23;
        wpump_pwm = 0x7F; // 0x7F = 127 = 50%
        wpump_rampup(wpump_pwm); // gradually turn on the water pump
        wpump_ontime = 900000; // 15 minutes
        wpump_offtime = 900000; // 15 minutes
        if (wpump_on) wpump_interval = wpump_ontime;
        else wpump_interval = wpump_offtime;
        enabled_temperature_sensors = 0; // 0 = none, 1 = external, 2 = internal, 3 = both external and internal temperature sensors are enabled
        temperature_unit = 1; // Celsius = 1, Fahrenheit = 2, Kelvin = 4
        beta = 3950;
        inv_beta = 1.00 / (float)beta;
    }

    wpump_on = true;
    wdt_enable(WDTO_4S); // reset program if it hangs for more than 4 seconds
    send_usb_packet(reset, 0x01, ack, 1); // confirm device readiness
}


void loop()
{
    wdt_reset(); // reset watchdog timer to 0 seconds so no accidental restart occurs
    current_millis = millis(); // check current time
    handle_usb_data();
    
    if (mode_button_pressed)
    {
        mode_button_pressed = false; // re-arm
        // TODO
    }
    
    if ((current_millis - last_wpump_millis >= wpump_interval) && (current_millis > last_wpump_millis))
    {
        last_wpump_millis = current_millis; // save the last time we were here
        if (!service_mode)
        {
            if (wpump_on)
            {
                wpump_on = false;
                wpump_interval = wpump_offtime;
                digitalWrite(WATERPUMP_PIN, LOW);
                digitalWrite(LED_PIN, HIGH); // LOW = ON, HIGH/HIGH-Z = OFF
            }
            else
            {
                wpump_on = true;
                wpump_interval = wpump_ontime;
                digitalWrite(LED_PIN, LOW); // LOW = ON, HIGH/HIGH-Z = OFF
                wpump_rampup(wpump_pwm);
            }
        }
    }
    else if (current_millis < last_wpump_millis) // be prepared if the millis counter overflows every month or so
    {
        last_wpump_millis = 0;
    }

    if (current_millis - last_update_millis >= update_interval)
    {     
        last_update_millis = current_millis; // save the last time we were here
        get_temps(T_ext, T_int); // measure temperatures and save them in these arrays
        
        if (autoupdate) // update external devices regularly (USB, LCD)
        {
            update_status();
            update_lcd();
        }
    }
    else if (current_millis < last_update_millis) // be prepared if the millis counter overflows every month or so
    {
        last_update_millis = 0;
    }
}
