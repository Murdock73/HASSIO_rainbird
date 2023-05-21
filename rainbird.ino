#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

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
int moisture1 = D7; //Pin per sensore umidità del terreno 1
int moisture2 = D6; //Pin per sensore umidità del terreno 1
int moisture_value = 0;
float moisture_percentage;


DHT dht(D8, DHT22);

unsigned long startsolenoide1 = 0;
const long endsolenoide1 = 600000;
unsigned long startsolenoide2 = 0;
const long endsolenoide2 = 600000;
unsigned long timestampD; 
const long intervalDHT = 60000;  
unsigned long timestampMoisture; 
const long intervalmoisture = 1800000;  

// current temperature & humidity, updated in loop()
float temp = 0.0;
float hum = 0.0;
char moisture_send[4];
char temp_send[5];
char hum_send[5];


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
  pinMode(D8, INPUT);
  pinMode(moisture1, OUTPUT);
  digitalWrite(moisture1, LOW);
  pinMode(moisture2, OUTPUT);
  digitalWrite(moisture2, LOW);

  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp8266-RAINBIRD");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123");

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
  
  dht.begin();
  delay(100);

}

void loop()
{
  ArduinoOTA.handle();
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

  if ((millis() - timestampD) > intervalDHT) {
    // save the last time you updated the DHT values
    timestampD = millis();
    // Read temperature as Celsius (the default) and Humidity
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    Serial.println(temp);
    Serial.println(hum);
    String(temp).toCharArray(temp_send, 5);
    String(hum).toCharArray(hum_send, 5);
    Serial.println(temp_send);
    Serial.println(hum_send);
    client.publish("sensor/TempOut", temp_send);
    client.publish("sensor/HumOut", hum_send);
            
  }

 if ((millis() - timestampMoisture) > intervalmoisture) {
    // save the last time you updated the DHT values
    timestampMoisture = millis();
    // Attivo il sensore 1 per leggere l'umidità del terreno e pubblico il valore
    digitalWrite(moisture1, HIGH);
    delay(100);
    moisture_value= analogRead(A0);
    Serial.print("sensore1 ");
    Serial.println(moisture_value);
    moisture_percentage = ( 100 - ( (moisture_value/1023.00) * 100 ) );
    String(moisture_percentage).toCharArray(moisture_send, 4);
    client.publish("sensor/GrassMoisture", moisture_send);
    digitalWrite(moisture1, LOW);    
     // Attivo il sensore 2 per leggere l'umidità del terreno e pubblico il valore
    digitalWrite(moisture2, HIGH);
    delay(100);
    moisture_value= analogRead(A0);
    Serial.print("sensore2 ");
    Serial.println(moisture_value);
    moisture_percentage = ( 100 - ( (moisture_value/1023.00) * 100 ) );
    String(moisture_percentage).toCharArray(moisture_send, 4);
    client.publish("sensor/OrtensieMoisture", moisture_send);
    digitalWrite(moisture2, LOW);    
            
  }




}
