/*
 * ESP32 com Sensor PIR e LED para AWS IoT Core
 * ------------------------------------------------
 * OBJETIVO: Versão de protótipo com LED.
 *
 * DETECÇÃO (Caminho 1):
 * 1. Detecta movimento com o Sensor PIR.
 * 2. Aciona o LED localmente (lógica Active HIGH).
 * 3. Envia uma mensagem {"movimento": true} para o tópico da AWS.
 *
 * COMANDO (Caminho 2):
 * 1. Ouve (subscribe) um tópico de comandos da AWS.
 * 2. Ao receber {"comando": "desativar_alarme"}, desliga o LED.
 * 3. O alarme só é rearmado após 10s *sem* movimento.
 */

// Bibliotecas de Wi-Fi e Segurança
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Biblioteca do Sistema de Ficheiros (para ler as "keys")
#include <LittleFS.h>

// Biblioteca MQTT
#include <PubSubClient.h>

// Biblioteca JSON
#include <ArduinoJson.h>

// Biblioteca de Hora
#include <time.h>

// --- 1. CONFIGURAÇÕES DE REDE E AWS ---
const char* ssid = "Mendes"; // Seu Wi-Fi SSID
const char* password = "135792468"; // Sua Senha Wi-Fi

// O seu endpoint da AWS
const char* awsEndpoint = "agbkdvwra4l3y-ats.iot.us-east-1.amazonaws.com";

// Nomes EXATOS dos ficheiros como estão na pasta 'data'
const char* awsCert = "/python_iot-certificate.pem.crt";
const char* awsKey = "/python_iot-private.pem.key";
const char* awsRootCA = "/AmazonRootCA1.pem";

// --- 2. CONFIGURAÇÕES DO PROJETO ---
#define PIR_PIN 27    // Pino de DADOS (OUT) do sensor PIR (GPIO27)
#define LED_PIN 26    // Pino para o LED (GPIO26)
#define BUZZ_PIN 25

const char* thingName = "meu-esp32-alarme";       // Nome único do objeto
const char* publishTopic = "esp32/movimento";   // Tópico para ENVIAR dados
const char* subscribeTopic = "esp32/comandos";  // Tópico para RECEBER comandos

// --- 3. VARIÁVEIS DE ESTADO ---
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

bool alarmeAtivo = false;      // O alarme está tocando?
bool alarmeSilenciado = false; // O usuário silenciou o alarme?
unsigned long ultimoMovimento = 0; // Última vez que o PIR detectou movimento
const long tempoReset = 500; // 10 segundos (10.000ms) sem movimento para rearmar

// --- Funções de Conexão (Wi-Fi, Certificados, Hora, AWS) ---

void connectWiFi() {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

bool loadCertificates() {
  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao montar LittleFS");
    return false;
  }
  Serial.println("Sistema de ficheiros LittleFS montado.");

  File rootCAFile = LittleFS.open(awsRootCA, "r");
  if (!rootCAFile) { Serial.println("Falha ao abrir o ficheiro Root CA."); return false; }
  if (!net.loadCACert(rootCAFile, rootCAFile.size())) { Serial.println("Erro ao carregar o certificado CA Raiz"); rootCAFile.close(); return false; }
  rootCAFile.close();
  Serial.println("Certificado CA Raiz carregado.");

  File certFile = LittleFS.open(awsCert, "r");
  if (!certFile) { Serial.println("Falha ao abrir o ficheiro de certificado."); return false; }
  if (!net.loadCertificate(certFile, certFile.size())) { Serial.println("Erro ao carregar o certificado do dispositivo."); certFile.close(); return false; }
  certFile.close();
  Serial.println("Certificado do dispositivo carregado.");

  File keyFile = LittleFS.open(awsKey, "r");
  if (!keyFile) { Serial.println("Falha ao abrir o ficheiro de chave privada."); return false; }
  if (!net.loadPrivateKey(keyFile, keyFile.size())) { Serial.println("Erro ao carregar a chave privada."); keyFile.close(); return false; }
  keyFile.close();
  Serial.println("Chave privada carregada.");

  return true;
}

// --- FUNÇÃO DE CALLBACK (CAMINHO REVERSO) ---
void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.println("]: ");

  // Desserializa o JSON recebido
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload, length);

  // Extrai o comando
  const char* comando = doc["comando"]; // Ex: "desativar_alarme"

  if (comando != NULL && strcmp(comando, "desativar_alarme") == 0) {
    Serial.println("Comando recebido: Desativar Alarme.");
    
    // --- MUDANÇA 1: Desliga o LED (enviando LOW) ---
    digitalWrite(LED_PIN, LOW); // DESLIGA o LED (Active HIGH)
    digitalWrite(BUZZ_PIN, LOW); // LIGA o LED (Active HIGH)
    
    alarmeSilenciado = true;     // Marca que foi silenciado pelo usuário
    alarmeAtivo = false;         // O alarme não está mais "ativo" (tocando)
  }
}

void connectAWS() {
  // Configurar hora via NTP
  Serial.println("Configurando hora via NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nHora configurada.");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Hora atual UTC: ");
  Serial.print(asctime(&timeinfo));

  while (!client.connected()) {
    Serial.print("Tentando conectar ao AWS IoT...");
    if (client.connect(thingName)) {
      Serial.println(" Conectado!");
      
      // --- IMPORTANTE: Inscreve-se no tópico de comandos ---
      client.subscribe(subscribeTopic);
      Serial.print("Inscrito no tópico: ");
      Serial.println(subscribeTopic);
      
    } else {
      Serial.print(" falhou, rc=");
      Serial.print(client.state());
      Serial.println(". Tentando novamente em 5 segundos");
      Serial.println("Verifique o endpoint, certificados e a hora do dispositivo.");
      delay(5000);
    }
  }
}

// --- FUNÇÃO PARA PUBLICAR DADOS (CAMINHO 1) ---
void publicarMovimento() {
  StaticJsonDocument<200> doc;
  doc["movimento"] = true;
  doc["timestamp"] = time(nullptr);

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Publica a mensagem
  if (client.publish(publishTopic, jsonBuffer)) {
    Serial.print("Publicado no tópico '");
    Serial.print(publishTopic);
    Serial.print("': ");
    Serial.println(jsonBuffer);
  } else {
    Serial.println("Falha ao publicar mensagem.");
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(9600);
  delay(1000);

  // Define os pinos dos sensores
  pinMode(PIR_PIN, INPUT);
  
  // --- MUDANÇA 2: Configura o pino do LED ---
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Garante que o LED começa DESLIGADO (Active HIGH)
  digitalWrite(BUZZ_PIN, LOW); // Garante que o LED começa DESLIGADO (Active HIGH)

  connectWiFi();

  if (!loadCertificates()) {
    Serial.println("Erro ao carregar certificados. Parando.");
    while (1);
  }

  client.setServer(awsEndpoint, 8883);
  client.setCallback(messageReceived); // Define a função de callback
}

// --- LOOP ---
void loop() {
  if (!client.connected()) {
    connectAWS();
  }
  client.loop(); // Mantém a conexão MQTT e verifica por mensagens recebidas

  // --- LÓGICA DO SENSOR DE MOVIMENTO ---
  int pirState = digitalRead(PIR_PIN);

  if (pirState == HIGH) {
    // 1. Movimento detectado
    ultimoMovimento = millis(); // Atualiza a última vez que viu movimento

    // 2. Se o alarme não estava tocando E não foi silenciado, acione-o
    if (!alarmeAtivo && !alarmeSilenciado) {
      Serial.println("Movimento Detectado! Acionando alarme (LED).");
      
      // --- MUDANÇA 3: Liga o LED (enviando HIGH) ---
      digitalWrite(LED_PIN, HIGH); // LIGA o LED (Active HIGH)
      digitalWrite(BUZZ_PIN, HIGH); // LIGA o LED (Active HIGH)
      
      alarmeAtivo = true;           // Marca que o alarme está tocando
      
      // 3. Publica para a AWS (apenas uma vez por evento)
      publicarMovimento();
    }
  } else {
    // 4. Sem movimento
    
    // 5. Lógica de Reset
    // Se o alarme foi silenciado pelo usuário E já passou o tempo de reset sem movimento
    if (alarmeSilenciado && (millis() - ultimoMovimento > tempoReset)) {
      Serial.println("Tempo de reset atingido. Re-armando o sistema.");
      alarmeSilenciado = false; // Rearma o sistema, pronto para a próxima detecção
    }
  }
}