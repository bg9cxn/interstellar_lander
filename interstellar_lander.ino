#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// ===== WiFi info =====
const char* ssid     = "";//<-----------your WiFi name @@@@@@@@@@@@@@@@@@@@@@
const char* password = "";//<-----------your WiFi password @@@@@@@@@@@@@@@@@@

// ===== OLED setup =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// @@@@@@@@@@@@@@@@@@@@@32x128px  oled banner paste here------>

// <------32x128px  oled banner paste here@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ===== SHT31 setup =====
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// ===== NTP setup =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 8 * 3600, 60000); // GMT+8

// ===== Pins  =====
#define I2C_SDA 8
#define I2C_SCL 9
#define LED_PIN 0

// ===== RTC Data =====
RTC_DATA_ATTR int successCount = 0;
RTC_DATA_ATTR int failedCount = 0;
RTC_DATA_ATTR unsigned long lastNTPUpdate = 0;
RTC_DATA_ATTR unsigned long totalRuntime = 0;
// 【新增】用于在深度睡眠期间保存当前时间（UTC时间戳）
RTC_DATA_ATTR unsigned long savedEpoch = 0; 

// 全局变量
const unsigned long UPDATE_INTERVAL = 1800000;      // 30分钟 (毫秒)
const unsigned long SLEEP_INTERVAL_US = 10000000;   // 10秒唤醒 (微秒，用于esp_sleep)
unsigned long loopStartMillis = 0;
bool logoShown = false; 

// 辅助函数：连接WiFi
void connectWiFi() {
  Serial.println("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // 最多等待5秒，超时则跳过（避免耗尽电池）
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 80) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
  } else {
    Serial.println("\nWiFi connect failed!");
  }
}

// 辅助函数：关闭WiFi
void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi closed.");
}

void setup() {
  Serial.begin(9600);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(LED_PIN, OUTPUT);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  
  esp_reset_reason_t reset_reason = esp_reset_reason();
  display.setRotation(1);
  
  if (reset_reason == ESP_RST_POWERON) {
    Serial.println("Power On detected, showing logos...");
    display.clearDisplay();
    display.drawBitmap(0, 0, epd_bitmap_oled_1, 32, 128, SSD1306_WHITE);
    display.display();
    delay(3000); 
    display.clearDisplay();
    display.drawBitmap(0, 0, epd_bitmap_oled_2, 32, 128, SSD1306_WHITE);
    display.display();
    delay(3000);
    display.clearDisplay();
    display.drawBitmap(0, 0, epd_bitmap_oled_3, 32, 128, SSD1306_WHITE);
    display.display();
    delay(3000);
    display.clearDisplay();
    display.drawBitmap(0, 0, epd_bitmap_oled_4, 32, 128, SSD1306_WHITE);
    display.display();
    delay(3000);
    display.clearDisplay(); 
  }

  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
  }

  // 1. 时间累加 (唤醒一次加10秒)
  savedEpoch += (SLEEP_INTERVAL_US / 1000000); 
  lastNTPUpdate += (SLEEP_INTERVAL_US / 1000); 

  // 2. 检查是否需要更新 NTP
  bool needTimeUpdate = false;
  
  if (reset_reason == ESP_RST_POWERON) {
    needTimeUpdate = true;
    lastNTPUpdate = 0;
  } 
  else if (lastNTPUpdate >= UPDATE_INTERVAL) {
    needTimeUpdate = true;
    lastNTPUpdate = 0;
  }

  if (needTimeUpdate) {
    connectWiFi();
    bool updateStatus = timeClient.update();
    
    if (updateStatus) {
      successCount++;
      Serial.print("NTP Success, count: ");
      Serial.println(successCount);
      
      // 【修复】获取NTP时间并保存到 savedEpoch
      // getEpochTime() 返回的是带时区偏移的时间，我们要减去偏移量得到UTC时间保存
      // 这样 savedEpoch 存的就是标准的 UTC 时间戳
      savedEpoch = timeClient.getEpochTime() - (8 * 3600);
      
    } else {
      failedCount++;
      Serial.print("NTP Failed, count: ");
      Serial.println(failedCount);
      lastNTPUpdate = 0; 
    }
    disconnectWiFi();
  }
  
  // 这里不再调用 setEpoch，因为我们将在 loop 里自己算时间
}

void loop() {
  loopStartMillis = millis();

  // ===== 【核心修复】自己计算时间 =====
  // savedEpoch 是 UTC 时间戳，加上 8 小时时区偏移量得到本地时间
  unsigned long localEpoch = savedEpoch + (8 * 3600);

  // 计算时分秒
  int seconds = localEpoch % 60;
  int minutes = (localEpoch / 60) % 60;
  int hours = (localEpoch / 3600) % 24;

  // AM/PM 处理
  bool isPM = false;
  if (hours >= 12) {
    isPM = true;
    if (hours > 12) hours -= 12;
  }
  if (hours == 0) hours = 12;
  // ================================

  // 读取温湿度
  float tempC = sht31.readTemperature();
  float hum   = sht31.readHumidity();

  // 格式化字符串
  char hStr[3], mStr[3], sStr[3];
  sprintf(hStr, "%02d", hours);
  sprintf(mStr, "%02d", minutes);
  sprintf(sStr, "%02d", seconds);

  // ===== 刷新 OLED =====
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setTextSize(0.5);
  display.setCursor(2, 0);
  display.println("/////");
  display.setCursor(60, 10);
  display.println(isPM ? "PM" : "AM");
  display.drawLine(0, 10, 32, 10, SSD1306_WHITE);

  // hh
  display.setTextSize(2.7);
  display.setCursor(4, 40);
  display.println(hStr);

  // mm
  display.setTextSize(2.7);
  display.setCursor(4, 68);
  display.println(mStr);

  // ss
  // display.setTextSize(2);
  // display.setCursor(5, 72);
  // display.println(sStr);
  
  // --------
  display.drawLine(0, 95, 32, 95, SSD1306_WHITE);

  // 温度及湿度
  display.setTextSize(1);
  display.setCursor(5, 105);
  display.print((int)tempC);
  display.print((char)247); // °
  display.print("C");
  
  display.setCursor(5, 120);
  display.print((int)hum);
  display.print("%");

  display.display();

  // ===== LED 闪烁 =====
  digitalWrite(LED_PIN, HIGH);
  delay(20);
  digitalWrite(LED_PIN, LOW);

  // ===== 准备进入深度睡眠 =====
  
  // 1. 累计运行时间
  totalRuntime += (millis() - loopStartMillis);

  // 2. 设置唤醒源并睡眠
  Serial.println("Going to sleep...");
  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_US); 
  esp_deep_sleep_start();
}
