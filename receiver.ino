#include "LoRaWan_APP.h"            // LoRa driver for Heltec boards
#include "Arduino.h"
#include "HT_SSD1306Wire.h"         // OLED display library
#include <WiFi.h>                   // Wi-Fi support
#include <time.h>                   // Time synchronization via NTP
#include <HTTPClient.h>             // For HTTP POST to Home Assistant

// Wi-Fi credentials
const char* ssid     = "SSID";
const char* password = "PSK";

// Home Assistant REST API endpoint for updating the temperature sensor state
const char* serverName = "http://xx.xx.xx.xx:8123/api/states/sensor.loratemp";

// Long-lived access token for Home Assistant authentication
const char* token = "xxxxxx..."; // REDACTED for security — replace with your own token

// Simple 12x8 pixel Wi-Fi icon for OLED, not really nice or useful
const uint8_t wifi_icon_bits[] = {
  0x00, 0x00,
  0x0e, 0x00,
  0x11, 0x00,
  0x20, 0x80,
  0x44, 0x40,
  0x82, 0x20,
  0x01, 0x10,
  0x00, 0x00
};

// LoRa configuration parameters
#define RF_FREQUENCY 865000000        // Frequency for EU868 band
#define TX_OUTPUT_POWER 5             // Transmission power in dBm
#define LORA_BANDWIDTH 0              // 125 kHz
#define LORA_SPREADING_FACTOR 7       // SF7
#define LORA_CODINGRATE 1             // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 30                // Buffer size for incoming packets

char rxpacket[BUFFER_SIZE];           // Buffer to store received LoRa packet
static RadioEvents_t RadioEvents;     // Event handler for LoRa events

// OLED display instance
SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// LoRa receiver state
typedef enum {
  LOWPOWER,
  STATE_RX
} States_t;

States_t state;

// Function prototypes
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void VextON(void);
void sendToHA(float temperature);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected");
  } else {
    Serial.println("Wi-Fi failed to connect");
  }

  // Synchronize time via NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for time sync");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Time synced");

  // Set timezone to Finland (EET)
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();

  // Initialize Heltec board and OLED power
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  VextON();

  // Initialize OLED display
  factory_display.init();
  factory_display.setContrast(255);
  factory_display.flipScreenVertically();
  factory_display.setFont(ArialMT_Plain_16);
  factory_display.drawString(0, 0, "Waiting for remote measurements...");
  factory_display.display();

  // Set up LoRa reception handler
  RadioEvents.RxDone = OnRxDone;

  // Initialize LoRa radio
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  // Start in RX mode
  state = STATE_RX;
}

void loop() {
  // Simple state machine for LoRa RX
  switch (state) {
    case STATE_RX:
      Radio.Rx(0);      // Listen for incoming packets
      state = LOWPOWER;
      break;

    case LOWPOWER:
      Radio.IrqProcess(); // Process received packets
      break;

    default:
      break;
  }
}

// Called when a LoRa packet is received
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0'; // Null-terminate
  Radio.Sleep();

  // Expect packets to begin with "XX01:"
  if (strncmp(rxpacket, "XX01", 4) == 0) {
    Serial.printf("Received: \"%s\" | RSSI: %d\n", rxpacket, rssi);

    // Extract temperature value from payload
    String tempStr = String(rxpacket).substring(5); // skip "TK01:"
    float temp = tempStr.toFloat();
    float roundedTemp = roundf(temp * 10) / 10.0;

    // Format temperature string for display
    char displayStr[10];
    snprintf(displayStr, sizeof(displayStr), "%.1f °C", roundedTemp);

    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char timeStr[20];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // Update OLED display
    factory_display.clear();
    factory_display.setFont(ArialMT_Plain_10);
    factory_display.drawString(0, 0, "Etälämpötila:");
    factory_display.setFont(ArialMT_Plain_24);
    factory_display.drawString(0, 24, displayStr);
    factory_display.setFont(ArialMT_Plain_10);
    factory_display.drawString(0, 50, String("Aika: ") + timeStr);

    // Show Wi-Fi icon if connected
    if (WiFi.status() == WL_CONNECTED) {
        factory_display.drawXbm(112, 0, 12, 8, wifi_icon_bits);
    }

    factory_display.display();

    // Send temperature to Home Assistant
    sendToHA(roundedTemp);

  } else {
    Serial.println("Ignored unknown packet.");
  }

  // Resume listening
  state = STATE_RX;
}

// Enables OLED power
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW); // LOW = ON for Vext
}

// Sends temperature to Home Assistant via HTTP POST
void sendToHA(float temperature) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(token));

    // Construct Home Assistant state payload
    String json = "{\"state\": \"" + String(temperature, 1) + "\", \"attributes\": {\"unit_of_measurement\": \"°C\", \"friendly_name\": \"LoRa Temp 1\"}}";

    int httpResponseCode = http.POST(json);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
