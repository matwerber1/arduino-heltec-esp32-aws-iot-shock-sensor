#include <MQTTClient.h>
#include <MQTT.h>       // Be sure to install the MQTT client from Arduino Library Manager authored by Joel Gaehwiler
#include "heltec.h"
#include "WiFi.h"
#include "images.h"
#include "secrets.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/shockAlert"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/alert"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

const int publishLedPin = 14; // LED with 100 ohm resistor that lights up when MQTT message received from AWS
const int shockLedPin = 13; // LED with 100ohm resistor that will light up when shock occurs
const int shockPin = 12;    // I used the shock sensor from this kit: https://www.amazon.com/kuman-K5-USFor-Raspberry-Projects-Tutorials/dp/B016D5L5KE
int shockVal;


void setup()
{
  Serial.println("Setting up...");
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  logo();
  delay(300);
  Heltec.display->clear();
  pinMode(shockLedPin, OUTPUT);
  pinMode(publishLedPin, OUTPUT);
  pinMode(shockPin,INPUT);
  connectAWS();
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  writeLine("Connecting to wifi...");
  Serial.println("Connecting to Wi-Fi");

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

  Serial.print("Connecting to AWS IOT");
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

  Serial.println("AWS IoT Connected!");
  writeLine("Connected to AWS!");
  delay(2000);
}


void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["shockSensor"] = shockVal;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  Serial.println("Publishing shock to IoT...");
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {

  Serial.println("incoming: " + topic + " - " + payload);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const String message = doc["msg"];

  digitalWrite(publishLedPin,HIGH);
  writeLine(message);
  delay(2000);
  digitalWrite(publishLedPin,LOW);
  
}


String repeatString(String myStr, int count) {
  String response = myStr;
  for (int i = 1; i < count; i++) {
    response = response + myStr;
  }
  return response;  
}


void logo(){
  Heltec.display -> clear();
  Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
  Heltec.display -> display();
}


void loop(){
  client.loop();

  shockVal=digitalRead(shockPin);
  
  if(shockVal==HIGH) {
    digitalWrite(shockLedPin,HIGH);
    Serial.println("Shock detected...");
    writeLine("Disturbance in force!");
    delay(500);
    publishMessage();
    digitalWrite(shockLedPin,LOW);
  }
  else {
    digitalWrite(shockLedPin,LOW);
    writeLine("No motion...");
  }
  
}

void writeLine(String str) {
  Heltec.display -> clear(); 
  Heltec.display -> drawString(0, 0, str);
  Heltec.display -> display();
}
