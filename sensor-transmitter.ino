#include "LoRaWan_APP.h"            // LoRa driver for Heltec boards
#include "Arduino.h"
#include <OneWire.h>                // For 1-Wire communication with DS18B20
#include <DallasTemperature.h>      // DS18B20 temperature sensor library
#include "esp_bt.h"                 // ESP32 Bluetooth control (power saving)
#include "esp_wifi.h"               // ESP32 WiFi control (power saving)

// ===============================
// DS18B20 Temperature Sensor Setup
// ===============================

#define ONE_WIRE_BUS 7              // GPIO pin connected to DS18B20 data line
OneWire oneWire(ONE_WIRE_BUS);     // Create OneWire instance
DallasTemperature sensors(&oneWire); // Pass OneWire to Dallas library

// ===============================
// LoRa Configuration
// ===============================

#define RF_FREQUENCY 865000000      // Frequency for EU868 band (Hz)
#define TX_OUTPUT_POWER 5           // Transmission power in dBm
#define LORA_BANDWIDTH 0            // 125 kHz bandwidth
#define LORA_SPREADING_FACTOR 7     // SF7 for fast, short-range comms
#define LORA_CODINGRATE 1           // 4/5 error correction
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 30              // Max buffer for outgoing messages

char txpacket[BUFFER_SIZE];         // Outgoing message buffer

// LoRa event structure
static RadioEvents_t RadioEvents;

// Function prototypes for LoRa TX events
void OnTxDone(void);
void OnTxTimeout(void);

// ===============================
// Setup
// ===============================

void setup() {
  // Serial can be useful for debugging, but disabled here for power saving
  // Serial.begin(115200);

  // Reduce CPU frequency to 80 MHz to lower power consumption
  setCpuFrequencyMhz(80);

  // Disable Bluetooth and WiFi completely
  btStop(); 
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_wifi_deinit();

  // Initialize MCU and peripherals (Heltec board specifics)
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // Initialize the temperature sensor
  sensors.begin();

  // Trigger temperature measurement
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0); // Get first sensor reading

  // Format the packet to send
  // "XX01" is a custom prefix to distinguish this sensor's data on the receiving side
  snprintf(txpacket, BUFFER_SIZE, "XX01: %.2fC", tempC);
  // Example: "XX01: 23.57C"

  // Set up LoRa event handlers
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  // Initialize the LoRa radio and configuration
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
                    LORA_FIX_LENGTH_PAYLOAD_ON, true, 0, 0,
                    LORA_IQ_INVERSION_ON, 3000); // 3000 ms timeout

  // Transmit the temperature packet
  Radio.Send((uint8_t *)txpacket, strlen(txpacket));
}

// ===============================
// Main loop (not used, but needed for IRQ processing)
// ===============================

void loop() {
  // Required to handle interrupt-driven LoRa events
  Radio.IrqProcess();
}

// ===============================
// LoRa Callback: Transmission Successful
// ===============================

void OnTxDone(void) {
  // Put radio to sleep to save power
  Radio.Sleep();

  // Enter deep sleep for 4 hours (14400 seconds)
  esp_sleep_enable_timer_wakeup(14400000000ULL); // microseconds
  esp_deep_sleep_start();
}

// ===============================
// LoRa Callback: Transmission Timeout
// ===============================

void OnTxTimeout(void) {
  // Even if timeout occurs, go to sleep to conserve power
  Radio.Sleep();

  // Sleep for 60 seconds before trying again (shorter duration in case of error)
  esp_sleep_enable_timer_wakeup(60000000ULL); // microseconds
  esp_deep_sleep_start();
}
