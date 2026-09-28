#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "secrets.h"

#define DHTPIN 4
#define DHTTYPE DHT11

const char* MQTT_BROKER = "192.168.1.11";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "esp32-dht11";
const char* TOPIC_TEMPERATURA = "esp32/dht11/temperatura";
const char* TOPIC_HUMEDAD = "esp32/dht11/humedad";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void conectarWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - inicio > 15000) {
      Serial.println("\nNo se pudo conectar en 15s, reintentando...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      inicio = millis();
    }
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  while (!mqtt.connected()) {
    Serial.print("Conectando a MQTT (");
    Serial.print(MQTT_BROKER);
    Serial.print(")... ");

    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("conectado!");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(mqtt.state());
      Serial.println(" reintentando en 3s");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  conectarWiFi();
  conectarMQTT();
  Serial.println("Iniciando lectura del DHT11...");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, reconectando...");
    conectarWiFi();
  }

  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  delay(2000); // el DHT11 necesita ~2s entre lecturas

  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("Error al leer el sensor DHT11");
    return;
  }

  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.print(" %  Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" C");

  char payload[8];

  dtostrf(temperatura, 4, 1, payload);
  mqtt.publish(TOPIC_TEMPERATURA, payload);

  dtostrf(humedad, 4, 1, payload);
  mqtt.publish(TOPIC_HUMEDAD, payload);
}
