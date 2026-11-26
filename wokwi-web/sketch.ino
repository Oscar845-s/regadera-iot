/*
 * Regadera automatica IoT - version para simular en wokwi.com
 * (El firmware real del proyecto esta en ESP-IDF, carpeta main/.
 *  Este sketch replica la misma logica para probarla en el navegador.)
 *
 * Simulacion:
 *  - Potenciometro = sensor de humedad de suelo (girar = mas seco / mas humedo)
 *  - LED azul      = bomba de agua (se prende cuando el rele la activa)
 *  - Clic en el DHT22 para cambiar temperatura y humedad del aire
 */
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

// ---------- Adafruit IO (opcional: deja AIO_USERNAME vacio para no usar la nube) ----------
#define AIO_USERNAME  ""            // tu usuario de Adafruit IO
#define AIO_KEY       ""            // tu AIO Key (no la compartas)

// ---------- WiFi virtual de Wokwi ----------
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASS     ""

// ---------- Pines (mismos que el proyecto real) ----------
#define PIN_SUELO     34    // AO del sensor de suelo   (E12)
#define PIN_DHT       4     // DATA del DHT22           (f5)
#define PIN_RELE      26    // IN1 del rele             (E7)
#define RELE_ACTIVO_EN_BAJO  true   // si el LED funciona al reves, cambia a false

// ---------- Calibracion (potenciometro: 4095 = seco, 0 = mojado) ----------
#define VALOR_SECO    4095
#define VALOR_MOJADO  0

// ---------- Riego ----------
int umbral = 35;                     // regar si la humedad baja de este %
const unsigned long TIEMPO_RIEGO   = 3000;
const unsigned long ESPERA_ABSORCION = 20000;
const int MAX_RIEGOS_SIN_MEJORA = 3;
const int MEJORA_MINIMA = 3;

// ---------- Tiempos ----------
const unsigned long PERIODO_LECTURA = 2500;
const unsigned long PUBLICAR_CADA   = 20000;

DHTesp dht;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Estado de la logica de riego
int riegosSinMejora = 0;
int humedadAntesRiego = -1;
bool bloqueado = false;
bool riegoManualPendiente = false;

unsigned long proximaEvaluacion = 0;
unsigned long proximaPublicacion = 0;
unsigned long proximoIntentoMqtt = 0;

bool usarNube() { return strlen(AIO_USERNAME) > 0; }

String topico(const char *feed) {
  return String(AIO_USERNAME) + "/feeds/" + feed;
}

void publicar(const char *feed, const String &valor) {
  if (usarNube() && mqtt.connected()) {
    mqtt.publish(topico(feed).c_str(), valor.c_str());
  }
}

void rele(bool encender) {
  digitalWrite(PIN_RELE, (encender ^ RELE_ACTIVO_EN_BAJO) ? HIGH : LOW);
}

int leerHumedadSuelo() {
  long suma = 0;
  for (int i = 0; i < 16; i++) {
    suma += analogRead(PIN_SUELO);
    delay(2);
  }
  int crudo = suma / 16;
  int pct = (long)(VALOR_SECO - crudo) * 100 / (VALOR_SECO - VALOR_MOJADO);
  return constrain(pct, 0, 100);
}

void regar(const char *motivo) {
  Serial.printf(">> Regando %lu ms (%s)\n", TIEMPO_RIEGO, motivo);
  rele(true);
  publicar("bomba", "1");
  delay(TIEMPO_RIEGO);
  rele(false);
  publicar("bomba", "0");
}

// Misma logica que main/riego_logic.c del firmware ESP-IDF
enum Accion { NADA, REGAR, BLOQUEAR };

Accion decidir(int humedad) {
  if (bloqueado) return NADA;
  if (humedadAntesRiego >= 0) {
    if (humedad - humedadAntesRiego >= MEJORA_MINIMA) riegosSinMejora = 0;
    else riegosSinMejora++;
    humedadAntesRiego = -1;
  }
  if (riegosSinMejora >= MAX_RIEGOS_SIN_MEJORA) {
    bloqueado = true;
    return BLOQUEAR;
  }
  if (humedad < umbral) {
    humedadAntesRiego = humedad;
    return REGAR;
  }
  riegosSinMejora = 0;
  return NADA;
}

// Comandos desde el dashboard de Adafruit IO
void alRecibir(char *topic, byte *payload, unsigned int len) {
  String t(topic);
  String valor;
  for (unsigned int i = 0; i < len; i++) valor += (char)payload[i];
  Serial.printf("Recibido %s = %s\n", topic, valor.c_str());

  if (t.endsWith("/feeds/riego-manual") && valor.toInt() == 1) {
    riegoManualPendiente = true;
  } else if (t.endsWith("/feeds/umbral")) {
    int u = valor.toInt();
    if (u >= 5 && u <= 95) umbral = u;
  } else if (t.endsWith("/throttle")) {
    Serial.println("Adafruit IO: limite de datos excedido");
  }
}

void conectarMqtt() {
  if (!usarNube() || mqtt.connected() || WiFi.status() != WL_CONNECTED) return;
  if (millis() < proximoIntentoMqtt) return;

  Serial.print("Conectando a Adafruit IO... ");
  String estado = topico("estado");
  String id = "regadera-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqtt.connect(id.c_str(), AIO_USERNAME, AIO_KEY, estado.c_str(), 1, false, "offline")) {
    Serial.println("OK");
    mqtt.subscribe(topico("riego-manual").c_str());
    mqtt.subscribe(topico("umbral").c_str());
    mqtt.subscribe((String(AIO_USERNAME) + "/throttle").c_str());
    mqtt.publish((topico("umbral") + "/get").c_str(), "");
    mqtt.publish(estado.c_str(), "online");
    proximaPublicacion = 0;
  } else {
    Serial.printf("fallo (rc=%d), reintento en 5 s\n", mqtt.state());
    proximoIntentoMqtt = millis() + 5000;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RELE, OUTPUT);
  rele(false);
  analogReadResolution(12);
  dht.setup(PIN_DHT, DHTesp::DHT22);

  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);   // la red de Wokwi usa el canal 6
  mqtt.setServer("io.adafruit.com", 1883);
  mqtt.setCallback(alRecibir);
  Serial.println("Regadera IoT iniciada");
}

void loop() {
  conectarMqtt();
  mqtt.loop();

  static unsigned long ultimaLectura = 0;
  if (millis() - ultimaLectura < PERIODO_LECTURA) return;
  ultimaLectura = millis();

  int humedad = leerHumedadSuelo();
  TempAndHumidity aire = dht.getTempAndHumidity();
  bool dhtOk = dht.getStatus() == DHTesp::ERROR_NONE;

  Serial.printf("Suelo: %d %% | Aire: %.1f C, %.1f %% | Umbral: %d %% | %s%s\n",
                humedad, aire.temperature, aire.humidity, umbral,
                WiFi.status() == WL_CONNECTED ? "WiFi OK" : "sin WiFi",
                bloqueado ? " | BLOQUEADO" : "");

  if (riegoManualPendiente) {
    riegoManualPendiente = false;
    bloqueado = false; riegosSinMejora = 0; humedadAntesRiego = -1;
    regar("manual desde el dashboard");
    proximaEvaluacion = millis() + ESPERA_ABSORCION;
  }

  if (millis() >= proximaEvaluacion) {
    switch (decidir(humedad)) {
      case REGAR:
        regar("humedad baja");
        proximaEvaluacion = millis() + ESPERA_ABSORCION;
        break;
      case BLOQUEAR:
        Serial.println("ALERTA: riego bloqueado, la humedad no sube");
        publicar("alertas", "Riego bloqueado: revisar deposito o sensor");
        break;
      default:
        break;
    }
  }

  if (millis() >= proximaPublicacion && mqtt.connected()) {
    publicar("humedad-suelo", String(humedad));
    if (dhtOk) {
      publicar("temperatura", String(aire.temperature, 1));
      publicar("humedad-aire", String(aire.humidity, 1));
    }
    proximaPublicacion = millis() + PUBLICAR_CADA;
  }
}
