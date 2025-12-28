// 引入DHT11库
#include <DHT.h>

/************************* 硬件引脚定义 *************************/
// DHT11配置
#define DHT_TYPE DHT11       // 传感器类型：DHT11
#define DHT_PIN   2          // DHT11 DATA接GPIO2
// HC-SR501配置
#define PIR_PIN   3          // HC-SR501 OUT接GPIO3
// 蜂鸣器配置
#define BUZZER_PIN 1         // 蜂鸣器接GPIO1
// LED配置
#define LED_R     7          // 红灯接GPIO7
#define LED_Y     6          // 黄灯接GPIO6
#define LED_G     5          // 绿灯接GPIO5
#define LED_W     4          // 白灯接GPIO4

/************************* 常量与变量定义 *************************/
// 温度阈值
const float TEMP_LOW = 21.0;    // 低温阈值（19℃）
const float TEMP_HIGH = 23.0;   // 高温阈值（21℃）
// 延迟与防抖配置
const unsigned long PIR_INIT_DELAY = 10000;  // PIR传感器初始化延迟（10秒）
const unsigned long PIR_DEBOUNCE_DELAY = 100;// PIR防抖延迟（100ms）
const unsigned long DHT_READ_INTERVAL = 1000; // DHT11读取间隔（1秒）
const unsigned long WHITE_LED_HOLD_TIME = 2000; // 白灯延时熄灭时长（5秒，可修改）
// 核心修改：将蜂鸣器报警延时缩短为2秒（2000ms），可根据需求改为1秒（1000ms）、1.5秒（1500ms）
const unsigned long BUZZER_HOLD_TIME = 2000;   // 蜂鸣器报警延时时长（2秒，短延时）
// 状态变量
DHT dht(DHT_PIN, DHT_TYPE);     // 初始化DHT11对象
bool currentPirState = LOW;     // 当前PIR传感器状态（防抖后）
unsigned long lastPirDebounceTime = 0; // PIR防抖时间戳
unsigned long lastDhtReadTime = 0;    // DHT11上次读取时间戳
float currentTemp = 0.0;        // 当前温度值
float currentHum = 0.0;         // 当前湿度值
unsigned long lastPersonDetectedTime = 0; // 最后一次检测到人体的时间戳
unsigned long lastAlarmTriggerTime = 0;   // 最后一次触发报警的时间戳
bool isWhiteLedOn = false;      // 白灯当前状态标记
bool isRedLedOn = false;        // 红灯当前状态标记

/************************* 初始化函数 *************************/
void setup() {
  // 初始化串口（适配XIAO ESP32 S3的CDC串口）
  Serial.begin(115200);
  while (!Serial) {             // 等待串口就绪（仅ESP32 S3需要）
    delay(10);
  }

  // 初始化DHT11传感器
  dht.begin();

  // 初始化引脚模式
  pinMode(PIR_PIN, INPUT);      // PIR传感器为输入
  pinMode(BUZZER_PIN, OUTPUT);  // 蜂鸣器为输出
  // 初始化LED引脚为输出，并设置初始状态为灭
  pinMode(LED_R, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_W, OUTPUT);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_Y, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_W, LOW);
  // 初始化蜂鸣器为关闭
  digitalWrite(BUZZER_PIN, LOW);

  // 串口提示信息
  Serial.println("=====================================");
  Serial.println("系统初始化完成，开始运行...");
  Serial.println("PIR传感器初始化中（约10秒），期间可能有波动...");
  Serial.println("=====================================");
  // PIR传感器上电后有10秒初始化时间，跳过避免误触发
  delay(PIR_INIT_DELAY);
  Serial.println("PIR传感器初始化完成，开始检测人体！");
}

/************************* 主循环函数 *************************/
void loop() {
  unsigned long currentTime = millis();

  // 1. 读取PIR传感器状态（带防抖）
  readPirState(currentTime);

  // 2. 读取DHT11温湿度（按间隔读取，避免数据异常）
  readDhtData(currentTime);

  // 3. 控制红黄绿LED（根据温度），并更新红灯状态标记
  controlTempLEDs(currentTime);

  // 4. 控制白灯（检测到人后延时熄灭），并更新白灯状态标记
  controlWhiteLED(currentTime);

  // 5. 控制蜂鸣器（红灯+白灯亮时触发，短延时报警）
  controlBuzzer(currentTime);

  // 小延迟，降低CPU占用
  delay(50);
}

/************************* 自定义函数：读取PIR状态（防抖） *************************/
void readPirState(unsigned long currentTime) {
  // 读取当前PIR电平（原始状态）
  bool tempPirState = digitalRead(PIR_PIN);
  // 防抖处理：电平稳定超过防抖时间才更新状态
  if (currentTime - lastPirDebounceTime > PIR_DEBOUNCE_DELAY) {
    // 若检测到人体（高电平），更新最后一次检测的时间戳
    if (tempPirState == HIGH) {
      lastPersonDetectedTime = currentTime; // 刷新计时
      Serial.println("👤  检测到人体，刷新白灯延时计时！");
    }
    currentPirState = tempPirState; // 更新防抖后的PIR状态
    lastPirDebounceTime = currentTime;
  }
}

/************************* 自定义函数：读取DHT11温湿度 *************************/
void readDhtData(unsigned long currentTime) {
  // 按间隔读取，避免频繁读取导致数据错误
  if (currentTime - lastDhtReadTime > DHT_READ_INTERVAL) {
    lastDhtReadTime = currentTime;
    // 读取湿度
    currentHum = dht.readHumidity();
    // 读取温度（摄氏度）
    currentTemp = dht.readTemperature();

    // 检查是否读取失败
    if (isnan(currentHum) || isnan(currentTemp)) {
      Serial.println("❌ 读取DHT11数据失败！");
      currentTemp = 0.0; // 重置温度值，避免影响逻辑
      currentHum = 0.0;
      return;
    }

    // 串口打印温湿度（便于调试）
    Serial.print("🌡️  温湿度：");
    Serial.print(currentTemp);
    Serial.print("℃, ");
    Serial.print(currentHum);
    Serial.println("%RH");
  }
}

/************************* 自定义函数：控制红黄绿LED（温度逻辑） *************************/
void controlTempLEDs(unsigned long currentTime) {
  if (currentTemp < TEMP_LOW) {
    // 温度低于19℃：绿灯亮，红/黄灯灭
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_Y, LOW);
    digitalWrite(LED_R, LOW);
    isRedLedOn = false; // 更新红灯状态：灭
  } else if (currentTemp >= TEMP_LOW && currentTemp <= TEMP_HIGH) {
    // 温度19~21℃：黄灯亮，红/绿灯灭
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_Y, HIGH);
    digitalWrite(LED_R, LOW);
    isRedLedOn = false; // 更新红灯状态：灭
  } else {
    // 温度高于21℃：红灯亮，黄/绿灯灭
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_Y, LOW);
    digitalWrite(LED_R, HIGH);
    isRedLedOn = true; // 更新红灯状态：亮
    Serial.println("🔥  温度高于21℃ → 红灯亮");
  }
}

/************************* 自定义函数：控制白灯（延时熄灭逻辑） *************************/
void controlWhiteLED(unsigned long currentTime) {
  // 判断：当前时间是否在“最后一次检测人体的时间 + 延时时长”内
  if (currentTime - lastPersonDetectedTime < WHITE_LED_HOLD_TIME) {
    digitalWrite(LED_W, HIGH); // 白灯保持亮
    isWhiteLedOn = true;       // 更新白灯状态：亮
  } else {
    digitalWrite(LED_W, LOW);  // 延时结束，白灯熄灭
    isWhiteLedOn = false;      // 更新白灯状态：灭
  }
}

/************************* 自定义函数：控制蜂鸣器（短延时报警逻辑） *************************/
void controlBuzzer(unsigned long currentTime) {
  // 触发条件：红灯亮（温度>21℃）+ 白灯亮（检测到人或延时）
  bool alarmTrigger = isRedLedOn && isWhiteLedOn;

  // 若满足触发条件，更新最后一次报警触发的时间戳
  if (alarmTrigger) {
    lastAlarmTriggerTime = currentTime; // 刷新报警计时
    Serial.println("🚨  触发报警：红灯+白灯同时亮，刷新报警延时计时！");
  }

  // 判断：当前时间是否在“最后一次报警触发的时间 + 蜂鸣器短延时时长”内
  if (currentTime - lastAlarmTriggerTime < BUZZER_HOLD_TIME) {
    digitalWrite(BUZZER_PIN, HIGH); // 蜂鸣器持续报警
  } else {
    digitalWrite(BUZZER_PIN, LOW);  // 报警延时结束，关闭蜂鸣器
  }
}