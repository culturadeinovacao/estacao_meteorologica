/************************************
* Parque Tecnológico de Santo André *
* Rede Ciência Cidadã               *
* Estação Meteorológica Maker       *
* Autor: Matheus Valadares Teixeira *
*************************************/

// =====================================================
// BIBLIOTECAS DOS SENSORES E DISPLAY
// =====================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BH1750.h>

// =====================================================
// BIBLIOTECAS DE WI-FI E MQTT
// Compatíveis com Arduino UNO R4 WiFi
// =====================================================
#include <WiFiS3.h>
#include <PubSubClient.h>

// =====================================================
// CONFIGURAÇÕES DO WI-FI
// =====================================================
const char* WIFI_SSID = "SUA_INTERNET";
const char* WIFI_PASSWORD = "SENHA_DA_INTERNET";

// =====================================================
// CONFIGURAÇÕES DO MQTT
// Broker público gratuito da HiveMQ
// =====================================================
const char* MQTT_BROKER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

// Use um tópico específico para evitar conflito com outros usuários
const char* MQTT_TOPIC =
  "rcc/curuumaker/dados";

// O identificador deve ser único no broker
const char* MQTT_CLIENT_ID =
  "curuumaker_01";

// true mantém a última leitura armazenada no broker
const bool MQTT_RETAIN = true;

// Cliente TCP comum, pois a porta 1883 não utiliza TLS
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// =====================================================
// CONFIGURAÇÕES DA TELA OLED
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// CONFIGURAÇÕES DO DHT11
// =====================================================
#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================================
// CONFIGURAÇÕES DO GY-30 / BH1750
// =====================================================
BH1750 lightMeter;

// =====================================================
// SENSOR DE UMIDADE DO SOLO
// =====================================================
const int SOIL_PIN = A0;

// Valores obtidos durante a calibração
const int VALOR_SECO = 1020;
const int VALOR_UMIDO = 410;

// =====================================================
// INTERVALOS DE EXECUÇÃO
// =====================================================
const unsigned long INTERVALO_LEITURA = 15000;
const unsigned long INTERVALO_WIFI = 10000;
const unsigned long INTERVALO_MQTT = 5000;

unsigned long ultimaLeitura = 0;
unsigned long ultimaTentativaWiFi = 0;
unsigned long ultimaTentativaMQTT = 0;

// =====================================================
// CONEXÃO INICIAL COM O WI-FI
// =====================================================
void iniciarWiFi() {
  Serial.println();
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicioTentativa = millis();

  // Tenta conectar durante 15 segundos
  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - inicioTentativa < 15000
  ) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado com sucesso!");

    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Intensidade do sinal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("Nao foi possível conectar ao Wi-Fi.");
    Serial.println("A estacao continuara funcionando offline.");
  }
}

// =====================================================
// RECONEXÃO AUTOMÁTICA DO WI-FI
// =====================================================
void manterWiFiConectado() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimaTentativaWiFi < INTERVALO_WIFI) {
    return;
  }

  ultimaTentativaWiFi = agora;

  Serial.println("Wi-Fi desconectado. Tentando reconectar...");

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// =====================================================
// CONEXÃO E RECONEXÃO COM O MQTT
// =====================================================
void manterMQTTConectado() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimaTentativaMQTT < INTERVALO_MQTT) {
    return;
  }

  ultimaTentativaMQTT = agora;

  Serial.print("Conectando ao broker MQTT... ");

  // Broker público: não utiliza usuário nem senha
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println("conectado!");
  } else {
    Serial.print("falha. Codigo MQTT: ");
    Serial.println(mqttClient.state());
  }
}

// =====================================================
// PUBLICAÇÃO DOS DADOS NO MQTT
// =====================================================
void publicarDadosMQTT(
  float temperatura,
  float umidadeAr,
  bool dhtValido,
  int umidadeSolo,
  int soloRaw,
  float luminosidade,
  bool luminosidadeValida
) {
  if (!mqttClient.connected()) {
    Serial.println("Dados nao publicados: MQTT desconectado.");
    return;
  }

  /*
    Mensagem produzida:

    {
      "temperatura_ar_c": 25.4,
      "umidade_ar_pct": 63.2,
      "umidade_solo_pct": 47,
      "umidade_solo_raw": 730,
      "luminosidade_lux": 1250.0
    }
  */

  String payload;
  payload.reserve(200);

  payload = "{";

  payload += "\"temperatura_ar_c\":";
  if (dhtValido) {
    payload += String(temperatura, 1);
  } else {
    payload += "null";
  }

  payload += ",";

  payload += "\"umidade_ar_pct\":";
  if (dhtValido) {
    payload += String(umidadeAr, 1);
  } else {
    payload += "null";
  }

  payload += ",";

  payload += "\"umidade_solo_pct\":";
  payload += String(umidadeSolo);

  payload += ",";

  payload += "\"umidade_solo_raw\":";
  payload += String(soloRaw);

  payload += ",";

  payload += "\"luminosidade_lux\":";
  if (luminosidadeValida) {
    payload += String(luminosidade, 1);
  } else {
    payload += "null";
  }

  payload += "}";

  bool publicado = mqttClient.publish(
    MQTT_TOPIC,
    payload.c_str(),
    MQTT_RETAIN
  );

  if (publicado) {
    Serial.println("Dados publicados no MQTT:");
    Serial.println(payload);
  } else {
    Serial.println("Falha ao publicar os dados no MQTT.");
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(9600);

  // Inicializa a comunicação I2C
  Wire.begin();

  // Inicializa o display OLED
  if (!display.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  )) {
    Serial.println(
      F("Falha ao inicializar a tela OLED!")
    );

    for (;;) {
      // Interrompe o programa caso o OLED não seja encontrado
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.display();

  // Inicializa o DHT11
  dht.begin();

  // Inicializa o BH1750
  if (
    lightMeter.begin(
      BH1750::CONTINUOUS_HIGH_RES_MODE
    )
  ) {
    Serial.println(
      F("GY-30 inicializado com sucesso.")
    );
  } else {
    Serial.println(
      F("Erro ao inicializar o GY-30.")
    );
  }

  // Configura o broker MQTT
  mqttClient.setServer(
    MQTT_BROKER,
    MQTT_PORT
  );

  // Aumenta o espaço disponível para a mensagem JSON
  mqttClient.setBufferSize(256);

  // Intervalo máximo entre verificações da conexão
  mqttClient.setKeepAlive(30);

  iniciarWiFi();

  // Permite fazer a primeira leitura imediatamente
  ultimaLeitura = millis() - INTERVALO_LEITURA;
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // Mantém as conexões ativas
  manterWiFiConectado();
  manterMQTTConectado();

  unsigned long agora = millis();

  // Evita usar delay(), permitindo que o MQTT continue ativo
  if (agora - ultimaLeitura < INTERVALO_LEITURA) {
    return;
  }

  ultimaLeitura = agora;

  // ===================================================
  // LEITURA DO DHT11
  // ===================================================
  float umidadeAr = dht.readHumidity();
  float temperaturaAr = dht.readTemperature();

  bool leituraDHTValida =
    !isnan(umidadeAr) &&
    !isnan(temperaturaAr);

  if (!leituraDHTValida) {
    Serial.println(
      F("Falha na leitura do sensor DHT11!")
    );
  }

  // ===================================================
  // LEITURA DO BH1750
  // ===================================================
  float lux = lightMeter.readLightLevel();

  bool leituraLuxValida =
    !isnan(lux) &&
    lux >= 0;

  if (!leituraLuxValida) {
    Serial.println(
      F("Falha na leitura do sensor BH1750!")
    );
  }

  // ===================================================
  // LEITURA DO SENSOR DE SOLO
  // ===================================================
  int soloRaw = analogRead(SOIL_PIN);

  int umidadeSolo = map(
    soloRaw,
    VALOR_SECO,
    VALOR_UMIDO,
    0,
    100
  );

  umidadeSolo = constrain(
    umidadeSolo,
    0,
    100
  );

  // ===================================================
  // ATUALIZAÇÃO DA TELA OLED
  // ===================================================
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(15, 0);
  display.print("MONITOR DE SENSORES");

  display.drawFastHLine(
    0,
    10,
    128,
    SSD1306_WHITE
  );

  display.setCursor(0, 16);
  display.print("Temp. Ar: ");

  if (leituraDHTValida) {
    display.print(temperaturaAr, 1);
    display.println(" C");
  } else {
    display.println("Erro");
  }

  display.print("Umid. Ar: ");

  if (leituraDHTValida) {
    display.print(umidadeAr, 1);
    display.println(" %");
  } else {
    display.println("Erro");
  }

  display.print("Umid. Solo: ");
  display.print(umidadeSolo);
  display.println(" %");

  display.print("Luminos.: ");

  if (leituraLuxValida) {
    display.print(lux, 0);
    display.println(" lx");
  } else {
    display.println("Erro");
  }

  // Indicador da conexão MQTT
  display.setCursor(0, 54);
  display.print("MQTT: ");

  if (mqttClient.connected()) {
    display.print("ONLINE");
  } else {
    display.print("OFFLINE");
  }

  display.display();

  // ===================================================
  // PUBLICAÇÃO MQTT
  // ===================================================
  publicarDadosMQTT(
    temperaturaAr,
    umidadeAr,
    leituraDHTValida,
    umidadeSolo,
    soloRaw,
    lux,
    leituraLuxValida
  );
}
