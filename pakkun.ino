#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==========================================
// 1. TEMPLATE AKSES (ISI DI SINI)
// ==========================================
const char* ssid = "BATANG PANE";       // Nama WiFi
const char* password = "manggaraya1"; // Password WiFi

#define BOTtoken "8716471473:AAFE9F89dahVUrEzOlamngGKHbAvjYtsxN0"  // Contoh: "123456789:ABCDefgh..."
#define CHAT_ID "6447786515"               // Contoh: "987654321"

// ==========================================
// 2. KONFIGURASI PIN & OBJEK
// ==========================================
// OLED (GND, VDD, SCK, SDA)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PZEM (TX -> 16, RX -> 17)
PZEM004Tv30 pzem(Serial2, 16, 17);

// Telegram
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

unsigned long lastTimeBotRan;
int botRequestDelay = 1000; // Cek pesan tiap 1 detik

// ==========================================
// 3. FUNGSI TELEGRAM
// ==========================================
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Akses Ditolak!", "");
      continue;
    }

    String text = bot.messages[i].text;
    if (text == "/start") {
      String welcome = "Halo Dimas! Gunakan perintah berikut:\n";
      welcome += "/cek : Lihat status listrik real-time\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/cek") {
      float voltage = pzem.voltage();
      float power = pzem.power();
      float energy = pzem.energy();

      String laporan = "📊 *STATUS LISTRIK*\n";
      laporan += "Tegangan: " + String(voltage, 1) + " V\n";
      laporan += "Daya: " + String(power, 1) + " Watt\n";
      laporan += "Total: " + String(energy, 3) + " kWh\n";
      bot.sendMessage(chat_id, laporan, "Markdown");
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi OLED (SCK=22, SDA=21)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED Gagal!"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,20);
  display.println("Connecting WiFi...");
  display.display();

  // Koneksi WiFi
  WiFi.begin(ssid, password);
  client.setInsecure(); // Wajib untuk Telegram di ESP32

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  display.clearDisplay();
  display.setCursor(0,20);
  display.println("WiFi Ready!");
  display.println("Bot Active!");
  display.display();
  delay(2000);
}

void loop() {
  // Update Data PZEM
  float v = pzem.voltage();
  float p = pzem.power();

  // Update Tampilan OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("V: "); display.print(isnan(v) ? 0 : v, 1); display.println(" Volt");
  display.print("P: "); display.print(isnan(p) ? 0 : p, 1); display.println(" Watt");
  display.setCursor(0,40);
  display.print("WiFi: "); display.print(WiFi.SSID());
  display.display();

  // Cek Pesan Telegram
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}