# CuruMaker — Guia Técnico e do Colaborador

> **Projeto:** Rede Ciência Cidadã · Parque Tecnológico de Santo André  
> **Autor original:** Matheus Valadares Teixeira  
> **Plataforma principal:** Arduino UNO R4 WiFi  
> **Arquivo principal:** `Arduino_R4_test.ino`  
> **Comunicação:** Wi-Fi + MQTT  
> **Broker de desenvolvimento:** HiveMQ público

---

## Índice

1. [Visão geral](#1-visão-geral)
2. [Arquitetura do firmware](#2-arquitetura-do-firmware)
3. [Configuração e instalação](#3-configuração-e-instalação)
4. [Formato dos dados MQTT](#4-formato-dos-dados-mqtt)
5. [Referência das funções](#5-referência-das-funções)
6. [Intervalos de execução](#6-intervalos-de-execução)
7. [Exemplos de uso](#7-exemplos-de-uso)
8. [Armadilhas comuns e FAQ](#8-armadilhas-comuns-e-faq)
9. [Solução de problemas](#9-solução-de-problemas)
10. [Boas práticas e melhorias futuras](#10-boas-práticas-e-melhorias-futuras)

---

## 1. Visão geral

### 1.1 O que o firmware faz

O firmware transforma um **Arduino UNO R4 WiFi** em uma estação ambiental conectada. O sistema mede quatro variáveis, apresenta os valores localmente em um display OLED e envia as leituras para um broker MQTT.

| Grandeza | Sensor | Unidade / formato |
|---|---|---|
| Temperatura do ar | DHT11 | °C, uma casa decimal |
| Umidade relativa do ar | DHT11 | %, uma casa decimal |
| Umidade do solo | Sensor capacitivo | %, valor inteiro |
| Leitura bruta do solo | Sensor capacitivo | Valor do ADC |
| Luminosidade | GY-30 / BH1750 | lux |

A versão conectada acrescenta:

- conexão Wi-Fi usando `WiFiS3`;
- comunicação MQTT usando `PubSubClient`;
- publicação das leituras em JSON;
- reconexão automática ao Wi-Fi e ao broker;
- exibição do estado `ONLINE` ou `OFFLINE` no OLED;
- tratamento de dados inválidos sem convertê-los em medidas falsas;
- temporização não bloqueante baseada em `millis()`.

### 1.2 Finalidade científica e pedagógica

O CuruMaker integra a Rede Ciência Cidadã e busca facilitar a produção de dados ambientais hiperlocais. Em escolas, laboratórios e espaços públicos, o equipamento pode apoiar investigações sobre microclima, conforto térmico, iluminação, umidade do solo, comportamento de plantas, variação diária e diferenças ambientais entre bairros.

A conectividade MQTT transforma cada estação em um nó de uma rede de observação. Em vez de apenas exibir uma medida instantânea, o sistema pode alimentar séries temporais, dashboards, alertas, atividades de análise estatística e comparações espaciais entre diferentes unidades.

### 1.3 Limites da versão atual

O firmware não implementa armazenamento local, timestamp absoluto, criptografia TLS ou autenticação MQTT. O broker público deve ser considerado um ambiente de teste e demonstração. Para coleta institucional ou científica de longa duração, recomenda-se um broker privado e uma estratégia explícita de identificação, tempo, integridade e retenção dos dados.

---

## 2. Arquitetura do firmware

### 2.1 Fluxo geral

```text
setup()
  ├─ Inicializa Serial
  ├─ Inicializa barramento I2C
  ├─ Inicializa OLED
  ├─ Inicializa DHT11
  ├─ Inicializa BH1750
  ├─ Configura o cliente MQTT
  └─ Tenta conectar ao Wi-Fi

loop()
  ├─ Verifica e recupera a conexão Wi-Fi
  ├─ Verifica e recupera a conexão MQTT
  ├─ Executa mqttClient.loop()
  └─ Quando INTERVALO_LEITURA é atingido:
       ├─ Lê DHT11
       ├─ Lê BH1750
       ├─ Lê sensor de solo
       ├─ Valida os dados
       ├─ Atualiza o OLED
       └─ Publica o JSON no MQTT
```

### 2.2 Caminho dos dados

```text
Sensores
   │
   ▼
Leitura e validação
   │
   ├──► OLED
   ├──► Monitor Serial
   └──► JSON ──► MQTT ──► HiveMQ ──► aplicações consumidoras
```

As aplicações consumidoras podem incluir Node-RED, n8n, Python, bancos de dados de séries temporais, Grafana, aplicativos web e outras plataformas compatíveis com MQTT.

### 2.3 Estratégia de temporização

O firmware não utiliza um `delay(2000)` como temporizador principal. Em vez disso, compara o valor atual de `millis()` com o instante da última leitura:

```cpp
unsigned long agora = millis();

if (agora - ultimaLeitura < INTERVALO_LEITURA) {
  return;
}

ultimaLeitura = agora;
```

Essa abordagem permite que o programa continue verificando as conexões enquanto aguarda o próximo ciclo de sensores. Isso é importante porque o cliente MQTT precisa executar `mqttClient.loop()` periodicamente para manter a sessão ativa.

---

## 3. Configuração e instalação

### 3.1 Hardware

| Componente | Modelo | Interface | Pino / endereço |
|---|---|---|---|
| Microcontrolador | Arduino UNO R4 WiFi | Wi-Fi / GPIO / I2C / ADC | — |
| Temperatura e umidade do ar | DHT11 | Digital | D2 |
| Luminosidade | GY-30 / BH1750 | I2C | `0x23` por padrão |
| Display | SSD1306 128×64 | I2C | `0x3C` por padrão |
| Umidade do solo | Capacitivo | Analógica | A0 |

A versão atual depende da conectividade integrada do **UNO R4 WiFi**. O UNO R4 Minima pode executar a parte de sensores e OLED, mas não a comunicação Wi-Fi do firmware sem hardware adicional.

### 3.2 Diagrama de conexões

```text
Arduino UNO R4 WiFi       DHT11
D2 ────────────────────── DATA
5V ────────────────────── VCC
GND ───────────────────── GND

Arduino UNO R4 WiFi       SSD1306              GY-30 / BH1750
A4 / SDA ──────────────── SDA ──────────────── SDA
A5 / SCL ──────────────── SCL ──────────────── SCL
3.3V ou 5V* ───────────── VCC ──────────────── VCC
GND ───────────────────── GND ──────────────── GND

Arduino UNO R4 WiFi       Sensor capacitivo de solo
A0 ────────────────────── AOUT
5V* ───────────────────── VCC
GND ───────────────────── GND
```

`*` Confirme a tensão aceita pelos módulos específicos utilizados. Módulos visualmente semelhantes podem empregar reguladores e níveis lógicos diferentes.

### 3.3 Bibliotecas

Instale pela Arduino IDE:

| Biblioteca | Finalidade | Instalação |
|---|---|---|
| `Adafruit GFX Library` | Primitivas gráficas | Gerenciador de Bibliotecas |
| `Adafruit SSD1306` | Controle do OLED | Gerenciador de Bibliotecas |
| `DHT sensor library` | Leitura do DHT11 | Gerenciador de Bibliotecas |
| `Adafruit Unified Sensor` | Dependência do DHT | Gerenciador de Bibliotecas |
| `BH1750` | Leitura de luminosidade | Gerenciador de Bibliotecas |
| `PubSubClient` | Cliente MQTT | Gerenciador de Bibliotecas |
| `Wire` | Comunicação I2C | Nativa |
| `WiFiS3` | Wi-Fi do UNO R4 WiFi | Pacote da placa |

### 3.4 Configuração do Wi-Fi

Edite:

```cpp
const char* WIFI_SSID = "NOME_DO_WIFI";
const char* WIFI_PASSWORD = "SENHA_DO_WIFI";
```

O firmware tenta se conectar durante a inicialização. Caso a conexão falhe, a estação continua realizando as leituras e exibindo os valores localmente. Novas tentativas são realizadas durante o `loop()`.

> Para um repositório público, não envie credenciais reais ao Git. Uma melhoria recomendada é mover esses valores para `arduino_secrets.h` e ignorar esse arquivo com `.gitignore`.

### 3.5 Configuração do broker MQTT

```cpp
const char* MQTT_BROKER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
```

A porta `1883` estabelece uma conexão MQTT sem TLS. O broker público da HiveMQ não exige usuário e senha para testes.

### 3.6 Tópico e identificador da estação

```cpp
const char* MQTT_TOPIC =
  "rede-ciencia-cidada/santo-andre/estacao-001/dados";

const char* MQTT_CLIENT_ID =
  "estacao-maker-santo-andre-001";
```

O tópico representa a posição lógica da estação dentro da rede. Uma convenção recomendada é:

```text
rede-ciencia-cidada/<municipio>/<estacao>/dados
```

Exemplos:

```text
rede-ciencia-cidada/santo-andre/estacao-001/dados
rede-ciencia-cidada/santo-andre/estacao-002/dados
rede-ciencia-cidada/santo-andre/estacao-parque-central/dados
```

O `MQTT_CLIENT_ID` precisa ser único entre conexões simultâneas. Quando dois dispositivos se conectam com o mesmo ID, o broker geralmente encerra a conexão anterior.

### 3.7 Retenção da última mensagem

```cpp
const bool MQTT_RETAIN = true;
```

Com `retain = true`, o broker armazena a última mensagem do tópico e a entrega imediatamente a novos assinantes. Isso é útil para dashboards, pois permite mostrar a leitura mais recente sem esperar o próximo ciclo.

A mensagem retida não constitui um histórico. Para séries temporais, um consumidor deve registrar cada publicação em um banco de dados ou arquivo.

### 3.8 Calibração do sensor de solo

O sensor de solo deve ser calibrado para o conjunto real de placa, alimentação, cabos e sensor.

```cpp
const int VALOR_SECO = 1020;
const int VALOR_UMIDO = 410;
```

Procedimento sugerido:

1. Mostre temporariamente `soloRaw` no Monitor Serial.
2. Registre várias leituras na condição adotada como seca.
3. Registre várias leituras na condição adotada como úmida.
4. Utilize valores representativos, preferencialmente médias ou medianas.
5. Atualize `VALOR_SECO` e `VALOR_UMIDO`.
6. Repita a calibração caso a alimentação, o sensor ou o substrato sejam alterados.

O cálculo utilizado é:

```cpp
int umidadeSolo = map(
  soloRaw,
  VALOR_SECO,
  VALOR_UMIDO,
  0,
  100
);

umidadeSolo = constrain(umidadeSolo, 0, 100);
```

A porcentagem resultante é uma escala calibrada para o projeto, não uma medição universal de teor volumétrico de água no solo.

---

## 4. Formato dos dados MQTT

### 4.1 Tópico padrão

```text
rede-ciencia-cidada/santo-andre/estacao-001/dados
```

### 4.2 Payload JSON

```json
{
  "temperatura_ar_c": 24.7,
  "umidade_ar_pct": 61.3,
  "umidade_solo_pct": 52,
  "umidade_solo_raw": 702,
  "luminosidade_lux": 1384.0
}
```

### 4.3 Dicionário de campos

| Campo | Tipo esperado | Unidade | Origem |
|---|---|---|---|
| `temperatura_ar_c` | Número ou `null` | °C | DHT11 |
| `umidade_ar_pct` | Número ou `null` | % | DHT11 |
| `umidade_solo_pct` | Inteiro | % calibrada | Sensor capacitivo |
| `umidade_solo_raw` | Inteiro | Valor ADC | Sensor capacitivo |
| `luminosidade_lux` | Número ou `null` | lux | BH1750 |

A inclusão de `umidade_solo_raw` permite auditar a calibração e investigar saturações em 0% ou 100%.

### 4.4 Leituras inválidas

Quando o DHT11 falha, o firmware produz:

```json
{
  "temperatura_ar_c": null,
  "umidade_ar_pct": null,
  "umidade_solo_pct": 52,
  "umidade_solo_raw": 702,
  "luminosidade_lux": 1384.0
}
```

Quando o BH1750 falha, `luminosidade_lux` é enviado como `null`.

O uso de `null` preserva a diferença entre “o sensor mediu zero” e “não houve medida válida”. Essa distinção é essencial para análise de dados e para impedir que falhas técnicas contaminem médias, mínimos e gráficos.

### 4.5 Assinatura com curingas

Para receber todas as estações de Santo André:

```text
rede-ciencia-cidada/santo-andre/+/dados
```

Para receber todas as mensagens abaixo do projeto:

```text
rede-ciencia-cidada/#
```

Curingas amplos são úteis para desenvolvimento, mas consumidores de produção devem assinar apenas os ramos necessários.

---

## 5. Referência das funções

### 5.1 `setup()`

Executada uma vez após ligar ou reiniciar a placa.

Responsabilidades:

1. inicia o Monitor Serial;
2. inicia o barramento I2C;
3. inicia o display OLED;
4. inicia o DHT11;
5. inicia o BH1750;
6. configura broker, buffer e keep-alive do MQTT;
7. inicia a conexão Wi-Fi;
8. prepara a primeira leitura imediata.

Trechos relevantes:

```cpp
mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
mqttClient.setBufferSize(256);
mqttClient.setKeepAlive(30);
```

O buffer de `256` bytes é suficiente para o JSON atual. Caso novos campos sejam adicionados, verifique se o payload continua cabendo nesse limite.

### 5.2 `iniciarWiFi()`

Realiza a tentativa inicial de conexão à rede.

```cpp
void iniciarWiFi();
```

A função aguarda por até aproximadamente 15 segundos. Se não conseguir conectar, registra a falha no Serial e permite que a estação continue operando offline.

### 5.3 `manterWiFiConectado()`

Verifica o estado da rede e inicia uma nova tentativa quando necessário.

```cpp
void manterWiFiConectado();
```

A função usa `INTERVALO_WIFI` para impedir tentativas contínuas e bloqueantes.

### 5.4 `manterMQTTConectado()`

Mantém a sessão MQTT e executa o processamento interno do cliente.

```cpp
void manterMQTTConectado();
```

Comportamento:

- retorna imediatamente se o Wi-Fi estiver desconectado;
- chama `mqttClient.loop()` quando a sessão está ativa;
- tenta reconectar após `INTERVALO_MQTT` quando a sessão está inativa;
- usa `MQTT_CLIENT_ID` durante a conexão.

> `INTERVALO_MQTT` controla tentativas de reconexão, não a frequência de publicação.

### 5.5 `publicarDadosMQTT(...)`

Monta o JSON e o envia ao tópico configurado.

```cpp
void publicarDadosMQTT(
  float temperatura,
  float umidadeAr,
  bool dhtValido,
  int umidadeSolo,
  int soloRaw,
  float luminosidade,
  bool luminosidadeValida
);
```

A função:

- verifica se o MQTT está conectado;
- reserva memória para a `String`;
- inclui números válidos ou `null`;
- publica com a opção `MQTT_RETAIN`;
- registra sucesso ou falha no Monitor Serial.

### 5.6 `loop()`

Executada continuamente.

O `loop()` está dividido em duas camadas temporais:

1. manutenção contínua das conexões;
2. ciclo periódico de leitura e publicação.

```cpp
void loop() {
  manterWiFiConectado();
  manterMQTTConectado();

  unsigned long agora = millis();

  if (agora - ultimaLeitura < INTERVALO_LEITURA) {
    return;
  }

  ultimaLeitura = agora;

  // Leituras, OLED e publicação MQTT
}
```

Essa estrutura evita bloquear a comunicação durante a espera entre amostras.

---

## 6. Intervalos de execução

### 6.1 Intervalo de leitura e publicação

A constante que controla a frequência das leituras e do envio MQTT é:

```cpp
const unsigned long INTERVALO_LEITURA = 2000;
```

O valor é dado em milissegundos.

| Intervalo desejado | Valor |
|---|---:|
| 2 segundos | `2000` |
| 5 segundos | `5000` |
| 10 segundos | `10000` |
| 30 segundos | `30000` |
| 1 minuto | `60000` |
| 5 minutos | `300000` |

No firmware atual, uma única constante controla três ações:

- leitura dos sensores;
- atualização do display;
- publicação MQTT.

Portanto, alterar `INTERVALO_LEITURA` modifica todo o ciclo de aquisição.

### 6.2 Intervalo de reconexão Wi-Fi

```cpp
const unsigned long INTERVALO_WIFI = 10000;
```

Define quanto tempo o firmware aguarda entre tentativas de recuperar o Wi-Fi.

### 6.3 Intervalo de reconexão MQTT

```cpp
const unsigned long INTERVALO_MQTT = 5000;
```

Define quanto tempo o firmware aguarda entre tentativas de recuperar a sessão MQTT.

Essa constante não controla o envio regular das leituras.

### 6.4 Separando leitura, OLED e publicação

Em aplicações reais, pode ser útil ler o DHT11 a cada 2 segundos, atualizar o display a cada 2 segundos e publicar apenas a cada 30 segundos. Para isso, crie temporizadores independentes:

```cpp
const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_ENVIO_MQTT = 30000;

unsigned long ultimaLeitura = 0;
unsigned long ultimoEnvioMQTT = 0;
```

Depois, separe as verificações no `loop()`. Essa alteração reduz tráfego e armazenamento sem diminuir a responsividade local.

---

## 7. Exemplos de uso

### 7.1 Teste com cliente MQTT

Conecte um cliente MQTT com os seguintes parâmetros:

```text
Host: broker.hivemq.com
Porta: 1883
Usuário: não utilizado
Senha: não utilizada
```

Assine:

```text
rede-ciencia-cidada/santo-andre/estacao-001/dados
```

A cada ciclo, uma mensagem JSON deverá aparecer.

### 7.2 Alterar o intervalo para 30 segundos

```cpp
const unsigned long INTERVALO_LEITURA = 30000;
```

Isso fará com que sensores, OLED e MQTT sejam atualizados a cada 30 segundos.

### 7.3 Adicionar uma nova estação

Na segunda placa:

```cpp
const char* MQTT_TOPIC =
  "rede-ciencia-cidada/santo-andre/estacao-002/dados";

const char* MQTT_CLIENT_ID =
  "estacao-maker-santo-andre-002";
```

Não basta alterar apenas o tópico. O ID do cliente também deve ser único.

### 7.4 Consumir várias estações

Assine o tópico curinga:

```text
rede-ciencia-cidada/santo-andre/+/dados
```

A aplicação consumidora pode extrair o identificador da estação do próprio tópico.

### 7.5 Log serial esperado

```text
GY-30 inicializado com sucesso.
Conectando ao Wi-Fi: NOME_DO_WIFI
Wi-Fi conectado com sucesso!
Endereco IP: 192.168.0.120
Conectando ao broker MQTT... conectado!
Dados publicados no MQTT:
{"temperatura_ar_c":24.7,"umidade_ar_pct":61.3,"umidade_solo_pct":52,"umidade_solo_raw":702,"luminosidade_lux":1384.0}
```

---

## 8. Armadilhas comuns e FAQ

### 8.1 O MQTT conecta e desconecta repetidamente

Verifique se outra placa, programa ou teste está usando o mesmo `MQTT_CLIENT_ID`. IDs duplicados podem provocar uma disputa em que cada conexão derruba a outra.

Use um identificador único por estação.

### 8.2 O display mostra `MQTT: OFFLINE`, mas os sensores funcionam

Isso significa que a aquisição local está ativa, porém o Wi-Fi ou o broker não está disponível. Verifique o Monitor Serial para distinguir:

- falha de conexão Wi-Fi;
- falha de resolução do endereço do broker;
- rejeição ou queda da sessão MQTT.

### 8.3 Alterei `INTERVALO_MQTT`, mas a frequência de publicação não mudou

Esse valor controla somente a espera entre tentativas de reconexão.

Altere:

```cpp
const unsigned long INTERVALO_LEITURA = 2000;
```

### 8.4 O DHT11 retorna falhas frequentes

Possíveis causas:

- intervalo de leitura inadequado;
- fiação longa ou instável;
- ausência de resistor pull-up quando necessário;
- alimentação incorreta;
- sensor com defeito;
- biblioteca ou modelo configurado incorretamente.

Mantenha `INTERVALO_LEITURA` em pelo menos 2 segundos na configuração atual, a menos que leitura e publicação sejam separadas em temporizadores diferentes.

### 8.5 O JSON contém `null`

`null` indica que não houve leitura válida naquele ciclo. Não é uma falha do formato JSON. O consumidor deve tratar esse valor como dado ausente e não como zero.

### 8.6 A mensagem não aparece imediatamente ao assinar

Com `MQTT_RETAIN = true`, a última mensagem válida normalmente é entregue imediatamente. Se isso não acontecer, verifique:

- se houve ao menos uma publicação bem-sucedida;
- se o tópico do cliente é exatamente o mesmo;
- se o firmware foi alterado para `MQTT_RETAIN = false`;
- se o cliente está usando uma sessão ou filtro diferente.

### 8.7 Posso publicar cada sensor em um tópico separado?

Sim. Por exemplo:

```text
rede-ciencia-cidada/santo-andre/estacao-001/temperatura
rede-ciencia-cidada/santo-andre/estacao-001/umidade-ar
rede-ciencia-cidada/santo-andre/estacao-001/umidade-solo
rede-ciencia-cidada/santo-andre/estacao-001/luminosidade
```

A mensagem única em JSON reduz o número de publicações e mantém as leituras de um ciclo agrupadas. Tópicos separados facilitam consumidores muito simples. A escolha depende da arquitetura do sistema.

### 8.8 O OLED não inicializa

O endereço mais comum é `0x3C`, mas alguns displays usam `0x3D`. Utilize um scanner I2C e altere:

```cpp
display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
```

### 8.9 O BH1750 retorna erro ou zero lux

O endereço padrão costuma ser `0x23`. Alguns módulos podem usar `0x5C` de acordo com a configuração do pino `ADDR`.

```cpp
lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C);
```

### 8.10 O sensor de solo permanece em 0% ou 100%

Leia `soloRaw` e refaça a calibração. O `constrain()` limita a saída visual, mas não corrige referências de calibração inadequadas.

---

## 9. Solução de problemas

### 9.1 Wi-Fi não conecta

Checklist:

- [ ] SSID e senha estão corretos?
- [ ] A rede opera em faixa compatível com a placa?
- [ ] O sinal está suficientemente forte?
- [ ] O firmware do módulo de conectividade da placa está atualizado?
- [ ] A rede exige página de autenticação, cadastro ou login institucional?
- [ ] O roteador bloqueia novos dispositivos ou clientes IoT?

Redes com portal cativo não funcionam apenas com SSID e senha no firmware.

### 9.2 MQTT não conecta

Checklist:

- [ ] O Wi-Fi está conectado?
- [ ] O host é `broker.hivemq.com`?
- [ ] A porta é `1883`?
- [ ] O `MQTT_CLIENT_ID` é único?
- [ ] A rede permite saída pela porta MQTT?
- [ ] O broker público está acessível naquele momento?

O valor retornado por `mqttClient.state()` ajuda no diagnóstico. Registre o código exibido no Serial antes de alterar diversas partes do firmware simultaneamente.

### 9.3 MQTT conecta, mas a publicação falha

Possíveis causas:

- buffer menor que o payload;
- sessão caiu entre a leitura e a publicação;
- tópico incorreto;
- problema temporário de rede;
- crescimento excessivo do JSON após novas funcionalidades.

O firmware configura:

```cpp
mqttClient.setBufferSize(256);
```

Aumente esse valor caso o JSON seja ampliado.

### 9.4 Dados congelados no dashboard

Verifique se o dashboard está mostrando apenas a mensagem retida. Confirme no Monitor Serial se novas publicações continuam ocorrendo e se o consumidor está atualizando a interface a cada mensagem.

Também confira se o intervalo foi aumentado inadvertidamente para vários minutos.

### 9.5 Leituras inválidas contaminando gráficos

O firmware atual publica `null`, mas o pipeline de dados precisa preservar esse valor. Algumas ferramentas podem converter `null` em zero durante transformações, importações CSV ou preenchimentos automáticos. Configure o sistema consumidor para armazenar dado ausente como ausente.

### 9.6 Texto cortado no OLED

O display de 128×64 com fonte padrão possui espaço vertical limitado. A linha de status MQTT ocupa a região inferior. Ao adicionar novas informações, ajuste cuidadosamente os valores de `setCursor(x, y)` ou implemente alternância de páginas.

---

## 10. Boas práticas e melhorias futuras

### 10.1 Separar credenciais do código

Crie um arquivo não versionado:

```cpp
// arduino_secrets.h
#define SECRET_SSID "nome-da-rede"
#define SECRET_PASS "senha-da-rede"
```

No firmware:

```cpp
#include "arduino_secrets.h"

const char* WIFI_SSID = SECRET_SSID;
const char* WIFI_PASSWORD = SECRET_PASS;
```

Adicione `arduino_secrets.h` ao `.gitignore`.

### 10.2 Usar broker privado, autenticação e TLS

O broker público é adequado para experimentação. Em produção, utilize:

- usuário e senha por dispositivo ou grupo;
- TLS;
- políticas de autorização por tópico;
- logs de conexão;
- limites de publicação;
- política de retenção e backup.

### 10.3 Adicionar timestamp

O payload atual não informa quando a medida foi produzida. Para séries temporais confiáveis, adicione um timestamp obtido por RTC ou sincronização de rede.

Exemplo conceitual:

```json
{
  "timestamp": "2026-07-30T09:45:00-03:00",
  "temperatura_ar_c": 24.7,
  "umidade_ar_pct": 61.3,
  "umidade_solo_pct": 52,
  "umidade_solo_raw": 702,
  "luminosidade_lux": 1384.0
}
```

### 10.4 Publicar estado de disponibilidade

Uma evolução natural é adotar um tópico de estado:

```text
rede-ciencia-cidada/santo-andre/estacao-001/status
```

Com mensagens como `online` e `offline`, incluindo Last Will and Testament do MQTT. Isso permite diferenciar uma estação sem variação ambiental de uma estação desconectada.

### 10.5 Implementar buffer offline

Atualmente, uma leitura produzida sem conexão MQTT é perdida. Para campanhas científicas, considere armazenar temporariamente os dados em memória, cartão SD ou outro meio e reenviá-los após a reconexão.

### 10.6 Separar intervalos

Use temporizadores independentes para:

- aquisição dos sensores;
- atualização do OLED;
- publicação MQTT;
- armazenamento local;
- envio de diagnóstico.

Essa separação permite preservar uma interface local responsiva enquanto reduz tráfego e volume de dados.

### 10.7 Evitar fragmentação de memória

O firmware atual usa `String` para montar o JSON, com `reserve(200)` para reduzir realocações. Em versões maiores ou de longa duração, considere um buffer de caracteres com `snprintf()` ou uma biblioteca JSON dimensionada adequadamente.

### 10.8 Identificação e metadados

Além do tópico, uma rede de ciência cidadã pode registrar:

- ID persistente da estação;
- versão do firmware;
- modelo e versão dos sensores;
- data de calibração;
- localização em nível adequado de privacidade;
- responsável técnico;
- qualidade do sinal Wi-Fi;
- tensão de alimentação;
- código de erro ou status dos sensores.

### 10.9 Qualidade científica dos dados

A conectividade não transforma automaticamente um sensor de baixo custo em instrumento científico. Para uso comparativo, documente:

- protocolo de instalação;
- abrigo e exposição dos sensores;
- altura e posição da estação;
- frequência de amostragem;
- calibração e substituição de componentes;
- critérios de exclusão de dados;
- períodos de indisponibilidade;
- transformações realizadas no pipeline.

A força do projeto está na combinação entre instrumentação acessível, transparência metodológica e interpretação crítica dos dados.

---

*Documentação atualizada para a versão do CuruMaker com Arduino UNO R4 WiFi, publicação MQTT e broker HiveMQ de desenvolvimento.*
