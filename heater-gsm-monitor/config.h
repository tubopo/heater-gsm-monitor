#ifndef CONFIG_H
#define CONFIG_H

/*
 * Configuration Header
 * LOLIN ESP8266 NodeMCU V3 + SIM800L UDV2013072402
 * Dallas DS18B20 Temperature Sensor
 * SSD1306 OLED Display (I2C 128x64)
 * TTP223 Touch Button
 */

// ============================================
// TinyGSM Configuration
// ============================================
#define TINY_GSM_MODEM_SIM800

// ============================================
// Serial Communication
// ============================================
#define BAUD_RATE 9600 // SIM800L baud rate

// ============================================
// Pin Definitions - LOLIN ESP8266 NodeMCU V3
// ============================================
// SIM800L Serial pins (Software Serial)
#define SIM800_RX_PIN D8 // GPIO15 - Swapped
#define SIM800_TX_PIN D7 // GPIO13 - Swapped

// SIM800L Control pins
#define SIM800_RST_PIN D0 // GPIO16 - Reset (optional, can be left unconnected)

// Dallas DS18B20 Temperature Sensor
#define TEMP_SENSOR_PIN D5 // GPIO14 for 1-Wire bus

// TTP223 Touch Button
#define BUTTON_PIN D6 // GPIO12 - Touch button input

// Status LED
#define STATUS_LED_PIN D4 // GPIO2 - Built-in LED

// I2C for SSD1306 Display
#define I2C_SDA_PIN D2 // GPIO4 - I2C SDA
#define I2C_SCL_PIN D1 // GPIO5 - I2C SCL

// ============================================
// Display Configuration (SSD1306)
// ============================================
#define DISPLAY_ADDRESS 0x3C // SSD1306 I2C address
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_TIMEOUT 30000 // Display auto-off after 30 seconds (ms)

// ============================================
// GSM Configuration (SIM800L)
// ============================================
#define GSM_INIT_TIMEOUT 10000 // Timeout for module init (ms)
#define GSM_INIT_RETRIES 3     // Number of init retries

// ============================================
// Timing Configuration (milliseconds)
// ============================================
#define SENSOR_READ_INTERVAL 10000   // Read sensor every 10 seconds
#define DISPLAY_UPDATE_INTERVAL 2000 // Update display every 2 seconds
#define GSM_CHECK_INTERVAL 60000     // Check GSM signal every 60 seconds
#define ALERT_COOLDOWN 180000        // Alert cooldown (avoid spam) - 3 minutes
#define MAX_ALERTS_PER_EVENT 3       // Maximum alerts per problem event before returning to normal

// ============================================
// Temperature Thresholds (Celsius)
// ============================================
#define TEMP_MIN 10.0           // Minimum safe temperature
#define TEMP_MAX 45.0           // Maximum safe temperature
#define TEMP_CRITICAL_LOW 5.0   // Critical low (freezing)
#define TEMP_CRITICAL_HIGH 55.0 // Critical high (overheating)

// Temperature states
#define TEMP_STATE_NORMAL 0
#define TEMP_STATE_WARNING 1
#define TEMP_STATE_CRITICAL 2

// ============================================
// SMS Configuration (using arduino_secrets.h)
// ============================================
#include "arduino_secrets.h"

#define PHONE_NUMBER SECRET_PHONE_NUMBER
#define SIM_PIN SECRET_SIM_PIN
#define ALERT_ON_CRITICAL 1 // Send SMS on critical temperatures (1=yes, 0=no)
#define ALERT_ON_WARNING 1  // Send SMS on warning temperatures (1=yes, 0=no)

#endif // CONFIG_H
