#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "esp_bt.h" //for shutting down Bluetooth
#include "esp_wifi.h" //for shutting down WiFi

#define ONE_WIRE_BUS 7 //GPIO used to connect DS18B20 data wire
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define RF_FREQUENCY 865000000 // Hz
#define TX_OUTPUT_POWER 5 // dBm, optimizing this could lead to longer battery life
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 30


char txpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

void setup() {
  //Serial.begin(115200); can be used for troubleshooting and testing through serial connection, commented out for actual usage for battery life
  setCpuFrequencyMhz(80); // Set CPU to 80 MHz for improved battery life
  //delay(2000); can be used for troubleshooting and testing through serial connection,
  //Serial.println("Booting and measuring temperature..."); can be used for troubleshooting and testing through serial connection,
  btStop(); 
  esp_bt_controller_disable();
  esp_wifi_stop();
  esp_wifi_deinit();

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  sensors.begin();

  // Take temperature reading
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // Format reading
  snprintf(txpacket, BUFFER_SIZE, "XX01: %.2fC", tempC); //XX01 used as a prefix to discard other possible messages picked up by the receiver
  //Serial.printf("Sending: \"%s\"\n", txpacket); can be used for troubleshooting and testing through serial connection,

  //Sending
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
                    LORA_FIX_LENGTH_PAYLOAD_ON, true, 0, 0,
                    LORA_IQ_INVERSION_ON, 3000);

  Radio.Send((uint8_t *)txpacket, strlen(txpacket));
}

void loop() {
  Radio.IrqProcess();
}

void OnTxDone(void) {
 // Serial.println("TX done. Going to deep sleep for 1 minute...");
  Radio.Sleep();

  // 4 hours sleep
  esp_sleep_enable_timer_wakeup(14400000000ULL);
  esp_deep_sleep_start();
}

void OnTxTimeout(void) {
  Serial.println("TX Timeout. Sleeping anyway...");
  Radio.Sleep();

  esp_sleep_enable_timer_wakeup(60000000ULL);
  esp_deep_sleep_start();
}
