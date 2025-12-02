// esp32_gs2_iot.ino

#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>

// ====== CONFIGURAÇÕES WiFi / NODE-RED ======
const char* WIFI_SSID     = "AndroidADFTK7";    // TODO: ajuste se precisar
const char* WIFI_PASSWORD = "estrela7839";      // TODO: ajuste se precisar

// Node-RED rodando na VM Linux na porta 1880
// DEIXEI O MESMO ENDPOINT QUE VOCÊ USOU ANTES
const char* NODE_RED_URL  = "http://192.168.43.82:1880/api/leituras-iot";

// Identificador único deste ESP32, igual ao campo IDENTIFICADOR_HW no banco
// (já existe no seu script: 'ESP32-ABC-001')
const char* DEVICE_ID     = "ESP32-ABC-001";

// Pino do botão (use o pino do botão "da direita" da sua placa)
// Se estiver usando DevKit comum, pode usar 0 (BOOT) ou outro que você preferir.
const int BUTTON_PIN = 0;  // TODO: ajuste se o seu botão estiver em outro pino



// ====== FUNÇÕES AUXILIARES ======

void conectaWiFi() {
  Serial.println();
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Falha ao conectar no WiFi :(");
  }
}

// Gera uma leitura "fake" parecida com as que você colocou no script SQL
// (acc_x/acc_y pequenos, acc_z ~ 1.0, gyro com valores baixos etc.)
void geraLeituraFake(
  float &acc_x, float &acc_y, float &acc_z,
  float &gyro_x, float &gyro_y, float &gyro_z,
  int &movimentado, int &choque,
  String &estado_ativo, String &observacao
) {
  // variação pequena de aceleração em X/Y
  acc_x = (float)random(-30, 31) / 1000.0;  // -0.0300 a 0.0300
  acc_y = (float)random(-30, 31) / 1000.0;  // -0.0300 a 0.0300
  // eixo Z próximo de 1g
  acc_z = 1.0 + (float)random(-10, 11) / 1000.0; // 0.9900 a 1.0100

  // giroscópio com pequenas variações
  gyro_x = (float)random(-20, 21) / 100.0;  // -0.20 a 0.20
  gyro_y = (float)random(-20, 21) / 100.0;  // -0.20 a 0.20
  gyro_z = (float)random(-20, 21) / 100.0;  // -0.20 a 0.20

  // probabilidade maior de "em uso" do que parado
  bool emUso  = random(0, 100) < 70;  // 70% de chance de estar sendo movimentado
  bool impacto = emUso && (random(0, 100) < 15); // se movimentado, 15% chance de choque

  movimentado = emUso ? 1 : 0;
  choque      = impacto ? 1 : 0;

  if (impacto) {
    estado_ativo = "MAN";  // por ex: manutenção ou alerta
    observacao   = "Possível queda ou impacto detectado pelo sensor fake.";
  } else if (emUso) {
    estado_ativo = "EMP";  // em emprestimo / uso
    observacao   = "Ativo em uso, gerado pelo ESP32 (leitura fake).";
  } else {
    estado_ativo = "ATV";  // ativo, parado
    observacao   = "Ativo estável e parado (leitura fake).";
  }
}

void enviaLeituraFake() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, tentando reconectar...");
    conectaWiFi();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Ainda sem WiFi, não vou enviar a leitura.");
    return;
  }

  // Gera valores fake
  float acc_x, acc_y, acc_z;
  float gyro_x, gyro_y, gyro_z;
  int movimentado, choque;
  String estado_ativo, observacao;

  geraLeituraFake(
    acc_x, acc_y, acc_z,
    gyro_x, gyro_y, gyro_z,
    movimentado, choque,
    estado_ativo, observacao
  );

  // Monta JSON de envio: precisa bater com o que o Node-RED espera
  // (identificador_hw, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z,
  //  movimentado, choque, estado_ativo, observacao)
  String json = "{";
  json += "\"identificador_hw\":\"" + String(DEVICE_ID) + "\",";
  json += "\"acc_x\":" + String(acc_x, 4) + ",";
  json += "\"acc_y\":" + String(acc_y, 4) + ",";
  json += "\"acc_z\":" + String(acc_z, 4) + ",";
  json += "\"gyro_x\":" + String(gyro_x, 4) + ",";
  json += "\"gyro_y\":" + String(gyro_y, 4) + ",";
  json += "\"gyro_z\":" + String(gyro_z, 4) + ",";
  json += "\"movimentado\":" + String(movimentado) + ",";
  json += "\"choque\":" + String(choque) + ",";
  json += "\"estado_ativo\":\"" + estado_ativo + "\",";
  json += "\"observacao\":\"" + observacao + "\"";
  json += "}";

  Serial.println("JSON enviado para Node-RED:");
  Serial.println(json);

  HTTPClient http;
  http.begin(NODE_RED_URL);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(json);

  Serial.print("HTTP status: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String resposta = http.getString();
    Serial.println("Resposta do Node-RED:");
    Serial.println(resposta);
  } else {
    Serial.println("Falha ao enviar POST para Node-RED.");
  }

  http.end();
}

// ====== SETUP / LOOP ======

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(0)); // para variar as leituras fake

  conectaWiFi();

  Serial.println("ESP32 pronto. Pressione o botão para enviar leituras IoT.");
}

void loop() {
  static bool ultimoEstadoBotao = HIGH;
  bool estadoAtual = digitalRead(BUTTON_PIN);

  // detecção de borda: HIGH -> LOW (botão apertado)
  if (ultimoEstadoBotao == HIGH && estadoAtual == LOW) {
    Serial.println("Botão pressionado! Enviando leitura fake para Node-RED...");
    enviaLeituraFake();
    delay(250); // debounce simples
  }

  ultimoEstadoBotao = estadoAtual;
  delay(10);
}
