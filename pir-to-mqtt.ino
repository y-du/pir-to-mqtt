/*
   Copyright 2020 Yann Dumont

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


const byte pir_pin{3};
const byte led_pin{1};
String client_id{PIR_ID};


WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(client_id);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
     delay(LOOP_DELAY);
  }
}


boolean reconnect() {
    mqtt_client.connect(client_id.c_str());
    return mqtt_client.connected();
  }


void setup() {
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  pinMode(pir_pin, INPUT);
  randomSeed(micros());
  client_id += "-" + String(random(0xffff), HEX);
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  setupWifi();
  digitalWrite(led_pin, HIGH);
}


unsigned long now{0};
unsigned long last_reconnect{0};
byte motion{false};
byte pir_init{false};
unsigned long last_msg{0};
String topic{String() + MQTT_TOPIC + "/" + PIR_ID + "/" + SRV_ID};


void loop() {
  if (!mqtt_client.connected()) {
    now = millis();
    if (now - last_reconnect > MQTT_RECONNECT) {
      last_reconnect = now;
      if (reconnect()) {
        last_reconnect = 0;
        digitalWrite(led_pin, LOW);
        delay(100);
        digitalWrite(led_pin, HIGH);
      }
    }
  } else {
    mqtt_client.loop();
    if (pir_init) {
      if (digitalRead(pir_pin) == HIGH) {
        if (motion == false) {
          motion = true;
        }
        now = millis();
        if (now - last_msg >= PUP_INTERVAL) {
          mqtt_client.publish(topic.c_str(), "1");
          last_msg = now;
        }
      } else {
        if (motion == true) {
          motion = false;
          last_msg = 0;
        }
      }
    } else {
      if (millis() >= PIR_INIT_TIME) {
        pir_init = true;
      }
    }
  }
  delay(LOOP_DELAY);
}
