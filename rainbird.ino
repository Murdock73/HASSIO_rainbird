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

int ONLINE = D1; // LED blu per connessione ON
int solenoide1 = D0; // Pin per elettrovalvola 1
int solenoide2 = D3; // Pin per elettrovalvola 2
int soilsens1 = D6; // Pin per sensore terreno 1
int soilsens2 = D7; // Pin per sensore terreno 2
int soilval1;
int soilval2;
float soilperc1;
float soilperc2;

unsigned long startsolenoide1 = 0;
unsigned long endsolenoide1 = 600000;
unsigned long startsolenoide2 = 0;
unsigned long endsolenoide2 = 600000;
unsigned long startsoil = 0;
unsigned long soil = 3600000;

#include <setupwifi.h>

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
      switch2 = "OFF";
      startsolenoide1 = millis();
      digitalWrite(solenoide1, HIGH); 
    }
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
      switch1 = "OFF";
      startsolenoide2 = millis();
      digitalWrite(solenoide2, HIGH); 
    }
  }

}
 
 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266_rainbird", MQTTuser, MQTTpwd)) {
      Serial.println("connected");
        digitalWrite(ONLINE, HIGH);
      // Once connected, publish an announcement...
      client.subscribe("HA/rainbird/#");
    } else {
      digitalWrite(ONLINE, LOW);
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
  pinMode(ONLINE, OUTPUT);
  digitalWrite(ONLINE, LOW);
  pinMode(soilsense1, OUTPUT);
  digitalWrite(soilsense1, LOW);
  pinMode(soilsense2, OUTPUT);
  digitalWrite(soilsense2, LOW);
  
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
  
  if (WiFi.status() != WL_CONNECTED) 
    digitalWrite(ONLINE, LOW);
    digitalWrite(solenoide1, LOW);
    digitalWrite(solenoide2, LOW);
  }
  
  if (!client.connected()) {
    digitalWrite(ONLINE, LOW);
    digitalWrite(solenoide1, LOW);
    digitalWrite(solenoide2, LOW);
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
  
  // controllo di sicurezza per chiudere le valvole in caso di discrepanza tra gli stati
  if (digitalRead(solenoide1) == HIGH && switch1 == "OFF") {
    digitalWrite(solenoide1, LOW);
  }  
  
  if (digitalRead(solenoide2) == HIGH && switch2 == "OFF") {
    digitalWrite(solenoide2, LOW);
  }  

  if ((millis() - startsoil) > soil) {
    readsoil();         
  }  
}

void readsoil()
{
  // leggo e pubblico i valori dei sensori nel terreno 
  digitalWrite(soilsense1, HIGH);
  delay(100);
  soilval1 = analogRead(A0);
  digitalWrite(soilsense1, LOW);
  soilperc1 = (100 - ( (soilval1/1023.00) * 100 ) );
  client.publish("HA/rainbird/soil1/state", soilval1);
  client.publish("HA/rainbird/soil1p/state", soilperc1);
  Serial.print("SoilSense1 " + soilval1);
    // e 2
  digitalWrite(soilsense2, HIGH);
  delay(100);
  soilval2 = analogRead(A0);
  digitalWrite(soilsense2, LOW);
  soilperc2 = (100 - ( (soilval2/1023.00) * 100 ) );
  client.publish("HA/rainbird/soil2/state", soilval2);
  client.publish("HA/rainbird/soil2p/state", soilperc2);
  Serial.print("SoilSense2 " + soilval2);
}  
