# 🌦️ CuruMaker

<div align="center">

![Arduino](https://img.shields.io/badge/Arduino-UNO_R4_WiFi-00979D?style=for-the-badge&logo=arduino&logoColor=white)
![MQTT](https://img.shields.io/badge/MQTT-HiveMQ-660066?style=for-the-badge&logo=mqtt&logoColor=white)
![License](https://img.shields.io/badge/Licença-MIT-green?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Ativo-brightgreen?style=for-the-badge)

**Firmware open source para monitoramento ambiental conectado e de baixo custo**  
Parte da **Rede Ciência Cidadã** · Parque Tecnológico de Santo André

</div>

---

## 📖 Sobre o Projeto

O **CuruMaker** é uma estação ambiental maker baseada no **Arduino UNO R4 WiFi**. O dispositivo coleta dados de temperatura e umidade do ar, umidade do solo e luminosidade, apresenta as leituras localmente em um display OLED e publica os dados pela rede usando o protocolo **MQTT**.

O projeto integra a **Rede Ciência Cidadã**, iniciativa que distribui estações para escolas e espaços públicos de Santo André com o objetivo de produzir dados ambientais em escala local e apoiar atividades de investigação científica, tecnologia, educação ambiental e análise de dados.

```text
┌─────────────────────────────────────┐
│        MONITOR DE SENSORES          │
│─────────────────────────────────────│
│ Temp. Ar:   24.5 C                  │
│ Umid. Ar:   68.0 %                  │
│ Umid. Solo: 42 %                    │
│ Luminos.:   312 lx                  │
│ MQTT: ONLINE                        │
└─────────────────────────────────────┘
```

---

## ✨ Funcionalidades

- 🌡️ **Temperatura do ar** por sensor DHT11
- 💧 **Umidade relativa do ar** por sensor DHT11
- 🌱 **Umidade do solo** por sensor capacitivo com calibração ajustável
- ☀️ **Luminosidade** por módulo GY-30 / BH1750
- 🖥️ **Display OLED 128×64** com leituras e status da conexão MQTT
- 📶 **Conexão Wi-Fi** pelo Arduino UNO R4 WiFi
- 📡 **Publicação MQTT** em broker HiveMQ público
- 🧾 **Payload JSON** pronto para Node-RED, n8n, Python, bancos de dados e dashboards
- 🔁 **Reconexão automática** ao Wi-Fi e ao broker MQTT
- 🩺 **Tratamento de leituras inválidas**, publicadas como `null` em vez de valores falsos
- 🖥️ **Log serial** para configuração, diagnóstico e depuração

---

## 🧩 Arquitetura de Comunicação

```text
DHT11 ───────────────┐
BH1750 ──────────────┤
Sensor de solo ──────┤
                     ▼
             Arduino UNO R4 WiFi
                ├── Display OLED
                ├── Monitor Serial
                └── Wi-Fi → MQTT → HiveMQ
                                   └── Node-RED / n8n / Python / Dashboard
```

O firmware publica todas as grandezas em uma única mensagem JSON. Na configuração padrão, o tópico é:

```text
rede-ciencia-cidada/santo-andre/estacao-001/dados
```

Exemplo de mensagem:

```json
{
  "temperatura_ar_c": 24.7,
  "umidade_ar_pct": 61.3,
  "umidade_solo_pct": 52,
  "umidade_solo_raw": 702,
  "luminosidade_lux": 1384.0
}
```

Quando uma leitura do DHT11 ou BH1750 é inválida, o campo correspondente é enviado como `null`. Isso evita registrar uma falha do sensor como uma medição ambiental real.

> [!WARNING]
> O broker público da HiveMQ é compartilhado e não deve receber senhas, informações pessoais ou dados sensíveis. Para uso permanente, institucional ou em produção, utilize autenticação, TLS e um broker privado.

---

## 🛒 Hardware Necessário

| Componente | Quantidade | Função |
|---|---:|---|
| Arduino UNO R4 WiFi | 1 | Processamento e conectividade Wi-Fi |
| Sensor DHT11 | 1 | Temperatura e umidade do ar |
| Módulo GY-30 / BH1750 | 1 | Luminosidade em lux |
| Display OLED 128×64 / SSD1306 | 1 | Visualização local |
| Sensor capacitivo de umidade do solo | 1 | Umidade relativa do solo |
| Jumpers e protoboard | Conforme necessário | Montagem do circuito |

> A versão atual com MQTT depende do **UNO R4 WiFi**. O UNO R4 Minima não possui conectividade Wi-Fi integrada e exige um módulo ou gateway externo.

---

## ⚡ Início Rápido

### 1. Clone o repositório

```bash
git clone https://github.com/seu-usuario/curumaker.git
cd curumaker
```

### 2. Instale as bibliotecas

Na Arduino IDE, abra **Sketch → Incluir Biblioteca → Gerenciar Bibliotecas** e instale:

- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `DHT sensor library` by Adafruit
- `Adafruit Unified Sensor`
- `BH1750` by Christopher Laws
- `PubSubClient` by Nick O'Leary

As bibliotecas `Wire` e `WiFiS3` são fornecidas pelo pacote da placa Arduino UNO R4 WiFi.

### 3. Monte o circuito

```text
Arduino D2   ─── DHT11 DATA
Arduino A4   ─── SDA do OLED e do GY-30
Arduino A5   ─── SCL do OLED e do GY-30
Arduino A0   ─── AOUT do sensor de solo
```

Todos os módulos devem compartilhar o mesmo GND.

### 4. Configure o Wi-Fi

Edite estas constantes no firmware:

```cpp
const char* WIFI_SSID = "NOME_DO_WIFI";
const char* WIFI_PASSWORD = "SENHA_DO_WIFI";
```

### 5. Configure o MQTT

A configuração padrão usa o broker público da HiveMQ:

```cpp
const char* MQTT_BROKER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

const char* MQTT_TOPIC =
  "rede-ciencia-cidada/santo-andre/estacao-001/dados";

const char* MQTT_CLIENT_ID =
  "estacao-maker-santo-andre-001";
```

O `MQTT_CLIENT_ID` deve ser único. Caso duas placas usem o mesmo identificador, uma conexão poderá derrubar a outra.

Para várias estações, altere o identificador numérico:

```text
rede-ciencia-cidada/santo-andre/estacao-001/dados
rede-ciencia-cidada/santo-andre/estacao-002/dados
rede-ciencia-cidada/santo-andre/estacao-003/dados
```

### 6. Calibre o sensor de solo

Edite as constantes de acordo com as leituras do seu sensor:

```cpp
const int VALOR_SECO = 1020;
const int VALOR_UMIDO = 410;
```

- `VALOR_SECO`: leitura do sensor no ar ou em uma referência seca;
- `VALOR_UMIDO`: leitura no limite úmido adotado para o projeto.

Consulte o [Guia do Colaborador](DOCUMENTACAO_CuruMaker.md) para o procedimento completo.

### 7. Defina o intervalo de leitura e publicação

O ciclo de leitura, atualização do OLED e publicação MQTT é controlado por:

```cpp
const unsigned long INTERVALO_LEITURA = 2000;
```

O valor está em milissegundos:

```cpp
const unsigned long INTERVALO_LEITURA = 5000;   // 5 segundos
const unsigned long INTERVALO_LEITURA = 30000;  // 30 segundos
const unsigned long INTERVALO_LEITURA = 60000;  // 1 minuto
```

As constantes abaixo controlam somente as tentativas de reconexão:

```cpp
const unsigned long INTERVALO_WIFI = 10000;
const unsigned long INTERVALO_MQTT = 5000;
```

### 8. Faça o upload

Selecione **Arduino UNO R4 WiFi**, escolha a porta correta e envie o firmware. Abra o Monitor Serial em `9600 baud` para acompanhar o endereço IP, as conexões e as mensagens publicadas.

---

## 🔎 Testando a Publicação

Use um cliente MQTT, como MQTT Explorer, MQTTX, Node-RED ou um script Python, e conecte-se a:

```text
Broker: broker.hivemq.com
Porta: 1883
Tópico: rede-ciencia-cidada/santo-andre/estacao-001/dados
```

Para observar várias estações simultaneamente, assine:

```text
rede-ciencia-cidada/santo-andre/+/dados
```

---

## 📁 Estrutura do Repositório

```text
curumaker/
├── Arduino_R4_test.ino          # Firmware principal
├── README.md                    # Apresentação e início rápido
├── DOCUMENTACAO_CuruMaker.md    # Guia técnico e de colaboração
└── LICENSE                      # Licença MIT
```

---

## 🗺️ Roadmap

- [x] Leitura de temperatura e umidade do ar
- [x] Leitura de luminosidade
- [x] Leitura de umidade do solo
- [x] Exibição em display OLED
- [x] Conexão Wi-Fi
- [x] Publicação MQTT em JSON
- [x] Reconexão automática ao Wi-Fi e MQTT
- [x] Indicador MQTT no display
- [x] Tratamento de falhas com `null`
- [ ] Credenciais em arquivo separado `arduino_secrets.h`
- [ ] Broker privado com autenticação e TLS
- [ ] Identificador persistente e único por estação
- [ ] Timestamp com RTC ou sincronização NTP
- [ ] Buffer local para períodos sem internet
- [ ] Log em cartão SD
- [ ] Dashboard web para visualização histórica
- [ ] Suporte configurável ao DHT22
- [ ] Publicação separada de estado e disponibilidade

---

## 🤝 Como Contribuir

Contribuições são bem-vindas, especialmente nas áreas de instrumentação, conectividade, análise de dados e aplicação pedagógica.

1. Faça um fork do repositório.
2. Crie uma branch: `git checkout -b feature/minha-melhoria`.
3. Faça as alterações e testes.
4. Registre um commit claro: `git commit -m "Add: descrição da melhoria"`.
5. Envie a branch: `git push origin feature/minha-melhoria`.
6. Abra um Pull Request descrevendo o hardware utilizado e o procedimento de teste.

Antes de contribuir, leia o [Guia do Colaborador](DOCUMENTACAO_CuruMaker.md).

---

## 👤 Autor

**Matheus Valadares Teixeira**

Engenheiro Mecânico com formação complementar em Automação e Controle, Robótica Educacional e Educação para o Ensino Fundamental II e Médio. Atua na integração entre engenharia, ciência, tecnologia e práticas educacionais em contextos formais e não formais.

Parque Tecnológico de Santo André · Rede Ciência Cidadã

---

## 📄 Licença

Distribuído sob a licença MIT. Consulte o arquivo `LICENSE` para mais informações.

---

<div align="center">
  Feito com ❤️ para a comunidade maker de Santo André
</div>
