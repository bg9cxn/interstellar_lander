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

// LED
unsigned long lastBlink = 0;
static char counter[100]; // 移到外面，或者加 static

void setup() {
  Serial.begin(9600);
  Serial.println("\nStarted!");
  Wire.begin(I2C_SDA, I2C_SCL);
  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setRotation(1);
  display.drawBitmap(0, 0, epd_bitmap_oled_1, 32, 128, SSD1306_WHITE);
  display.display();
  // 停留 5 秒让你看到图片
  delay(5000); 

  display.clearDisplay();
  display.drawBitmap(0, 0, epd_bitmap_oled_2, 32, 128, SSD1306_WHITE);
  display.display();
  // 停留 5 秒让你看到图片
  delay(5000); 

  display.clearDisplay();
  display.drawBitmap(0, 0, epd_bitmap_oled_3, 32, 128, SSD1306_WHITE);
  display.display();
  // 停留 5 秒让你看到图片
  delay(5000); 

  display.clearDisplay();
  display.drawBitmap(0, 0, epd_bitmap_oled_4, 32, 128, SSD1306_WHITE);
  display.display();
  // 停留 5 秒让你看到图片
  delay(5000); 

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  


  // SHT31 init
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  // NTP init
  timeClient.begin();
  timeClient.update();

  // LED init
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // timeClient.update();
  // 在 loop() 开始处
  static int successCount = 0;
  static int failedCount = 0;
  static unsigned long lastNTPUpdate = 0;
  if (millis() - lastNTPUpdate > 1800000) { // 每30分钟更新一次时间
    bool updateStatus = timeClient.update();
    if (updateStatus){
      successCount += 1;
      sprintf(counter, "successCount = %d",successCount);
      Serial.println(counter);
    }
    else{
      failedCount += 1;
      sprintf(counter, "failedCount = %d",failedCount);
      Serial.println(counter);
    }
    lastNTPUpdate = millis();
  }


  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
  

  // AM/PM
  bool isPM = false;
  if (hours >= 12) {
    isPM = true;
    if (hours > 12) hours -= 12;
  }
  if (hours == 0) hours = 12;

  // 读取温湿度数据
  float tempC = sht31.readTemperature();
  float hum   = sht31.readHumidity();

  // 格式化时间
  char hStr[3], mStr[3], sStr[3];
  sprintf(hStr, "%02d", hours);
  sprintf(mStr, "%02d", minutes);
  sprintf(sStr, "%02d", seconds);
  // char buffer[50]; // 确保这个数组足够容纳拼接后的字符串
  // sprintf(buffer, "time is %s:%s:%s", hStr, mStr, sStr);
  // Serial.println(buffer);


  // ===== OLED =====
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
  display.setTextSize(2);
  display.setCursor(5, 30);
  display.println(hStr);

  // mm
  display.setTextSize(2);
  display.setCursor(5, 52);
  display.println(mStr);

  // ss
  display.setTextSize(2);
  display.setCursor(5, 72);
  display.println(sStr);
  

  // --------
  display.drawLine(0, 95, 32, 95, SSD1306_WHITE);

  // (°C) 温度及湿度显示
  display.setTextSize(1);
  display.setCursor(5, 105);
  display.print((int)tempC);
  display.print((char)247); // 摄氏度符号
  display.print("C");
  // display.print(successCount);
  display.setCursor(5, 120);
  display.print((int)hum);
  display.print("%");
  // display.print(failedCount);

  display.display();

  //===== LED =====
  unsigned long now = millis();
  if (now - lastBlink >= 10000) {  // 每10秒点亮LED一次
    digitalWrite(LED_PIN, HIGH);
    delay(20);                    // 点亮20ms
    digitalWrite(LED_PIN, LOW);
    lastBlink = now;
  }

}
