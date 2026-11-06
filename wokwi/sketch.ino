/*
 * CENTRAL DE SEGURANÇA IoT - Detecção de intrusos
 * ESP32 + PIR + LEDs + Buzzer + Botão + Telegram (Wokwi)
 */
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// ====== CONFIGURAÇÃO (troque pelos seus dados) ======
#define BOT_TOKEN "SEU_TOKEN_AQUI"
#define CHAT_ID   "SEU_CHAT_ID_AQUI"
// Firebase Realtime Database (sem https:// e sem barra no final)
#define FIREBASE_HOST "SEU-PROJETO-default-rtdb.firebaseio.com"
#define LOCAL     "Sala de Equipamentos"

// ====== PINOS ======
#define PIN_PIR     27
#define PIN_LED_G   26
#define PIN_LED_R   25
#define PIN_BUZZER  33
#define PIN_BOTAO   14

// ====== TEMPOS (ms) ======
const unsigned long INTERVALO_NOTIFICACAO = 10000; // anti-spam: 10 s
const unsigned long DEBOUNCE_BOTAO        = 200;
const unsigned long DURACAO_ALARME        = 5000;  // buzzer toca 5 s por evento

WiFiClientSecure client;
WiFiClientSecure fbClient;
UniversalTelegramBot bot(BOT_TOKEN, client);

bool armado = false;
bool alarmeAtivo = false;
bool telegramOk = false;
int  totalOcorrencias = 0;

unsigned long ultimoEnvio = 0;
bool jaEnviou = false;
unsigned long ultimoBotao = 0;
unsigned long inicioAlarme = 0;
int ultimoEstadoBotao = HIGH;

// ---------- Monitor Serial ----------
void imprimirStatus(const char* pir) {
  Serial.println("================================");
  Serial.println("CENTRAL DE SEGURANÇA IoT");
  Serial.println("================================");
  Serial.print("Wi-Fi: ");   Serial.println(WiFi.status() == WL_CONNECTED ? "CONECTADO" : "DESCONECTADO");
  Serial.print("Telegram: "); Serial.println(telegramOk ? "CONECTADO" : "FALHA");
  Serial.print("Sistema: ");  Serial.println(armado ? "ARMADO" : "DESARMADO");
  Serial.print("PIR: ");      Serial.println(pir);
  Serial.print("LED: ");      Serial.println(armado ? "VERMELHO" : "VERDE");
  Serial.print("Buzzer: ");   Serial.println(alarmeAtivo ? "LIGADO" : "DESLIGADO");
  Serial.println("--------------------------------");
}

// ---------- Telegram ----------
bool enviarTelegram(const String& msg) {
  bool ok = bot.sendMessage(CHAT_ID, msg, "");
  Serial.println(ok ? "Telegram: MENSAGEM ENVIADA" : "Telegram: FALHA NO ENVIO");
  return ok;
}

// ---------- Firebase ----------
// Grava um evento novo em /eventos e atualiza /status
void enviarFirebase(const char* tipo, const char* notificacao) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;

  String evento = "{\"tipo\":\"" + String(tipo) + "\","
                  "\"estado\":\"" + String(armado ? "ARMADO" : "DESARMADO") + "\","
                  "\"sensor\":\"PIR\","
                  "\"notificacao\":\"" + String(notificacao) + "\","
                  "\"ocorrencia\":" + String(totalOcorrencias) + ","
                  "\"ts\":{\".sv\":\"timestamp\"}}";
  http.begin(fbClient, String("https://") + FIREBASE_HOST + "/eventos.json");
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(evento);
  if (code == 200) Serial.println("Firebase: EVENTO GRAVADO");
  else { Serial.print("Firebase: FALHA ("); Serial.print(code); Serial.println(")"); }
  http.end();

  String status = "{\"estado\":\"" + String(armado ? "ARMADO" : "DESARMADO") + "\","
                  "\"ocorrencias\":" + String(totalOcorrencias) + ","
                  "\"ts\":{\".sv\":\"timestamp\"}}";
  http.begin(fbClient, String("https://") + FIREBASE_HOST + "/status.json");
  http.addHeader("Content-Type", "application/json");
  http.PUT(status);
  http.end();
}

void atualizarLeds() {
  digitalWrite(PIN_LED_G, armado ? LOW : HIGH);
  digitalWrite(PIN_LED_R, armado ? HIGH : LOW);
}

void alternarSistema() {
  armado = !armado;
  alarmeAtivo = false;
  noTone(PIN_BUZZER);
  jaEnviou = false;
  atualizarLeds();
  imprimirStatus("SEM MOVIMENTO");

  if (armado) {
    enviarTelegram("🔒 SISTEMA ARMADO\nA central de segurança foi ativada.\nMonitoramento iniciado.");
    enviarFirebase("Sistema armado", "Enviada");
  } else {
    enviarTelegram("🔓 SISTEMA DESARMADO\nA central de segurança foi desativada.");
    enviarFirebase("Sistema desarmado", "Enviada");
  }
}

void tratarIntrusao() {
  totalOcorrencias++;
  alarmeAtivo = true;
  inicioAlarme = millis();

  Serial.println("--------------------------------");
  Serial.println("MOVIMENTO DETECTADO!");
  Serial.println("--------------------------------");
  Serial.println("Sistema: ARMADO");
  Serial.println("LED vermelho: LIGADO");
  Serial.println("Buzzer: LIGADO");
  Serial.print("Ocorrência nº "); Serial.print(totalOcorrencias);
  Serial.print(" | tempo ligado: "); Serial.print(millis() / 1000); Serial.println(" s");

  // Controle de notificações (anti-spam)
  if (!jaEnviou || millis() - ultimoEnvio >= INTERVALO_NOTIFICACAO) {
    Serial.println("Enviando notificação...");
    String msg = "🚨 ALERTA DE INTRUSÃO 🚨\n";
    msg += "Sistema: CENTRAL DE SEGURANÇA\n";
    msg += "Local: " LOCAL "\n";
    msg += "Movimento detectado pelo sensor PIR.\n";
    msg += "Estado: ARMADO\n";
    msg += "Ação: ALARME ATIVADO\n";
    msg += "Ocorrência nº: " + String(totalOcorrencias) + "\n";
    msg += "Tempo de funcionamento: " + String(millis() / 1000) + " s";
    bool ok = enviarTelegram(msg);
    ultimoEnvio = millis();
    jaEnviou = true;
    enviarFirebase("Movimento detectado", ok ? "Enviada" : "Falha");
  } else {
    unsigned long resta = (INTERVALO_NOTIFICACAO - (millis() - ultimoEnvio)) / 1000;
    Serial.print("Notificação bloqueada (aguarde ~"); Serial.print(resta); Serial.println(" s)");
    enviarFirebase("Movimento detectado", "Bloqueada");
  }
}

// Padrão sonoro: BEEP-BEEP-BEEP (não bloqueante)
void atualizarBuzzer() {
  if (!alarmeAtivo) { noTone(PIN_BUZZER); return; }

  if (millis() - inicioAlarme >= DURACAO_ALARME) {
    alarmeAtivo = false;
    noTone(PIN_BUZZER);
    Serial.println("Buzzer: DESLIGADO");
    return;
  }
  // 200 ms ligado / 200 ms desligado
  if (((millis() - inicioAlarme) / 200) % 2 == 0) tone(PIN_BUZZER, 1000);
  else noTone(PIN_BUZZER);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  atualizarLeds();

  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.println();

  client.setInsecure();   // necessário para o Telegram no Wokwi
  fbClient.setInsecure(); // idem para o Firebase
  telegramOk = bot.getMe();

  imprimirStatus("SEM MOVIMENTO");
}

void loop() {
  // --- Botão: alterna ARMADO/DESARMADO (Opção A) ---
  int estado = digitalRead(PIN_BOTAO);
  if (estado == LOW && ultimoEstadoBotao == HIGH && millis() - ultimoBotao > DEBOUNCE_BOTAO) {
    ultimoBotao = millis();
    alternarSistema();
  }
  ultimoEstadoBotao = estado;

  // --- PIR: só age se estiver ARMADO ---
  static bool pirAnterior = false;
  bool pir = digitalRead(PIN_PIR);
  if (armado && pir && !pirAnterior) {   // borda de subida = novo evento
    tratarIntrusao();
  }
  pirAnterior = pir;

  atualizarBuzzer();
  delay(20);
}
