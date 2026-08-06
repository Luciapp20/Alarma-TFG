#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "secrets.h"

// --- 1. DATOS WIFI (desde secrets.h) ---
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// --- 2. CONFIGURACION NFC (PN532 por I2C) ---
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_PN532 nfc(-1, -1, &Wire);

// --- 3. TARJETA AUTORIZADA ---
String UID_AUTORIZADO = "5A E FB 3";

// --- 4. HARDWARE Y PINES ---
const int trigPin = 5;
const int echoPin = 27;
const int pinLed = 2;
const int pinBuzzer = 4;
const int pinBoton = 13;

// --- VARIABLES DE CONTROL ---
ModbusIP mb;
long duracion;
int distancia;
bool estadoBotonAnterior = HIGH;
unsigned long tiempoUltimaLectura = 0;
unsigned long ultimaLecturaNFC = 0;

void sonidoConfirmacion(int veces);
void sonidoError();

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pinLed, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinBoton, INPUT_PULLUP);

  // Inicio NFC
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Error: No se encontro el PN532");
  } else {
    Serial.println("Lector NFC OK");
    nfc.SAMConfig();
  }

  // WiFi
  Serial.print("Conectando WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Modbus TCP
  mb.server();
  mb.addHreg(0, 0); // Estado Alarma (0=Desarmada, 1=Armada)
  mb.addHreg(1, 0); // Deteccion movimiento (0=Limpio, 1=Intruso)
  mb.addHreg(2, 0); // Sirena manual / boton panico

  Serial.println("--- SISTEMA ONLINE ---");
}

void loop() {
  mb.task();

  // ==========================================
  // 1. LECTURA NFC (arma/desarma con tarjeta)
  // ==========================================
  if (millis() - ultimaLecturaNFC > 200) {
    uint8_t success;
    uint8_t uid[7];
    uint8_t uidLength;
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50);

    if (success) {
      String uidLeido = "";
      for (uint8_t i = 0; i < uidLength; i++) {
        uidLeido += String(uid[i], HEX);
        if (i < uidLength - 1) uidLeido += " ";
      }
      uidLeido.toUpperCase();

      Serial.print("UID: ");
      Serial.println(uidLeido);

      if (uidLeido == UID_AUTORIZADO) {
        Serial.println(">> LLAVE CORRECTA <<");

        int estadoActual = mb.Hreg(0);
        if (estadoActual == 0) {
          mb.Hreg(0, 1);
          Serial.println("Alarma ACTIVADA por tarjeta");
        } else {
          mb.Hreg(0, 0);
          Serial.println("Alarma DESACTIVADA por tarjeta");
        }
        sonidoConfirmacion(estadoActual == 0 ? 2 : 1);

      } else {
        Serial.println(">> NO AUTORIZADO <<");
        sonidoError();
      }
      delay(1000); // Anti-rebote de lectura continua
    }
    ultimaLecturaNFC = millis();
  }

  // ==========================================
  // 2. BOTON FISICO (alternar estado, redundante con NFC/app)
  // ==========================================
  int lecturaBoton = digitalRead(pinBoton);
  if (lecturaBoton == LOW && estadoBotonAnterior == HIGH) {
    int estado = mb.Hreg(0);
    mb.Hreg(0, !estado);
    delay(300);
  }
  estadoBotonAnterior = lecturaBoton;

  // ==========================================
  // 3. SENSOR ULTRASONICO
  // ==========================================
  if (millis() - tiempoUltimaLectura > 200) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duracion = pulseIn(echoPin, HIGH, 30000); // timeout de 30 ms
    distancia = duracion * 0.034 / 2;
    tiempoUltimaLectura = millis();

    if (distancia > 0 && distancia < 50) {
      mb.Hreg(1, 1);
    } else {
      mb.Hreg(1, 0);
    }
  }

  // ==========================================
  // 4. SIRENA Y LED
  // ==========================================
  if ((mb.Hreg(1) == 1 && mb.Hreg(0) == 1) || mb.Hreg(2) == 1) {
    digitalWrite(pinLed, HIGH);
    digitalWrite(pinBuzzer, HIGH);
  } else {
    digitalWrite(pinLed, LOW);
    digitalWrite(pinBuzzer, LOW);
  }
}

void sonidoConfirmacion(int veces) {
  for (int i = 0; i < veces; i++) {
    digitalWrite(pinBuzzer, HIGH);
    delay(100);
    digitalWrite(pinBuzzer, LOW);
    delay(100);
  }
}

void sonidoError() {
  digitalWrite(pinBuzzer, HIGH);
  delay(500);
  digitalWrite(pinBuzzer, LOW);
}
