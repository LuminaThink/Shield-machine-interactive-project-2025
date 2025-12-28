/*
 * 盾构机渣土状态监测系统
 * 使用DHT11温湿度传感器和LED状态指示
 * 模拟监测土仓内渣土的环境状态
 */

#include <DHT.h>

// DHT11温湿度传感器配置
#define DHTPIN 2      // DHT11数据引脚连接到XIAO ESP32的D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LED状态指示灯配置 - D5改为D6
const int LED_DRY = 3;     // 过干状态指示灯（D3）
const int LED_IDEAL = 4;   // 理想状态指示灯（D4）  
const int LED_WET = 6;     // 过湿状态指示灯（D6，原D5改为D6）

// 板载LED用于系统状态指示
const int SYSTEM_LED = LED_BUILTIN;

// 渣土状态阈值（根据实际校准）
const float TEMP_DRY = 30.0;     // 温度高于此值可能表示过干
const float TEMP_WET = 20.0;     // 温度低于此值可能表示过湿
const float HUMI_DRY = 40.0;     // 湿度低于此值表示过干
const float HUMI_WET = 70.0;     // 湿度高于此值表示过湿

// 状态枚举
enum SoilStatus {
  STATUS_DRY,     // 过干
  STATUS_IDEAL,   // 理想
  STATUS_WET,     // 过湿
  STATUS_ERROR    // 传感器错误
};

// 上一次读取时间
unsigned long lastReadTime = 0;
const long READ_INTERVAL = 2000;  // 每2秒读取一次

void setup() {
  Serial.begin(115200);
  
  // 初始化DHT传感器
  dht.begin();
  
  // 初始化LED引脚
  pinMode(LED_DRY, OUTPUT);
  pinMode(LED_IDEAL, OUTPUT);
  pinMode(LED_WET, OUTPUT);
  pinMode(SYSTEM_LED, OUTPUT);
  
  // 初始状态：关闭所有LED
  digitalWrite(LED_DRY, LOW);
  digitalWrite(LED_IDEAL, LOW);
  digitalWrite(LED_WET, LOW);
  digitalWrite(SYSTEM_LED, LOW);
  
  // 系统启动指示
  startupAnimation();
  
  // 显示系统信息
  displaySystemInfo();
}

void loop() {
  unsigned long currentTime = millis();
  
  // 定时读取传感器数据
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    
    // 读取温湿度
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    // 检查传感器读数是否有效
    if (isnan(humidity) || isnan(temperature)) {
      handleSensorError();
      return;
    }
    
    // 获取渣土状态
    SoilStatus status = analyzeSoilStatus(temperature, humidity);
    
    // 更新LED显示
    updateLEDs(status);
    
    // 更新板载LED
    updateSystemLED();
    
    // 串口输出状态信息
    displayStatus(temperature, humidity, status);
  }
  
  // 简单的呼吸灯效果表示系统运行中
  breathEffect();
}

// 分析渣土状态（结合温湿度）
SoilStatus analyzeSoilStatus(float temp, float humidity) {
  // 综合判断逻辑
  int dryScore = 0;
  int wetScore = 0;
  
  // 温度判断
  if (temp > TEMP_DRY) dryScore++;
  else if (temp < TEMP_WET) wetScore++;
  
  // 湿度判断
  if (humidity < HUMI_DRY) dryScore++;
  else if (humidity > HUMI_WET) wetScore++;
  
  // 综合评分判断状态
  if (dryScore >= 2) {
    return STATUS_DRY;      // 两项指标都显示干
  } else if (wetScore >= 2) {
    return STATUS_WET;      // 两项指标都显示湿
  } else if (dryScore == 1 && wetScore == 0) {
    return STATUS_DRY;      // 一项显示干，没有显示湿
  } else if (wetScore == 1 && dryScore == 0) {
    return STATUS_WET;      // 一项显示湿，没有显示干
  } else {
    return STATUS_IDEAL;    // 其他情况为理想状态
  }
}

// 更新LED状态显示
void updateLEDs(SoilStatus status) {
  // 先关闭所有状态LED
  digitalWrite(LED_DRY, LOW);
  digitalWrite(LED_IDEAL, LOW);
  digitalWrite(LED_WET, LOW);
  
  // 根据状态点亮对应LED
  switch (status) {
    case STATUS_DRY:
      digitalWrite(LED_DRY, HIGH);
      // 过干状态闪烁提醒
      delay(100);
      digitalWrite(LED_DRY, LOW);
      delay(100);
      digitalWrite(LED_DRY, HIGH);
      break;
      
    case STATUS_IDEAL:
      digitalWrite(LED_IDEAL, HIGH);
      break;
      
    case STATUS_WET:
      digitalWrite(LED_WET, HIGH);
      // 过湿状态闪烁提醒
      delay(100);
      digitalWrite(LED_WET, LOW);
      delay(100);
      digitalWrite(LED_WET, HIGH);
      break;
      
    case STATUS_ERROR:
      // 错误状态：所有LED交替闪烁
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_DRY, HIGH);
        digitalWrite(LED_WET, HIGH);
        delay(200);
        digitalWrite(LED_DRY, LOW);
        digitalWrite(LED_WET, LOW);
        digitalWrite(LED_IDEAL, HIGH);
        delay(200);
        digitalWrite(LED_IDEAL, LOW);
      }
      break;
  }
}

// 更新系统LED状态
void updateSystemLED() {
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(SYSTEM_LED, ledState);
}

// 系统启动动画
void startupAnimation() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(SYSTEM_LED, HIGH);
    digitalWrite(LED_DRY, HIGH);
    delay(200);
    digitalWrite(LED_DRY, LOW);
    digitalWrite(LED_IDEAL, HIGH);
    delay(200);
    digitalWrite(LED_IDEAL, LOW);
    digitalWrite(LED_WET, HIGH);
    delay(200);
    digitalWrite(LED_WET, LOW);
    digitalWrite(SYSTEM_LED, LOW);
    delay(200);
  }
}

// 显示系统信息
void displaySystemInfo() {
  Serial.println();
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║     盾构机渣土状态监测系统 - 温湿度版     ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.println("║ 硬件配置:                                ║");
  Serial.println("║  • DHT11温湿度传感器                      ║");
  Serial.println("║  • 三色LED状态指示器                      ║");
  Serial.println("║  • XIAO ESP32主控板                       ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.println("║ LED状态指示:                             ║");
  Serial.println("║  • 红灯(干) - 渣土过干，需加泡沫           ║");
  Serial.println("║  • 绿灯(理想) - 渣土状态良好               ║");
  Serial.println("║  • 蓝灯(湿) - 渣土过湿，需控水             ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.println("║ 引脚配置:                               ║");
  Serial.println("║  • 过干指示灯: D3 (GPIO3)                 ║");
  Serial.println("║  • 理想指示灯: D4 (GPIO4)                 ║");
  Serial.println("║  • 过湿指示灯: D6 (GPIO6)                 ║");
  Serial.println("╠════════════════════════════════════════════╣");
  Serial.println("║ 状态判断阈值:                            ║");
  Serial.print("║  过干: 温度>");
  Serial.print(TEMP_DRY);
  Serial.print("°C 或 湿度<");
  Serial.print(HUMI_DRY);
  Serial.println("%     ║");
  Serial.print("║  过湿: 温度<");
  Serial.print(TEMP_WET);
  Serial.print("°C 或 湿度>");
  Serial.print(HUMI_WET);
  Serial.println("%     ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
}

// 显示实时状态
void displayStatus(float temp, float humidity, SoilStatus status) {
  // 清屏（发送控制字符）
  Serial.write(27);       // ESC字符
  Serial.print("[2J");    // 清屏命令
  Serial.write(27);       // ESC字符
  Serial.print("[H");     // 光标回到左上角
  
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║        实时渣土状态监测                   ║");
  Serial.println("╠════════════════════════════════════════════╣");
  
  // 显示传感器数据
  Serial.print("║ 环境温度: ");
  Serial.print(temp);
  Serial.print("°C");
  Serial.print("    ");
  if (temp > TEMP_DRY) Serial.print("↑偏高 ");
  else if (temp < TEMP_WET) Serial.print("↓偏低 ");
  else Serial.print("✓正常 ");
  Serial.println("                ║");
  
  Serial.print("║ 环境湿度: ");
  Serial.print(humidity);
  Serial.print("%");
  Serial.print("    ");
  if (humidity < HUMI_DRY) Serial.print("↓偏低 ");
  else if (humidity > HUMI_WET) Serial.print("↑偏高 ");
  else Serial.print("✓正常 ");
  Serial.println("                ║");
  
  Serial.println("╠════════════════════════════════════════════╣");
  
  // 显示状态判断
  Serial.print("║ 渣土状态: ");
  switch (status) {
    case STATUS_DRY:
      Serial.print("【过 干】需添加泡沫改良剂              ║");
      Serial.println();
      Serial.println("║ 当前LED: 🔴 红灯闪烁 (D3)             ║");
      break;
    case STATUS_IDEAL:
      Serial.print("【理 想】渣土状态良好                 ║");
      Serial.println();
      Serial.println("║ 当前LED: 🟢 绿灯常亮 (D4)             ║");
      break;
    case STATUS_WET:
      Serial.print("【过 湿】需控制注水量                 ║");
      Serial.println();
      Serial.println("║ 当前LED: 🔵 蓝灯闪烁 (D6)             ║");
      break;
    case STATUS_ERROR:
      Serial.print("【错 误】传感器异常                  ║");
      Serial.println();
      Serial.println("║ 当前LED: 🟡 黄灯交替闪烁             ║");
      break;
  }
  
  Serial.println("╠════════════════════════════════════════════╣");
  
  // 显示工程建议
  Serial.println("║           工 程 建 议                   ║");
  Serial.println("╠────────────────────────────────────────────╣");
  
  switch (status) {
    case STATUS_DRY:
      Serial.println("║ 1. 增加泡沫注入量，改善渣土流动性     ║");
      Serial.println("║ 2. 检查注水系统，确保供水正常        ║");
      Serial.println("║ 3. 监控刀盘扭矩，防止过大磨损        ║");
      break;
    case STATUS_IDEAL:
      Serial.println("║ 1. 保持当前泡沫和注水参数            ║");
      Serial.println("║ 2. 继续监控渣土状态变化             ║");
      Serial.println("║ 3. 注意前方地质条件变化             ║");
      break;
    case STATUS_WET:
      Serial.println("║ 1. 减少注水量，防止开挖面失稳        ║");
      Serial.println("║ 2. 加强排水措施，控制含水量         ║");
      Serial.println("║ 3. 监控地下水情况，防止涌入         ║");
      break;
    case STATUS_ERROR:
      Serial.println("║ 1. 检查传感器连接是否正常           ║");
      Serial.println("║ 2. 检查传感器供电是否稳定           ║");
      Serial.println("║ 3. 重启系统或更换传感器             ║");
      break;
  }
  
  Serial.println("╚════════════════════════════════════════════╝");
  
  // 显示时间戳
  Serial.print("更新时间: ");
  Serial.print(millis() / 1000);
  Serial.println("秒");
  Serial.println();
}

// 处理传感器错误
void handleSensorError() {
  Serial.println("❌ 传感器读取失败，请检查连接！");
  updateLEDs(STATUS_ERROR);
  
  // 在错误状态下，板载LED快速闪烁
  digitalWrite(SYSTEM_LED, HIGH);
  delay(100);
  digitalWrite(SYSTEM_LED, LOW);
  delay(100);
}

// 呼吸灯效果
void breathEffect() {
  static int brightness = 0;
  static bool increasing = true;
  
  if (increasing) {
    brightness += 5;
    if (brightness >= 255) {
      brightness = 255;
      increasing = false;
    }
  } else {
    brightness -= 5;
    if (brightness <= 0) {
      brightness = 0;
      increasing = true;
    }
  }
  
  // 使用PWM控制LED亮度（需要PWM引脚）
  // analogWrite(SYSTEM_LED, brightness);
  delay(20);
}