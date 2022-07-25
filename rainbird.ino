#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

// Connection parms
#include <SSID.h>
#include <rainbirdMQTT.h>


// PubSubClient Settings
WiFiClient espClient;
PubSubClient client(espClient);
String switch1;
String switch2;
String strTopic;
String strPayload;


// Misc variables
unsigned long timestamp; 

int solenoide1 = D0; // Pin per elettrovalvola 1
int solenoide2 = D3; // Pin per elettrovalvola 2
unsigned long startsolenoide1 = 0;
unsigned long endsolenoide1 = 300000;
unsigned long startsolenoide2 = 0;
unsigned long endsolenoide2 = 300000;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timestamp = millis();
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  strTopic = String((char*)topic);

  if(strTopic == "HA/rainbird/solenoide1"){
    switch1 = String((char*)payload);
    Serial.println(switch1);

    if(switch1 == "RESET") {
      Serial.println("reset");
      ESP.restart();
    }
    
    if(switch1 == "OFF") {
      Serial.println("FERMA solenoide1");
      digitalWrite(solenoide1, LOW);
      startsolenoide1 = 0;
    }

    if(switch1 == "ON") {
      Serial.println("annaffia 1 e chiudo 2");
      digitalWrite(solenoide2, LOW); 
      startsolenoide2 = 0;
      client.publish("HA/rainbird/solenoide2/state", "OFF");
      startsolenoide1 = millis();
      digitalWrite(solenoide1, HIGH); 
    }
    switch1 = " ";
  }

  if(strTopic == "HA/rainbird/solenoide2"){
    switch2 = String((char*)payload);
    Serial.println(switch2);

    if(switch2 == "RESET") {
      Serial.println("reset");
      ESP.restart();
    }
    
    if(switch2 == "OFF") {
      Serial.println("FERMA solenoide2");
      digitalWrite(solenoide2, LOW);
      startsolenoide2 = 0;
    }

    if(switch2 == "ON") {
      Serial.println("annaffia 2 e chiudo 1");
      digitalWrite(solenoide1, LOW); 
      startsolenoide1 = 0;
      client.publish("HA/rainbird/solenoide1/state", "OFF");
      startsolenoide2 = millis();
      digitalWrite(solenoide2, HIGH); 
    }
    switch2 = " ";
  }

}
 
 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266_rainbird", MQTTuser, MQTTpwd)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe("HA/rainbird/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
 
void setup()
{
  Serial.begin(115200);
  
  pinMode(solenoide1, OUTPUT);
  digitalWrite(solenoide1, LOW);
  pinMode(solenoide2, OUTPUT);
  digitalWrite(solenoide2, LOW);
  
  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp8266-RAINBIRD");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if (startsolenoide1 > 0) {
    if ((millis() - startsolenoide1) > endsolenoide1 
     || (millis() - startsolenoide1) < 0) {
      digitalWrite(solenoide1, LOW);
      startsolenoide1 = 0;
      client.publish("HA/rainbird/solenoide1/state", "OFF");
      Serial.print("CHIUDO solenoide1");
    }
  }

  if (startsolenoide2 > 0) {
    if ((millis() - startsolenoide2) > endsolenoide2 
     || (millis() - startsolenoide2) < 0) {
      digitalWrite(solenoide2, LOW);
      startsolenoide2 = 0;
      client.publish("HA/rainbird/solenoide2/state", "OFF");
      Serial.print("CHIUDO solenoide2");
    }
  }

}
