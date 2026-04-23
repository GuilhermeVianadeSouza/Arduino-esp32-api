# include <WiFi.h>
# include <HTTPClient.h>
# include "AdafruitIO_WiFi.h"
# include <Adafruit_Sensor.h>
# include <DHT.h>

//-- Adafruit IO ------------
#define IO_USERNAME ""
#define IO_KEY ""

//-------------------- WIFI --------------------
#define WIFI_SSID ""
#define WIFI_PASS ""


//------------------------ API RENDER -------------------------
const char* serverBaseResource = "https://arduino-esp32-api.onrender.com/sensor";

// ----------------------------- DHT11 ------------------
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

//---------------------------- Conexão ADAFRUIT--------------------------
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

AdafruitIO_Feed *temperatura = io.feed("temperatura");
AdafruitIO_Feed *umidade = io.feed("umidade");

// ------------------------- Váriaveis -------------------------------
float ultimaTemp = -1000;
float ultimaUmidade = -1000;

unsigned long tempoAnterior = 0;
const unsigned long intervaloLeitura = 5000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();

  Serial.print("Iniciando conexão com Adafruit IO...");
  io.connect();
  //loop com diagnóstico
  while (io.status() < AIO_CONNECTED) {
    Serial.print("Status Adafruit: ");
    Serial.println(io.statusText());
    delay(1000);
  }
  Serial.println("Conectado ao Adafruit IO e à rede WiFi!");
}

void loop() {
  io.run();

  if(io.status() != AIO_CONNECTED) return;

  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnterior >= intervaloLeitura) {
    tempoAnterior = tempoAtual;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if(isnan(temp) || isnan(hum)) return;

    bool houveVariacao = false;

    if(abs(temp - ultimaTemp) >= 0.2) {
      temperatura->save(temp);
      ultimaTemp = temp;
      houveVariacao = true;
    }

    if(abs(hum - ultimaUmidade) >= 1.0){
      umidade->save(hum);
      ultimaUmidade = hum;
      houveVariacao = true;
    }

    if (houveVariacao) {
      if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        // Montagem da URL com Query Parameters
        String urlEnvio = String(serverBaseResource) + 
                          "?temp=" + String(ultimaTemp) + 
                          "&hum=" + String(ultimaUmidade);
        
        Serial.print("Requisitando via GET: ");
        Serial.println(urlEnvio);

        http.begin(urlEnvio); 
        int httpResponseCode = http.GET(); // Mudança para GET

        if (httpResponseCode > 0) {
          Serial.print("Resposta do Render: ");
          Serial.println(httpResponseCode);
        } else {
          Serial.print("Erro no GET: ");
          Serial.println(httpResponseCode);
        }
        http.end();
      }
    }
  }
}
