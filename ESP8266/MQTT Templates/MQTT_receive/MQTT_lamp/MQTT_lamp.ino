#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "ArduinoJson.h" 

#define rele_pin 4

const char *ssid = "{{ ssid }}";
const char *pass = "{{ pass }}";
const char *mqtt_server = "{{ ip_mqtt_server }}";
const int mqtt_port = {{ mqtt_port }};
const char *mqtt_user = "{{ mqtt_user }}";
const char *mqtt_pass = "{{ mqtt_password }}";

WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);

String received = "off";
int bright = 0;

void setup() {
  Serial.begin(115200);
  pinMode(rele_pin, OUTPUT);
  digitalWrite(rele_pin, LOW);
  WiFi.begin(ssid, pass);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);
 
  while (!mqtt_client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (mqtt_client.connect("ESP8266_Lamp", mqtt_user, mqtt_pass )) {
      Serial.println("connected");  
    } 
    else {
      Serial.print(" failed with state ");
      Serial.print(mqtt_client.state());
      delay(2000);
    }
  }
 
  mqtt_client.subscribe("pacs/lamp");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  payload[length] = '\0';
  String str_topic = String(topic);
  String str_payload = String((char*)payload);
  Serial.println("Payload: " + String(str_payload));

  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(str_payload);
  
  if (!root.success()) {
    Serial.println(("Parsing failed!"));
    return;
  }
  
  received = root["command"].as<String>(); 
  bright = root["bright"].as<String>().toInt(); 

  if(received == "on")
    analogWrite(rele_pin, bright);
  else if(received == "off")
    digitalWrite(rele_pin, LOW);
  
  jsonBuffer.clear();
}

void loop() {
  ArduinoOTA.handle();
  mqtt_client.loop();
}
