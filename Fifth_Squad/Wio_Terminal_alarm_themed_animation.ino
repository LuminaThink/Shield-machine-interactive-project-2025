#include <TFT_eSPI.h>

TFT_eSPI tft;

void setup() {
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  pinMode(WIO_BUZZER, OUTPUT); // 正确引脚名称
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(30, 100);
  tft.println("ALARM TEST");
}

void playAlarmSound() {
  // 专业警报音效：短-短-长
  analogWrite(WIO_BUZZER, 128); delay(100);
  analogWrite(WIO_BUZZER, 0);   delay(50);
  
  analogWrite(WIO_BUZZER, 128); delay(100);
  analogWrite(WIO_BUZZER, 0);   delay(50);
  
  analogWrite(WIO_BUZZER, 200); delay(300); // 音量增强
  analogWrite(WIO_BUZZER, 0);   delay(200);
}

void loop() {
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(110, 80);
  tft.print("!");
  
  tft.setTextSize(2);
  tft.setCursor(40, 200);
  tft.println("DOOR OPEN!");
  
  playAlarmSound(); // 播放专业警报音
  
  delay(2000);
}