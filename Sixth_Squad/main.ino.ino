#include <TFT_eSPI.h>
#include <DHT.h>

// 引脚定义
#define DHTPIN 0       // DHT11连接到D0引脚
#define DHTTYPE DHT11
#define BUZZER_PIN WIO_BUZZER   // Wio Terminal蜂鸣器引脚

DHT dht(DHTPIN, DHTTYPE);
TFT_eSPI tft;

// 颜色定义
#define TEMP_HIGH_COLOR TFT_RED
#define HUMI_HIGH_COLOR TFT_YELLOW
#define TEMP_NORMAL_COLOR TFT_GREEN
#define TEMP_LOW_COLOR TFT_BLUE
#define TITLE_COLOR TFT_CYAN
#define TEXT_COLOR TFT_WHITE

// 报警状态
bool tempAlarm = false;
bool humiAlarm = false;
unsigned long lastBeepTime = 0;
unsigned long beepInterval = 1000;
bool beepState = false;

// 紧急报警音乐（更高音调，更急促）
char emergencyMelody[] = "CCCCaaaaGGGGeeee ";  // 高音调警报
int emergencyBeats[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int emergencyTempo = 200;  // 更快的节奏
int emergencyNoteIndex = 0;

// 音符频率定义（高音区，声音更大）
char noteNames[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
int noteFrequencies[] = { 
  1915, 1700, 1519, 1432, 1275, 1136, 1014,  // 中音区
  956, 851, 758, 716, 637, 568, 507          // 高音区（频率更低，音调更高）
};

void setup() {
    Serial.begin(115200);
    
    // 初始化蜂鸣器
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // 初始化屏幕
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    
    // 显示标题
    displayTitle();
    
    // 初始化DHT传感器
    dht.begin();
    
    // 播放测试音（更大声）
    playLoudTestTone();
    
    delay(2000);
}

void loop() {
    delay(2000);
    
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("读取DHT11失败！");
        displayError();
        stopAlarm();
        return;
    }
    
    Serial.print("温度: ");
    Serial.print(temperature, 1);
    Serial.print("°C, 湿度: ");
    Serial.print(humidity, 1);
    Serial.println("%");
    
    // 检查报警条件
    checkAlarmConditions(temperature, humidity);
    
    // 控制报警声音
    controlAlarm();
    
    // 显示数据
    displayOnScreen(temperature, humidity);
    displayAlarmStatus(temperature, humidity);
}

void displayTitle() {
    tft.setTextColor(TITLE_COLOR, TFT_BLACK);
    tft.setTextSize(3);
    tft.drawString("温湿度监测", 60, 10);
    
    tft.setTextSize(2);
    tft.setTextColor(TEXT_COLOR, TFT_BLACK);
    tft.drawString("紧急报警系统", 60, 50);
    
    // 显示状态说明
    tft.setTextSize(1);
    tft.setTextColor(TEMP_HIGH_COLOR, TFT_BLACK);
    tft.drawString(">40°C:红+警报", 10, 220);
    
    tft.setTextColor(HUMI_HIGH_COLOR, TFT_BLACK);
    tft.drawString(">90%:黄+警报", 110, 220);
    
    tft.setTextColor(TEMP_NORMAL_COLOR, TFT_BLACK);
    tft.drawString("音量:最大", 210, 220);
}

void checkAlarmConditions(float temp, float hum) {
    bool prevTempAlarm = tempAlarm;
    bool prevHumiAlarm = humiAlarm;
    
    tempAlarm = (temp > 40.0);
    humiAlarm = (hum > 90.0);
    
    if (tempAlarm && !prevTempAlarm) {
        Serial.println("⚠️⚠️⚠️ 高温紧急报警！温度超过40°C");
        Serial.println("播放紧急警报音...");
        emergencyNoteIndex = 0;
    }
    if (humiAlarm && !prevHumiAlarm) {
        Serial.println("⚠️⚠️⚠️ 高湿紧急报警！湿度超过90%");
        Serial.println("播放紧急警报音...");
        emergencyNoteIndex = 0;
    }
    
    if ((tempAlarm != prevTempAlarm) || (humiAlarm != prevHumiAlarm)) {
        playEmergencyAlert();
    }
}

void controlAlarm() {
    unsigned long currentTime = millis();
    
    if (tempAlarm || humiAlarm) {
        // 根据报警类型设置不同的播放间隔
        if (tempAlarm && humiAlarm) {
            beepInterval = 300;   // 双报警：极快速播放
        } else if (tempAlarm) {
            beepInterval = 500;   // 高温报警：快速播放
        } else if (humiAlarm) {
            beepInterval = 700;   // 高湿报警：中等速度
        }
        
        if (currentTime - lastBeepTime >= beepInterval) {
            // 播放紧急警报音
            playEmergencySound();
            lastBeepTime = currentTime;
            
            // 显示警报状态（闪烁）
            static bool alertBlink = false;
            alertBlink = !alertBlink;
            if (alertBlink) {
                tft.fillRect(280, 220, 30, 10, TFT_RED);
                tft.setTextColor(TFT_YELLOW, TFT_RED);
                tft.setTextSize(1);
                tft.setCursor(282, 221);
                tft.print("警");
            } else {
                tft.fillRect(280, 220, 30, 10, TFT_BLACK);
            }
        }
    } else {
        // 没有报警，停止声音
        digitalWrite(BUZZER_PIN, LOW);
        emergencyNoteIndex = 0;
        
        // 清除警报状态显示
        tft.fillRect(280, 220, 30, 10, TFT_BLACK);
    }
}

// ========== 紧急报警声音函数 ==========

void playLoudTestTone() {
    Serial.println("播放最大音量测试音...");
    // 播放高音调大音量测试音
    playLoudTone(800, 500);  // 高频率，长时间
    delay(300);
    playLoudTone(1000, 300); // 更高频率
    delay(200);
    playLoudTone(1200, 200); // 最高频率
    digitalWrite(BUZZER_PIN, LOW);
}

void playEmergencyAlert() {
    Serial.println("播放紧急警报提示音");
    // 三个急促的高音
    for (int i = 0; i < 3; i++) {
        playLoudTone(1500, 100); // 极高频率
        delay(80);
    }
    digitalWrite(BUZZER_PIN, LOW);
}

void playEmergencySound() {
    // 交替播放高低音，模拟警笛效果
    static bool highTone = true;
    
    if (highTone) {
        playLoudTone(1200, 150); // 高音
    } else {
        playLoudTone(800, 150);  // 低音
    }
    
    highTone = !highTone;
}

void playLoudTone(int frequency, int duration) {
    // 使用更高占空比让声音更大
    long period = 1000000L / frequency; // 周期（微秒）
    long halfPeriod = period / 2;
    long cycles = duration * 1000L / period;
    
    for (long i = 0; i < cycles; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

// 原始的音乐播放函数（保留但修改为更大声）
void playLoudNote(char note, int duration) {
    // 播放与音符名称对应的音调（更大声）
    for (int i = 0; i < 14; i++) {
        if (noteNames[i] == note) {
            // 使用更低频率值（对应更高音调）并增加音量
            int adjustedFreq = noteFrequencies[i] * 0.8; // 提高音调
            playLoudTone(adjustedFreq, duration);
            return;
        }
    }
}

void stopAlarm() {
    tempAlarm = false;
    humiAlarm = false;
    digitalWrite(BUZZER_PIN, LOW);
    emergencyNoteIndex = 0;
}

// ========== 显示函数 ==========

void displayOnScreen(float temp, float hum) {
    tft.fillRect(0, 80, 320, 100, TFT_BLACK);
    
    // 显示温度
    tft.setTextSize(3);
    tft.setCursor(20, 90);
    tft.setTextColor(TEXT_COLOR, TFT_BLACK);
    tft.print("温度: ");
    
    if (tempAlarm) {
        tft.setTextColor(TEMP_HIGH_COLOR, TFT_BLACK);
        // 红色强烈闪烁效果
        static int blinkCount = 0;
        blinkCount++;
        if (blinkCount % 3 == 0) {
            tft.fillRect(150, 90, 80, 35, TFT_RED);
        }
    } else if (temp < 15.0) {
        tft.setTextColor(TEMP_LOW_COLOR, TFT_BLACK);
    } else {
        tft.setTextColor(TEMP_NORMAL_COLOR, TFT_BLACK);
    }
    tft.print(temp, 1);
    tft.setTextColor(TEXT_COLOR, TFT_BLACK);
    tft.print(" C");
    
    // 显示湿度
    tft.setCursor(20, 130);
    tft.setTextColor(TEXT_COLOR, TFT_BLACK);
    tft.print("湿度: ");