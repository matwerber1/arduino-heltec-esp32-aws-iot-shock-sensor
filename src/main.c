#include <MQTTClient.h>       // Be sure to install the MQTT client from Arduino Library Manager authored by Joel Gaehwiler
#include <MQTT.h>
#include "heltec.h"           // includes library that makes it easier to use OLED
#include "WiFi.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "dht11.h"
#include "images.h"           // Default Heltec images displayed on OLED
#include "secrets.h"          // Contains your wifi and AWS IoT credentials/config

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_SHOCK_TOPIC "esp32/shockAlert"
#define AWS_IOT_PUBLISH_SHADOW_TOPIC "$aws/things/esp32/shadow/update"
#define AWS_IOT_SHADOW_DELTA_TOPIC "$aws/things/esp32/shadow/update/delta"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/alert"

#define MESSAGE_IN_PIN 5      // LED with 100 ohm resistor that lights up when MQTT message received from AWS
#define SHOCK_LED_PIN 17      // LED with 100ohm resistor that will light up when shock occurs
#define SHOCK_SENSOR_PIN 35   // I used the shock sensor from this kit: https://www.amazon.com/kuman-K5-USFor-Raspberry-Projects-Tutorials/dp/B016D5L5KE
#define LIGHT_LED_PIN 18          // Turn this off and on using device state from shadow; 
#define DHT11_PIN 14            // Temp/Humidity sensor

void writeLine(String str, bool writeToSerial = true);

dht11 DHT11;
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

int shockVal;             // 1 if shock detected, 0 if not
float humidity;           // read from DHT11
float tempF;              // read from DHT11

long previousMillis = 0;        // will store last time shadow was updated 
long shadowInterval = 3000;     // how often (ms) we update shadow
bool lightEnabled = false;      // the state.reported.lightEnabled value

void setup()
{
  setupDisplay();
  setupPins();
  connectAWS();
}

void loop(){
  client.loop();      // for MQTT to send/receive packets
  checkForShock();
  
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > shadowInterval) {
    previousMillis = currentMillis;
    updateShadow();
  }

}

void setupDisplay() {

  Serial.println("Setting up...");
    Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
    logo();
    delay(300);
    Heltec.display->clear(); 

}

void setupPins() {
  
  pinMode(SHOCK_LED_PIN, OUTPUT);
  pinMode(MESSAGE_IN_PIN, OUTPUT);
  pinMode(SHOCK_SENSOR_PIN,INPUT);
  pinMode(LIGHT_LED_PIN, OUTPUT);

}

void connectAWS()
{

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  writeLine("Connecting to wifi...");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  writeLine("Connecting to AWS...");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  client.subscribe(AWS_IOT_SHADOW_DELTA_TOPIC);

  writeLine("Connected to AWS!");
  delay(2000);
}


void publishShockMessage() {

  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["shockSensor"] = shockVal;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  Serial.println("Publishing shock to IoT...");
  client.publish(AWS_IOT_PUBLISH_SHOCK_TOPIC, jsonBuffer);

}

void messageHandler(String &topic, String &payload) {

  Serial.println("incoming: " + topic + " - " + payload);
  
  if (topic == "esp32/alert") {
    handleAlertMessage(payload);
  }
  else if (topic == AWS_IOT_SHADOW_DELTA_TOPIC) {
    handleShadowDelta(payload);
  }
  
}

void handleShadowDelta(String &payload) {

  StaticJsonDocument<200> doc; 
  deserializeJson(doc, payload);

  const bool deltaLightEnabled = doc["state"]["lightEnabled"];

  if (deltaLightEnabled.isNull()) {
    Serial.println("toggling light...");
    enableShadowLight(deltaLightEnabled);
  }

}

void enableShadowLight(bool enabled) {

  if (enabled) {
    Serial.println("Turning light on.");
    digitalWrite(LIGHT_LED_PIN,HIGH);
  }
  else {
    Serial.println("Turning light off.");
    digitalWrite(LIGHT_LED_PIN,LOW);
  }

  lightEnabled = enabled; 

}

// Just print received message to OLED display: 
void handleAlertMessage(String &payload) {

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const String message = doc["msg"];
  digitalWrite(MESSAGE_IN_PIN,HIGH);
  writeLine(message);
  delay(2000);
  digitalWrite(MESSAGE_IN_PIN,LOW);
}

void logo(){
  Heltec.display -> clear();
  Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
  Heltec.display -> display();
}

// Write a line to both serial out and OLED display: 
void writeLine(String str, bool writeToSerial) {
  if (writeToSerial) {
    Serial.println(str);
  }
  Heltec.display -> clear(); 
  Heltec.display -> drawString(10, 0, str);
  Heltec.display -> display();
}

void checkForShock() {
  
  shockVal=digitalRead(SHOCK_SENSOR_PIN);
  
  if(shockVal==HIGH) {
    digitalWrite(SHOCK_LED_PIN,HIGH);
    writeLine("Disturbance in force!");
    delay(1000);
    publishShockMessage();
    digitalWrite(SHOCK_LED_PIN,LOW);
  }
  else {
    digitalWrite(SHOCK_LED_PIN,LOW);
    writeLine("No motion...", false);
  }

}

void readTempHumidity() {

  int chk = DHT11.read(DHT11_PIN);
  humidity = (float)DHT11.humidity;
  float tempC = (float)DHT11.temperature;
  tempF = tempC * 9/5 + 32;

}

void updateShadow() {

  readTempHumidity();
  publishShadow();

}

void publishShadow() {

  DynamicJsonDocument doc(200);
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  reported["time"] = millis();
  reported["humidity"] = humidity;
  reported["tempFahrenheit"] = tempF;
  reported["lightEnabled"] = lightEnabled;
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  
  Serial.println("Publishing shadow to IoT...");
  client.publish(AWS_IOT_PUBLISH_SHADOW_TOPIC, jsonBuffer);

}
