/*
 * ============================================================================
 *  ORLARO - PM2.5 Monitoring & Automatic Air Purification System
 * ============================================================================
 *
 *  Firmware Version : 1.0.0
 *  Hardware         : ESP32 Dev Module
 *  Sensor           : Plantower PMS5003
 *  Author           : Orlaro Engineering
 *  License          : Proprietary
 *
 * ============================================================================
 *  HARDWARE WIRING TABLE
 * ============================================================================
 *
 *  +-------------------+----------+------------------+
 *  | Component         | Pin      | ESP32 GPIO       |
 *  +-------------------+----------+------------------+
 *  | PMS5003 VCC       | 5V       | 5V (VIN)         |
 *  | PMS5003 GND       | GND      | GND              |
 *  | PMS5003 TXD       | TX       | GPIO16 (RX2)     |
 *  | PMS5003 RXD       | RX       | GPIO17 (TX2)     |
 *  | Relay IN          | Signal   | GPIO23           |
 *  | Relay VCC         | 5V       | 5V (VIN)         |
 *  | Relay GND         | GND      | GND              |
 *  | Status LED        | Anode    | GPIO2            |
 *  +-------------------+----------+------------------+
 *
 * ============================================================================
 *  REQUIRED LIBRARIES (Install via Arduino Library Manager)
 * ============================================================================
 *
 *  1. WiFiManager by tzapu           (>= 2.0.0)
 *  2. ArduinoJson by Benoit Blanchon (>= 7.0.0)
 *  3. ESP32 Arduino Core             (>= 2.0.0)
 *
 *  Board Manager URL:
 *    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *
 * ============================================================================
 *  ARCHITECTURE OVERVIEW
 * ============================================================================
 *
 *  The firmware follows a non-blocking, event-driven architecture using
 *  millis()-based independent timers. Each subsystem operates on its own
 *  schedule:
 *
 *  ┌─────────────────────────────────────────────────────┐
 *  │                    MAIN LOOP                        │
 *  │                                                     │
 *  │  ┌──────────────┐  Interval: 1000ms                │
 *  │  │  PMS5003      │  Read & validate sensor frames   │
 *  │  └──────────────┘                                   │
 *  │                                                     │
 *  │  ┌──────────────┐  Interval: 10000ms               │
 *  │  │  Telemetry    │  POST JSON to cloud API          │
 *  │  └──────────────┘                                   │
 *  │                                                     │
 *  │  ┌──────────────┐  Interval: 100ms                 │
 *  │  │  Status LED   │  Blink patterns for status       │
 *  │  └──────────────┘                                   │
 *  │                                                     │
 *  │  ┌──────────────┐  Interval: 5000ms                │
 *  │  │  Diagnostics  │  Print system health info        │
 *  │  └──────────────┘                                   │
 *  │                                                     │
 *  │  ┌──────────────┐  Continuous                      │
 *  │  │  WiFi Mgmt    │  Maintain & reconnect WiFi       │
 *  │  └──────────────┘                                   │
 *  └─────────────────────────────────────────────────────┘
 *
 * ============================================================================
 */

// ============================================================================
//  INCLUDES
// ============================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

// ============================================================================
//  FIRMWARE METADATA
// ============================================================================

#define FIRMWARE_VERSION    "1.0.0"
#define DEVICE_ID           "orlaro_001"

// ============================================================================
//  PIN DEFINITIONS
// ============================================================================

#define PIN_RELAY           23      // Relay control (fan ON/OFF)
#define PIN_STATUS_LED      2       // Onboard status LED
#define PMS_RX_PIN          16      // ESP32 RX2 <- PMS5003 TXD
#define PMS_TX_PIN          17      // ESP32 TX2 -> PMS5003 RXD

// ============================================================================
//  PMS5003 PROTOCOL CONSTANTS
// ============================================================================

#define PMS_BAUD_RATE       9600
#define PMS_FRAME_LENGTH    32      // Total Plantower frame length in bytes
#define PMS_HEADER_BYTE1    0x42    // First header byte
#define PMS_HEADER_BYTE2    0x4D    // Second header byte
#define PMS_DATA_TIMEOUT_MS 5000    // Sensor timeout threshold

// ============================================================================
//  WIFI CONFIGURATION
// ============================================================================

#define WIFI_AP_NAME        "Orlaro_Setup"
#define WIFI_AP_PASSWORD    "orlaro123"
#define WIFI_RECONNECT_MS   30000   // Time before restarting config portal

// ============================================================================
//  CLOUD API CONFIGURATION
// ============================================================================

#define API_ENDPOINT        "https://zenithkandel.com.np/pm2.5/api.php"

// ============================================================================
//  TIMING INTERVALS (milliseconds) - All non-blocking via millis()
// ============================================================================

#define INTERVAL_SENSOR     1000    // Sensor read interval
#define INTERVAL_TELEMETRY  10000   // Cloud upload interval
#define INTERVAL_LED        100     // LED update interval
#define INTERVAL_DIAG       5000    // Diagnostics print interval

// ============================================================================
//  DATA STRUCTURES
// ============================================================================

/**
 * @brief  Holds validated PM sensor readings from PMS5003.
 *         Uses "standard" concentration values (CF=1).
 */
struct PMSData {
    uint16_t pm1_0;             // PM1.0 concentration (µg/m³)
    uint16_t pm2_5;             // PM2.5 concentration (µg/m³)
    uint16_t pm10;              // PM10  concentration (µg/m³)
    bool     valid;             // True if last read was valid
    unsigned long lastValidMs;  // Timestamp of last valid reading
};

/**
 * @brief  Stores the parsed server response fields.
 */
struct ServerConfig {
    bool     fanOn;             // Master fan control from server
    bool     autoMode;          // Automatic threshold-based control
    uint16_t threshold;         // PM2.5 threshold for auto mode
    bool     manualFanOn;       // Manual override fan state
    uint16_t pollInterval;      // Server-suggested poll interval (seconds)
    bool     valid;             // True if last parse succeeded
};

/**
 * @brief  Enumerates LED blinking patterns for visual feedback.
 */
enum LEDPattern {
    LED_SOLID_ON,               // WiFi OK + Sensor healthy
    LED_SLOW_BLINK,             // Connecting to WiFi
    LED_FAST_BLINK,             // Sensor error
    LED_DOUBLE_BLINK            // Server communication error
};

// ============================================================================
//  GLOBAL STATE VARIABLES
// ============================================================================

// -- Sensor Data --
PMSData       g_pmsData         = {0, 0, 0, false, 0};

// -- Server Configuration --
ServerConfig  g_serverCfg       = {false, true, 35, false, 10, false};

// -- Fan State Tracking --
bool          g_fanState        = false;    // Current relay output state
bool          g_prevFanState    = false;    // Previous state for change detection

// -- LED Pattern --
LEDPattern    g_ledPattern      = LED_SLOW_BLINK;

// -- Error Flags --
bool          g_sensorOnline    = false;
bool          g_serverError     = false;
bool          g_wifiConnected   = false;

// -- Non-Blocking Timers --
unsigned long g_timerSensor     = 0;
unsigned long g_timerTelemetry  = 0;
unsigned long g_timerLED        = 0;
unsigned long g_timerDiag       = 0;

// -- LED Blink State Machine --
unsigned long g_ledBlinkTimer   = 0;
uint8_t       g_ledBlinkPhase   = 0;
bool          g_ledState        = false;

// -- WiFi Reconnect Timer --
unsigned long g_wifiLostTime    = 0;
bool          g_wifiWasLost     = false;

// -- WiFi Manager --
WiFiManager   g_wifiManager;

// -- Uptime Tracking --
unsigned long g_bootTime        = 0;

// -- PMS5003 Serial Buffer --
uint8_t       g_pmsBuffer[PMS_FRAME_LENGTH];

// ============================================================================
//  FUNCTION PROTOTYPES
// ============================================================================

void setupWiFi();
void maintainWiFi();

void setupPMS5003();
bool readPMS5003();

void sendTelemetry();

void parseServerResponse(const String& response);

void updateRelay();

void updateStatusLED();

void printDiagnostics();

// ============================================================================
//  SETUP
// ============================================================================

/**
 * @brief  Arduino setup() - Initialize all subsystems.
 *
 *         Execution order:
 *           1. Serial debug console
 *           2. GPIO pin modes
 *           3. PMS5003 sensor UART
 *           4. WiFi connection via WiFiManager
 *           5. Initialize timers
 */
void setup() {
    // ---- Serial Console ----
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        ; // Wait up to 3 seconds for serial port (USB CDC)
    }

    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F("  ORLARO Air Purification System"));
    Serial.println(F("  Firmware: " FIRMWARE_VERSION));
    Serial.println(F("  Device:   " DEVICE_ID));
    Serial.println(F("============================================"));
    Serial.println();

    // ---- Record boot time ----
    g_bootTime = millis();

    // ---- GPIO Configuration ----
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);

    // Ensure relay starts OFF (fan off)
    digitalWrite(PIN_RELAY, LOW);
    g_fanState = false;
    g_prevFanState = false;

    // LED on during init
    digitalWrite(PIN_STATUS_LED, HIGH);

    // ---- Initialize PMS5003 ----
    setupPMS5003();

    // ---- Initialize WiFi ----
    setupWiFi();

    // ---- Initialize Timers ----
    unsigned long now = millis();
    g_timerSensor     = now;
    g_timerTelemetry  = now;
    g_timerLED        = now;
    g_timerDiag       = now;
    g_ledBlinkTimer   = now;

    Serial.println(F("[INIT] All subsystems initialized."));
    Serial.println(F("[INIT] Entering main loop..."));
    Serial.println();
}

// ============================================================================
//  MAIN LOOP
// ============================================================================

/**
 * @brief  Arduino loop() - Non-blocking main loop.
 *
 *         Each subsystem checks its own timer and executes independently.
 *         NO blocking delays are used anywhere in this loop.
 */
void loop() {
    unsigned long now = millis();

    // ---- WiFi Maintenance (continuous) ----
    maintainWiFi();

    // ---- Sensor Reading (every 1 second) ----
    if (now - g_timerSensor >= INTERVAL_SENSOR) {
        g_timerSensor = now;
        readPMS5003();
    }

    // ---- Telemetry Upload (every 10 seconds) ----
    if (now - g_timerTelemetry >= INTERVAL_TELEMETRY) {
        g_timerTelemetry = now;
        if (g_wifiConnected) {
            sendTelemetry();
        } else {
            Serial.println(F("[CLOUD] Skipping telemetry - WiFi not connected."));
        }
    }

    // ---- Status LED Update (every 100 ms) ----
    if (now - g_timerLED >= INTERVAL_LED) {
        g_timerLED = now;
        updateStatusLED();
    }

    // ---- Diagnostics Print (every 5 seconds) ----
    if (now - g_timerDiag >= INTERVAL_DIAG) {
        g_timerDiag = now;
        printDiagnostics();
    }
}

// ============================================================================
//  WIFI MANAGEMENT
// ============================================================================

/**
 * @brief  Initialize WiFi using WiFiManager library.
 *
 *         If credentials are saved, auto-connects on boot.
 *         If no credentials exist, starts a captive portal AP.
 *
 *         AP Name:     "Orlaro_Setup"
 *         AP Password: "orlaro123"
 */
void setupWiFi() {
    Serial.println(F("[WIFI] Initializing WiFi Manager..."));

    // Set WiFiManager configuration
    g_wifiManager.setConfigPortalTimeout(180);  // 3 minute portal timeout
    g_wifiManager.setConnectTimeout(20);         // 20 second connect timeout

    // Custom AP message callback
    g_wifiManager.setAPCallback([](WiFiManager* mgr) {
        Serial.println(F("[WIFI] ==================================="));
        Serial.println(F("[WIFI] AP Mode Started!"));
        Serial.print(F("[WIFI] AP Name: "));
        Serial.println(WIFI_AP_NAME);
        Serial.println(F("[WIFI] Captive portal is running..."));
        Serial.print(F("[WIFI] Portal IP: "));
        Serial.println(WiFi.softAPIP());
        Serial.println(F("[WIFI] Connect to AP and configure WiFi."));
        Serial.println(F("[WIFI] ==================================="));
    });

    // Save config callback
    g_wifiManager.setSaveConfigCallback([]() {
        Serial.println(F("[WIFI] Credentials received and saved!"));
    });

    // Attempt auto-connect, or start config portal
    Serial.println(F("[WIFI] Attempting auto-connect with saved credentials..."));
    g_ledPattern = LED_SLOW_BLINK;

    bool connected = g_wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);

    if (connected) {
        g_wifiConnected = true;
        g_ledPattern = LED_SOLID_ON;
        Serial.println(F("[WIFI] ==================================="));
        Serial.println(F("[WIFI] Connected successfully!"));
        Serial.print(F("[WIFI] SSID: "));
        Serial.println(WiFi.SSID());
        Serial.print(F("[WIFI] IP Address: "));
        Serial.println(WiFi.localIP());
        Serial.print(F("[WIFI] RSSI: "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm"));
        Serial.print(F("[WIFI] MAC: "));
        Serial.println(WiFi.macAddress());
        Serial.println(F("[WIFI] ==================================="));
    } else {
        g_wifiConnected = false;
        Serial.println(F("[WIFI] Failed to connect. Will retry in main loop."));
    }
}

/**
 * @brief  Maintain WiFi connection in main loop.
 *
 *         Monitors connection state and handles:
 *           - Automatic reconnection attempts
 *           - Config portal restart after prolonged disconnection
 */
void maintainWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        // ---- Connected ----
        if (!g_wifiConnected) {
            // Just reconnected
            g_wifiConnected = true;
            g_wifiWasLost = false;
            g_wifiLostTime = 0;

            Serial.println(F("[WIFI] Reconnected successfully!"));
            Serial.print(F("[WIFI] IP: "));
            Serial.println(WiFi.localIP());
            Serial.print(F("[WIFI] RSSI: "));
            Serial.print(WiFi.RSSI());
            Serial.println(F(" dBm"));
        }
    } else {
        // ---- Disconnected ----
        if (g_wifiConnected) {
            // Just lost connection
            g_wifiConnected = false;
            g_wifiWasLost = true;
            g_wifiLostTime = millis();
            g_ledPattern = LED_SLOW_BLINK;

            Serial.println(F("[WIFI] Connection lost! Attempting auto-reconnect..."));
        }

        // If disconnected for too long, restart config portal
        if (g_wifiWasLost && (millis() - g_wifiLostTime > WIFI_RECONNECT_MS)) {
            Serial.println(F("[WIFI] Reconnection timeout reached. Restarting config portal..."));
            g_wifiWasLost = false;

            // Non-blocking: start config portal with timeout
            g_wifiManager.setConfigPortalTimeout(120);
            g_wifiManager.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASSWORD);

            // After portal closes (timeout or configured)
            if (WiFi.status() == WL_CONNECTED) {
                g_wifiConnected = true;
                Serial.println(F("[WIFI] Connected via config portal!"));
            } else {
                g_wifiLostTime = millis();
                g_wifiWasLost = true;
                Serial.println(F("[WIFI] Portal closed. Will retry later."));
            }
        }
    }
}

// ============================================================================
//  PMS5003 SENSOR INTERFACE
// ============================================================================

/**
 * @brief  Initialize UART2 for PMS5003 communication.
 *
 *         Configures Serial2 on GPIO16 (RX) and GPIO17 (TX)
 *         at 9600 baud per Plantower PMS5003 specification.
 */
void setupPMS5003() {
    Serial.println(F("[PMS] Initializing PMS5003 sensor..."));
    Serial.print(F("[PMS] RX Pin: GPIO"));
    Serial.print(PMS_RX_PIN);
    Serial.print(F(" | TX Pin: GPIO"));
    Serial.println(PMS_TX_PIN);
    Serial.print(F("[PMS] Baud Rate: "));
    Serial.println(PMS_BAUD_RATE);

    Serial2.begin(PMS_BAUD_RATE, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);

    // Allow sensor startup time
    Serial.println(F("[PMS] Sensor initialized. Waiting for stable readings..."));
}

/**
 * @brief  Read and validate a complete PMS5003 data frame.
 *
 *         Plantower PMS5003 Frame Structure (32 bytes):
 *         ┌────────┬────────┬──────────┬───────────────────┬──────────┐
 *         │ 0x42   │ 0x4D   │ Length   │ Data (13 x 2B)    │ Checksum │
 *         │ (1B)   │ (1B)   │ (2B)     │ (26B)             │ (2B)     │
 *         └────────┴────────┴──────────┴───────────────────┴──────────┘
 *
 *         Data fields (standard/CF=1):
 *           Byte  4-5:  PM1.0
 *           Byte  6-7:  PM2.5
 *           Byte  8-9:  PM10
 *           Byte 10-11: PM1.0 (atmospheric)
 *           Byte 12-13: PM2.5 (atmospheric)
 *           Byte 14-15: PM10  (atmospheric)
 *           ...remaining: particle counts
 *
 * @return true if a valid frame was read and parsed successfully.
 */
bool readPMS5003() {
    // Check for sensor data timeout
    if (g_pmsData.lastValidMs > 0 &&
        (millis() - g_pmsData.lastValidMs > PMS_DATA_TIMEOUT_MS)) {
        if (g_sensorOnline) {
            Serial.println(F("[PMS] ERROR: Sensor data timeout! No valid data in 5 seconds."));
            g_sensorOnline = false;
            g_pmsData.valid = false;
            g_ledPattern = LED_FAST_BLINK;
        }
    }

    // Check if enough bytes are available
    if (Serial2.available() < PMS_FRAME_LENGTH) {
        return false;
    }

    // ---- Step 1: Synchronize to frame header 0x42 0x4D ----
    bool headerFound = false;
    while (Serial2.available() >= PMS_FRAME_LENGTH) {
        uint8_t byte1 = Serial2.peek();
        if (byte1 == PMS_HEADER_BYTE1) {
            // Read the first byte
            Serial2.read();
            if (Serial2.available() < PMS_FRAME_LENGTH - 1) {
                return false;
            }
            uint8_t byte2 = Serial2.peek();
            if (byte2 == PMS_HEADER_BYTE2) {
                // Valid header found
                g_pmsBuffer[0] = PMS_HEADER_BYTE1;
                g_pmsBuffer[1] = PMS_HEADER_BYTE2;
                headerFound = true;
                break;
            }
            // byte2 wasn't 0x4D, continue scanning
        } else {
            // Discard non-header byte
            Serial2.read();
        }
    }

    if (!headerFound) {
        return false;
    }

    // ---- Step 2: Read remaining frame bytes ----
    // We already have bytes 0 and 1; read byte 1 (0x4D) and then 2..31
    Serial2.read(); // consume the peeked 0x4D
    size_t bytesRead = Serial2.readBytes(&g_pmsBuffer[2], PMS_FRAME_LENGTH - 2);

    if (bytesRead != PMS_FRAME_LENGTH - 2) {
        Serial.println(F("[PMS] ERROR: Incomplete frame received."));
        g_pmsData.valid = false;
        return false;
    }

    // ---- Step 3: Verify frame length field ----
    uint16_t frameLength = (g_pmsBuffer[2] << 8) | g_pmsBuffer[3];
    if (frameLength != 2 * 13 + 2) {  // 28 bytes = 13 data words + checksum
        Serial.print(F("[PMS] ERROR: Invalid frame length: "));
        Serial.println(frameLength);
        g_pmsData.valid = false;
        return false;
    }

    // ---- Step 4: Verify checksum ----
    // Checksum = sum of all bytes from header to second-to-last byte
    uint16_t calculatedChecksum = 0;
    for (int i = 0; i < PMS_FRAME_LENGTH - 2; i++) {
        calculatedChecksum += g_pmsBuffer[i];
    }
    uint16_t receivedChecksum = (g_pmsBuffer[30] << 8) | g_pmsBuffer[31];

    if (calculatedChecksum != receivedChecksum) {
        Serial.print(F("[PMS] ERROR: Checksum mismatch! Calc=0x"));
        Serial.print(calculatedChecksum, HEX);
        Serial.print(F(" Recv=0x"));
        Serial.println(receivedChecksum, HEX);
        g_pmsData.valid = false;
        return false;
    }

    // ---- Step 5: Extract PM values (standard/CF=1 concentrations) ----
    g_pmsData.pm1_0 = (g_pmsBuffer[4]  << 8) | g_pmsBuffer[5];
    g_pmsData.pm2_5 = (g_pmsBuffer[6]  << 8) | g_pmsBuffer[7];
    g_pmsData.pm10  = (g_pmsBuffer[8]  << 8) | g_pmsBuffer[9];
    g_pmsData.valid = true;
    g_pmsData.lastValidMs = millis();

    // Update sensor status
    if (!g_sensorOnline) {
        Serial.println(F("[PMS] Sensor is ONLINE. Receiving valid data."));
    }
    g_sensorOnline = true;

    // Update LED if sensor was in error state
    if (g_ledPattern == LED_FAST_BLINK && g_wifiConnected && !g_serverError) {
        g_ledPattern = LED_SOLID_ON;
    }

    return true;
}

// ============================================================================
//  CLOUD TELEMETRY
// ============================================================================

/**
 * @brief  Send PM sensor data to the cloud API via HTTPS POST.
 *
 *         Constructs a JSON payload with device ID, PM readings, and
 *         sensor status. Parses the server's JSON response to update
 *         fan control configuration.
 *
 *         If sensor is offline, sends a minimal status payload.
 */
void sendTelemetry() {
    Serial.println(F("[CLOUD] ---- Telemetry Upload ----"));

    WiFiClientSecure client;
    client.setInsecure();  // Skip SSL certificate verification

    HTTPClient http;

    if (!http.begin(client, API_ENDPOINT)) {
        Serial.println(F("[CLOUD] ERROR: Failed to initialize HTTP connection."));
        g_serverError = true;
        g_ledPattern = LED_DOUBLE_BLINK;
        return;
    }

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);  // 10 second timeout

    // ---- Build JSON Payload ----
    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;

    if (g_sensorOnline && g_pmsData.valid) {
        doc["pm25"]          = g_pmsData.pm2_5;
        doc["pm10"]          = g_pmsData.pm10;
        doc["pm1"]           = g_pmsData.pm1_0;
        doc["sensor_status"] = "online";
        doc["firmware"]      = FIRMWARE_VERSION;
    } else {
        doc["sensor_status"] = "offline";
    }

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.print(F("[CLOUD] JSON Sent: "));
    Serial.println(jsonPayload);

    // ---- Send HTTP POST ----
    int httpCode = http.POST(jsonPayload);

    Serial.print(F("[CLOUD] HTTP Response Code: "));
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();

        if (response.length() == 0) {
            Serial.println(F("[CLOUD] ERROR: Empty response from server."));
            g_serverError = true;
            g_ledPattern = LED_DOUBLE_BLINK;
        } else {
            Serial.print(F("[CLOUD] JSON Received: "));
            Serial.println(response);

            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
                parseServerResponse(response);
                g_serverError = false;

                // Restore LED pattern if no other errors
                if (g_sensorOnline && g_wifiConnected) {
                    g_ledPattern = LED_SOLID_ON;
                }
            } else {
                Serial.print(F("[CLOUD] ERROR: HTTP error code: "));
                Serial.println(httpCode);
                g_serverError = true;
                g_ledPattern = LED_DOUBLE_BLINK;
            }
        }
    } else {
        Serial.print(F("[CLOUD] ERROR: HTTPS connection failed. Error: "));
        Serial.println(http.errorToString(httpCode));
        g_serverError = true;
        g_ledPattern = LED_DOUBLE_BLINK;
    }

    http.end();
    Serial.println(F("[CLOUD] ---- Upload Complete ----"));
}

// ============================================================================
//  SERVER RESPONSE PARSER
// ============================================================================

/**
 * @brief  Parse the JSON response from the cloud API.
 *
 *         Expected JSON format:
 *         {
 *           "status":       "success",
 *           "fan_on":       true/false,
 *           "auto_mode":    true/false,
 *           "threshold":    35,
 *           "manual_fan_on": false,
 *           "poll_interval": 10
 *         }
 *
 * @param  response  Raw JSON string from the server.
 */
void parseServerResponse(const String& response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        Serial.print(F("[PARSE] ERROR: JSON parsing failed: "));
        Serial.println(error.c_str());
        g_serverCfg.valid = false;
        return;
    }

    // Validate required "status" field
    if (!doc.containsKey("status")) {
        Serial.println(F("[PARSE] ERROR: Missing 'status' field in response."));
        g_serverCfg.valid = false;
        return;
    }

    String status = doc["status"].as<String>();
    if (status != "success") {
        Serial.print(F("[PARSE] WARNING: Server status: "));
        Serial.println(status);
    }

    // Parse all configuration fields with defaults
    g_serverCfg.fanOn        = doc["fan_on"]        | false;
    g_serverCfg.autoMode     = doc["auto_mode"]     | true;
    g_serverCfg.threshold    = doc["threshold"]     | 35;
    g_serverCfg.manualFanOn  = doc["manual_fan_on"] | false;
    g_serverCfg.pollInterval = doc["poll_interval"] | 10;
    g_serverCfg.valid        = true;

    Serial.println(F("[PARSE] Server config updated:"));
    Serial.print(F("[PARSE]   fan_on:        "));
    Serial.println(g_serverCfg.fanOn ? "true" : "false");
    Serial.print(F("[PARSE]   auto_mode:     "));
    Serial.println(g_serverCfg.autoMode ? "true" : "false");
    Serial.print(F("[PARSE]   threshold:     "));
    Serial.println(g_serverCfg.threshold);
    Serial.print(F("[PARSE]   manual_fan_on: "));
    Serial.println(g_serverCfg.manualFanOn ? "true" : "false");
    Serial.print(F("[PARSE]   poll_interval: "));
    Serial.println(g_serverCfg.pollInterval);

    // ---- Apply fan control from server response ----
    updateRelay();
}

// ============================================================================
//  RELAY / FAN CONTROL
// ============================================================================

/**
 * @brief  Update the relay (fan) state based on server configuration.
 *
 *         Control logic:
 *           - If server says fan_on = true  -> Relay HIGH (fan ON)
 *           - If server says fan_on = false -> Relay LOW  (fan OFF)
 *
 *         Logs every state change for diagnostics.
 */
void updateRelay() {
    bool newFanState = g_serverCfg.fanOn;

    // Apply the state
    g_fanState = newFanState;
    digitalWrite(PIN_RELAY, g_fanState ? HIGH : LOW);

    // Detect and log state changes
    if (g_fanState != g_prevFanState) {
        Serial.println(F(""));
        Serial.println(F("╔══════════════════════════════════════╗"));
        if (g_fanState) {
            Serial.println(F("║  FAN STATE CHANGED -> ON             ║"));
        } else {
            Serial.println(F("║  FAN STATE CHANGED -> OFF            ║"));
        }
        Serial.println(F("╚══════════════════════════════════════╝"));
        Serial.println(F(""));

        g_prevFanState = g_fanState;
    }
}

// ============================================================================
//  STATUS LED CONTROLLER
// ============================================================================

/**
 * @brief  Update the status LED based on current system state.
 *
 *         LED Patterns:
 *         ┌────────────────────┬──────────────────────────────────┐
 *         │ Pattern            │ Meaning                          │
 *         ├────────────────────┼──────────────────────────────────┤
 *         │ Solid ON           │ WiFi OK + Sensor Healthy         │
 *         │ Slow Blink (1s)    │ Connecting to WiFi               │
 *         │ Fast Blink (200ms) │ Sensor Error                     │
 *         │ Double Blink       │ Server Communication Error       │
 *         └────────────────────┴──────────────────────────────────┘
 */
void updateStatusLED() {
    unsigned long now = millis();

    // Dynamically determine the correct LED pattern based on system state
    if (!g_wifiConnected) {
        g_ledPattern = LED_SLOW_BLINK;
    } else if (!g_sensorOnline) {
        g_ledPattern = LED_FAST_BLINK;
    } else if (g_serverError) {
        g_ledPattern = LED_DOUBLE_BLINK;
    } else {
        g_ledPattern = LED_SOLID_ON;
    }

    switch (g_ledPattern) {
        case LED_SOLID_ON:
            // Continuous ON
            digitalWrite(PIN_STATUS_LED, HIGH);
            break;

        case LED_SLOW_BLINK:
            // Toggle every 1000ms
            if (now - g_ledBlinkTimer >= 1000) {
                g_ledBlinkTimer = now;
                g_ledState = !g_ledState;
                digitalWrite(PIN_STATUS_LED, g_ledState ? HIGH : LOW);
            }
            break;

        case LED_FAST_BLINK:
            // Toggle every 200ms
            if (now - g_ledBlinkTimer >= 200) {
                g_ledBlinkTimer = now;
                g_ledState = !g_ledState;
                digitalWrite(PIN_STATUS_LED, g_ledState ? HIGH : LOW);
            }
            break;

        case LED_DOUBLE_BLINK:
            // Double blink pattern: ON-OFF-ON-OFF---pause---repeat
            // Phase 0: ON  (100ms)
            // Phase 1: OFF (100ms)
            // Phase 2: ON  (100ms)
            // Phase 3: OFF (700ms) - long pause
            {
                uint16_t phaseDuration;
                switch (g_ledBlinkPhase) {
                    case 0: phaseDuration = 100; break;   // First blink ON
                    case 1: phaseDuration = 100; break;   // Gap
                    case 2: phaseDuration = 100; break;   // Second blink ON
                    case 3: phaseDuration = 700; break;   // Long pause
                    default: phaseDuration = 100; break;
                }

                if (now - g_ledBlinkTimer >= phaseDuration) {
                    g_ledBlinkTimer = now;
                    g_ledBlinkPhase = (g_ledBlinkPhase + 1) % 4;

                    // Phases 0 and 2 are ON, 1 and 3 are OFF
                    bool ledOn = (g_ledBlinkPhase == 0 || g_ledBlinkPhase == 2);
                    digitalWrite(PIN_STATUS_LED, ledOn ? HIGH : LOW);
                }
            }
            break;
    }
}

// ============================================================================
//  DIAGNOSTICS
// ============================================================================

/**
 * @brief  Print comprehensive system diagnostics to serial console.
 *
 *         Outputs: WiFi status, RSSI, IP, PM readings, packet status,
 *         fan state, heap memory, and uptime.
 */
void printDiagnostics() {
    unsigned long uptime = (millis() - g_bootTime) / 1000;
    uint16_t hours   = uptime / 3600;
    uint16_t minutes = (uptime % 3600) / 60;
    uint16_t seconds = uptime % 60;

    Serial.println(F(""));
    Serial.println(F("┌──────────────────────────────────────────┐"));
    Serial.println(F("│         ORLARO SYSTEM DIAGNOSTICS        │"));
    Serial.println(F("├──────────────────────────────────────────┤"));

    // -- WiFi Info --
    Serial.print(F("│ WiFi Status  : "));
    if (g_wifiConnected) {
        Serial.println(F("CONNECTED              │"));
        Serial.print(F("│ WiFi RSSI    : "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm                    │"));
        Serial.print(F("│ IP Address   : "));
        String ip = WiFi.localIP().toString();
        Serial.print(ip);
        // Pad to align the box
        for (int i = ip.length(); i < 24; i++) Serial.print(' ');
        Serial.println(F("│"));
    } else {
        Serial.println(F("DISCONNECTED            │"));
    }

    Serial.println(F("├──────────────────────────────────────────┤"));

    // -- Sensor Info --
    Serial.print(F("│ Sensor       : "));
    Serial.println(g_sensorOnline ? F("ONLINE                  │") : F("OFFLINE                 │"));
    Serial.print(F("│ Packet Valid : "));
    Serial.println(g_pmsData.valid ? F("YES                     │") : F("NO                      │"));

    if (g_sensorOnline && g_pmsData.valid) {
        Serial.print(F("│ PM1.0        : "));
        Serial.print(g_pmsData.pm1_0);
        Serial.println(F(" µg/m³                   │"));
        Serial.print(F("│ PM2.5        : "));
        Serial.print(g_pmsData.pm2_5);
        Serial.println(F(" µg/m³                   │"));
        Serial.print(F("│ PM10         : "));
        Serial.print(g_pmsData.pm10);
        Serial.println(F(" µg/m³                   │"));
    }

    Serial.println(F("├──────────────────────────────────────────┤"));

    // -- Fan / Relay --
    Serial.print(F("│ Fan State    : "));
    Serial.println(g_fanState ? F("ON (Relay HIGH)         │") : F("OFF (Relay LOW)         │"));
    Serial.print(F("│ Server Ctrl  : "));
    Serial.println(g_serverCfg.valid ? F("ACTIVE                  │") : F("INACTIVE                │"));

    Serial.println(F("├──────────────────────────────────────────┤"));

    // -- System Health --
    Serial.print(F("│ Free Heap    : "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes                   │"));
    Serial.print(F("│ Uptime       : "));
    char uptimeStr[16];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02d:%02d:%02d", hours, minutes, seconds);
    Serial.print(uptimeStr);
    Serial.println(F("                        │"));
    Serial.print(F("│ Server Error : "));
    Serial.println(g_serverError ? F("YES                     │") : F("NO                      │"));

    Serial.println(F("└──────────────────────────────────────────┘"));
    Serial.println(F(""));
}

// ============================================================================
//  END OF FIRMWARE
// ============================================================================
