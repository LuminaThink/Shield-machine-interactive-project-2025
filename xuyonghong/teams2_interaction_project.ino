/*
 * 超声波测距报警系统 - 毫米单位版
 * 按照指定的距离阈值设置
 * 
 * 连接：
 * HC-SR04: VCC->5V, GND->GND, Trig->GPIO4, Echo->GPIO5
 * 有源蜂鸣器: + -> GPIO6, - -> GND
 */

// 超声波引脚
const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

// 蜂鸣器引脚
const int BUZZER_PIN = 6;

// 常量定义
const float SOUND_SPEED_MM_PER_US = 0.343;   // 毫米/微秒

// 按照要求设置的距离阈值（单位：毫米）
const int DANGER_DISTANCE_MM = 20;      // <20mm：危险距离，持续报警
const int WARNING_DISTANCE_MM = 50;     // 20-50mm：警告距离，快速响3次
const int MEDIUM_DISTANCE_MM = 100;     // 50-100mm：中等距离，中速响3次
const int SAFE_DISTANCE_MM = 150;       // 100-150mm：安全距离，不报警
const int FAR_DISTANCE_MM = 200;        // 150-200mm：较远距离，慢速响2次
// >200mm：很远距离，非常慢响1次

// 蜂鸣器报警模式
enum BuzzerMode {
  BUZZER_OFF,           // 关闭（安全距离）
  BUZZER_DANGER,        // 危险：持续响
  BUZZER_WARNING,       // 警告：快速3次
  BUZZER_MEDIUM,        // 中等：中速3次
  BUZZER_FAR,           // 较远：慢速2次
  BUZZER_VERY_FAR       // 很远：非常慢1次
};

// 变量
int distanceMM = 0;      // 距离（毫米）
BuzzerMode currentMode = BUZZER_OFF;
unsigned long lastBuzzerTime = 0;
int beepCount = 0;
bool buzzerState = false;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  
  // 设置引脚模式
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始状态
  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("\n========================================");
  Serial.println("  超声波测距报警系统 - 毫米单位版");
  Serial.println("========================================");
  Serial.println("引脚配置:");
  Serial.println("  超声波: Trig=GPIO4, Echo=GPIO5");
  Serial.println("  蜂鸣器: GPIO6");
  Serial.println("\n【按照要求设置的报警规则】:");
  Serial.println("  距离阈值（毫米）:");
  Serial.println("  <20mm:    🔴 持续快速报警 (危险!)");
  Serial.println("  20-50mm:  🟡 快速响3次 (警告)");
  Serial.println("  50-100mm: 🟠 中速响3次 (中等)");
  Serial.println("  100-150mm:🟢 不报警 (安全距离)");
  Serial.println("  150-200mm:🔵 慢速响2次 (较远)");
  Serial.println("  >200mm:   🟣 非常慢响1次 (很远)");
  Serial.println("========================================\n");
  
  delay(1000);
}

// 测量距离函数（返回毫米） - 修正版
int measureDistanceMM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 修正：使用ECHO_PIN
  
  if (duration <= 0) return -1;  // 测量失败
  
  // 计算距离（毫米）
  return (int)(duration * SOUND_SPEED_MM_PER_US / 2);
}

// 根据距离设置蜂鸣器模式
BuzzerMode getBuzzerMode(int distanceMM) {
  if (distanceMM < 0) {
    return BUZZER_OFF;  // 测量失败时不报警
  }
  
  if (distanceMM < DANGER_DISTANCE_MM) {
    return BUZZER_DANGER;      // 危险距离：持续响
  }
  else if (distanceMM < WARNING_DISTANCE_MM) {
    return BUZZER_WARNING;     // 警告距离：快速响3次
  }
  else if (distanceMM < MEDIUM_DISTANCE_MM) {
    return BUZZER_MEDIUM;      // 中等距离：中速响3次
  }
  else if (distanceMM < SAFE_DISTANCE_MM) {
    return BUZZER_OFF;         // 安全距离：不报警
  }
  else if (distanceMM < FAR_DISTANCE_MM) {
    return BUZZER_FAR;         // 较远距离：慢速响2次
  }
  else {
    return BUZZER_VERY_FAR;    // 很远距离：非常慢响1次
  }
}

// 获取蜂鸣器参数
void getBuzzerParams(BuzzerMode mode, int &totalBeeps, int &beepDuration, int &pauseDuration, int &cycleDelay) {
  switch (mode) {
    case BUZZER_DANGER:
      totalBeeps = 0;        // 0表示持续响
      beepDuration = 100;
      pauseDuration = 0;
      cycleDelay = 0;
      break;
      
    case BUZZER_WARNING:     // 快速
      totalBeeps = 3;
      beepDuration = 80;     // 80ms响
      pauseDuration = 80;    // 80ms停
      cycleDelay = 800;      // 循环间隔0.8秒
      break;
      
    case BUZZER_MEDIUM:      // 中速
      totalBeeps = 3;
      beepDuration = 150;    // 150ms响
      pauseDuration = 150;   // 150ms停
      cycleDelay = 1200;     // 循环间隔1.2秒
      break;
      
    case BUZZER_FAR:         // 慢速
      totalBeeps = 2;
      beepDuration = 250;    // 250ms响
      pauseDuration = 250;   // 250ms停
      cycleDelay = 2000;     // 循环间隔2秒
      break;
      
    case BUZZER_VERY_FAR:    // 非常慢
      totalBeeps = 1;
      beepDuration = 400;    // 400ms响
      pauseDuration = 0;
      cycleDelay = 4000;     // 循环间隔4秒
      break;
      
    default:                 // OFF
      totalBeeps = 0;
      beepDuration = 0;
      pauseDuration = 0;
      cycleDelay = 0;
      break;
  }
}

// 控制蜂鸣器
void controlBuzzer() {
  unsigned long currentTime = millis();
  
  // 获取当前模式的参数
  int totalBeeps, beepDuration, pauseDuration, cycleDelay;
  getBuzzerParams(currentMode, totalBeeps, beepDuration, pauseDuration, cycleDelay);
  
  if (currentMode == BUZZER_DANGER) {
    // 持续报警模式
    digitalWrite(BUZZER_PIN, HIGH);
    beepCount = 0;
    return;
  }
  
  if (currentMode == BUZZER_OFF || totalBeeps == 0) {
    digitalWrite(BUZZER_PIN, LOW);
    beepCount = 0;
    return;
  }
  
  // 定时报警模式
  if (beepCount < totalBeeps) {
    // 正在响的次数内
    if (currentTime - lastBuzzerTime >= (buzzerState ? beepDuration : pauseDuration)) {
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
      lastBuzzerTime = currentTime;
      
      if (!buzzerState && pauseDuration > 0) {
        // 完成一次"响-停"循环
        beepCount++;
      }
    }
  } else {
    // 完成指定次数后等待
    digitalWrite(BUZZER_PIN, LOW);
    if (currentTime - lastBuzzerTime > cycleDelay) {
      beepCount = 0;
      buzzerState = false;
    }
  }
}

// 显示距离和状态
void displayStatus(int distanceMM) {
  Serial.print("距离: ");
  
  if (distanceMM < 0) {
    Serial.println("❌ 测量失败");
    return;
  }
  
  // 显示毫米和厘米
  Serial.print(distanceMM);
  Serial.print(" mm (");
  Serial.print(distanceMM / 10.0, 1);
  Serial.print(" cm) | 状态: ");
  
  if (distanceMM < DANGER_DISTANCE_MM) {
    Serial.println("🔴 危险! (持续报警)");
  }
  else if (distanceMM < WARNING_DISTANCE_MM) {
    Serial.println("🟡 警告 (快速响3次)");
  }
  else if (distanceMM < MEDIUM_DISTANCE_MM) {
    Serial.println("🟠 中等 (中速响3次)");
  }
  else if (distanceMM < SAFE_DISTANCE_MM) {
    Serial.println("🟢 安全 (不报警)");
  }
  else if (distanceMM < FAR_DISTANCE_MM) {
    Serial.println("🔵 较远 (慢速响2次)");
  }
  else {
    Serial.println("🟣 很远 (非常慢响1次)");
  }
}

void loop() {
  // 1. 测量距离（毫米）
  distanceMM = measureDistanceMM();
  
  // 2. 显示状态
  displayStatus(distanceMM);
  
  // 3. 根据距离确定报警模式
  BuzzerMode newMode = getBuzzerMode(distanceMM);
  
  // 如果模式改变，重置计数器
  if (newMode != currentMode) {
    currentMode = newMode;
    beepCount = 0;
    buzzerState = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  // 4. 控制蜂鸣器
  controlBuzzer();
  
  // 5. 延迟
  delay(50); // 主循环频率20Hz
}