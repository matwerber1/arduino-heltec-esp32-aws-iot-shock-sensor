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
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

const int ledPin = 13;      // LED with 100ohm resistor that will light up when shock occurs
const int shockPin = 12;    // I used the shock sensor from this kit: https://www.amazon.com/kuman-K5-USFor-Raspberry-Projects-Tutorials/dp/B016D5L5KE
int shockVal;


void setup()
{

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  logo();
  delay(300);
  Heltec.display->clear();
  pinMode(ledPin, OUTPUT);
  pinMode(shockPin,INPUT);
  WIFISetUp();
  connectAWS();
}


void WIFISetUp(void)
{

  writeLine("Connecting to wifi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int i = 1;

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    if (i > 3) {
      i = 1;
    }
    else {
      i ++;
    }

    delay(500);
    writeLine("Connecting to wifi " + repeatString(".", i));

  }
  
  writeLine("Wifi connected");
  delay(500);

}




void connectAWS()
{
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");
  writeLine("Connecting to AWS MQTT...");
  
  int i = 1;

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    if (i > 3) {
      i = 1;
    }
    else {
      i ++;
    }
    delay(500);
    writeLine("Connecting to IoT" + repeatString(".", i));
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    writeLine("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
  writeLine("Connected to AWS IoT!");

}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["shockSensor"] = shockVal;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
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

  shockVal=digitalRead(shockPin);
  
  if(shockVal==HIGH) {
    digitalWrite(ledPin,HIGH);
    writeLine("Disturbance in force!");
    delay(500);
    publishMessage();
    digitalWrite(ledPin,LOW);
  }
  else {
    digitalWrite(ledPin,LOW);
    writeLine("No motion...");
  }
}

void writeLine(String str) {
  Heltec.display -> clear(); 
  Heltec.display -> drawString(0, 0, str);
  Heltec.display -> display();
}