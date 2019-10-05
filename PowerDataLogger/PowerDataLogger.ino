#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <credentials.h>                                 // or define mySSID and myPASSWORD and THINGSPEAK_API_KEY

#define LOG_PERIOD 60000                                 // Logging period in milliseconds
#define THINKSPEAK_CHANNEL 879281

// ThingSpeak settings
const int channelID = THINKSPEAK_CHANNEL;
const char* thingspeakServer = "api.thingspeak.com";

unsigned long previousMillis;                            // Time measurement

WiFiClient client;
Adafruit_INA219 ina219;

void setup() {
  Serial.begin(115200);

  uint32_t currentFrequency;

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  ina219.begin();
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();

  Serial.println("Measuring voltage and current with INA219 ...");

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  while (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
}

void loop() {
  unsigned long currentMillis = millis();

  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  Serial.println("");

  delay(2000);

  if (currentMillis - previousMillis > LOG_PERIOD) {
    previousMillis = currentMillis;
    postThingspeak(shuntvoltage, busvoltage, current_mA, power_mW, loadvoltage);
  }
}

void connectWifi() {

  Serial.print("Connecting Wifi: ");
  Serial.println(mySSID);

  WiFi.begin(mySSID, myPASSWORD);
  int wifi_counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifi_counter++;
    if (wifi_counter >= 60) { //30 seconds timeout - reset board
      ESP.restart();
    }
  }

  Serial.println("Wi-Fi Connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
}

void postThingspeak(float shuntvoltage, float busvoltage, float current_mA, float power_mW, float loadvoltage) {
  if (client.connect(thingspeakServer, 80)) {

    // Construct API request body
    String body = "field1=";
    body += String(busvoltage);
    body += "&field2=";
    body += String(shuntvoltage);
    body += "&field3=";
    body += String(current_mA);
    body += "&field4=";
    body += String(power_mW);
    body += "&field5=";
    body += String(loadvoltage);
    body += "&latitude=";
    body += String(LATITUDE);
    body += "&longitude=";
    body += String(LONGITUDE);
    body += "&elevation=";
    body += String(ELEVATION);
    body += "&status=";
    body += String("OK");

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + String(WRITE_API_KEY_PDL) + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(body.length());
    client.print("\n\n");
    client.print(body);
    client.print("\n\n");
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }
  client.stop();
}
