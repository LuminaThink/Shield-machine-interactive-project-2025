#include <Wire.h>
#include <TFT_eSPI.h>

// ===================== 硬件适配（修复引脚+颜色错误）=====================
// 1. I2C配置（替换PA08/PA09为GPIO数字编号8/9）
#define I2C_SLAVE_ADDR 0x27    // 匹配XIAO的I2C从机地址
#define I2C_SDA 8              // Wio Terminal SDA引脚（原PA08=GPIO8）
#define I2C_SCL 9              // Wio Terminal SCL引脚（原PA09=GPIO9）

// 2. 屏幕配置（Wio Terminal官方参数）
#define TFT_BL_PIN 37          // 背光引脚（GPIO37）
TFT_eSPI tft;                  // TFT屏幕对象
// 3. 按键配置（替换PA29/PA30/PA31为GPIO数字编号29/30/31）
#define KEY_A 29               // A键=GPIO29（原PA29）
#define KEY_B 30               // B键=GPIO30（原PA30）
#define KEY_C 31               // C键=GPIO31（原PA31）

// ===================== 功能配置 =====================
// 界面模式枚举
enum ScreenMode { MODE_REAL_TIME, MODE_HISTORY, MODE_IMAGE };
ScreenMode currentMode = MODE_REAL_TIME;

// 传感器数据变量（适配MQ7）
float recvTemp = 0.0;          // 温度
float recvHumi = 0.0;          // 湿度
int recvMQ7 = 0;               // 接收的MQ7值
String recvAlarm = "NORMAL";   // 预警状态

// 折线图配置（适配90℃阈值）
#define MAX_POINTS 20          // 最近20个温度点
float tempPoints[MAX_POINTS] = {0};
int pointIndex = 0;
unsigned long lastRecordTime = 0;
const long RECORD_INTERVAL = 5000; // 5秒记录一次
const float TEMP_MAX = 90.0;   // 折线图纵轴最大值
const float TEMP_MIN = 0.0;    // 纵轴最小值

// ===================== 核心函数 =====================
// 初始化TFT屏幕（适配Wio Terminal）
void initTFT() {
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH); // 开启背光
  tft.begin();                   // 初始化屏幕
  tft.setRotation(3);            // 横屏（Wio Terminal默认）
  tft.fillScreen(TFT_BLACK);     // 清屏
  tft.setTextColor(TFT_WHITE);   // 默认文字颜色
  tft.setTextSize(2);            // 默认字号
  Serial.println("屏幕初始化完成");
}

// 解析XIAO发送的数据（适配「温度,湿度,MQ7值,预警状态」格式）
void parseData(String data) {
  // 空数据/短数据直接返回
  if (data.length() < 5) {
    Serial.println("解析失败：数据为空/过短");
    return;
  }

  // 分割4段数据：温度,湿度,MQ7值,预警状态
  int comma1 = data.indexOf(',');
  int comma2 = data.indexOf(',', comma1 + 1);
  int comma3 = data.indexOf(',', comma2 + 1);
  // 格式错误检查
  if (comma1 == -1 || comma2 == -1 || comma3 == -1) {
    Serial.printf("解析失败：格式错误（%s）\n", data.c_str());
    return;
  }

  // 解析各字段（带异常处理）
  String tempStr = data.substring(0, comma1);
  String humiStr = data.substring(comma1 + 1, comma2);
  String mq7Str = data.substring(comma2 + 1, comma3);
  recvAlarm = data.substring(comma3 + 1);

  // 转换为数值（避免非数字崩溃）
  recvTemp = tempStr.toFloat();
  recvHumi = humiStr.toFloat();
  recvMQ7 = mq7Str.toInt();

  // 记录温度到折线图数组（5秒一次）
  if (millis() - lastRecordTime >= RECORD_INTERVAL) {
    tempPoints[pointIndex] = recvTemp;
    pointIndex = (pointIndex + 1) % MAX_POINTS;
    lastRecordTime = millis();
    Serial.printf("记录温度：%.1f℃\n", recvTemp);
  }
}

// 读取XIAO的传感器数据（I2C主机，带错误处理）
void readDataFromXiao() {
  // 请求XIAO发送数据（最多64字节）
  int recvLen = Wire.requestFrom(I2C_SLAVE_ADDR, 64);
  if (recvLen == 0) {
    Serial.println("I2C无数据：未连接XIAO或地址错误");
    return;
  }

  // 拼接接收的数据
  String data = "";
  while (Wire.available()) {
    data += (char)Wire.read();
  }
  Serial.printf("接收XIAO数据：%s\n", data.c_str());
  
  // 解析数据
  parseData(data);
}

// 绘制实时监测界面（适配MQ7显示）
void drawRealTimeScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // 1. 温度（大号字体）
  tft.setTextSize(4);
  tft.setCursor(20, 20);
  tft.print("温度: ");
  tft.print(recvTemp, 1); // 保留1位小数
  tft.print("°C");

  // 2. 湿度
  tft.setTextSize(3);
  tft.setCursor(20, 80);
  tft.print("湿度: ");
  tft.print(recvHumi, 1);
  tft.print("%RH");

  // 3. MQ7值
  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.print("MQ-7: ");
  tft.print(recvMQ7);
  tft.print(" ADC");

  // 4. 预警状态（颜色区分）
  tft.setCursor(20, 170);
  tft.print("状态: ");
  if (recvAlarm == "WARN")      tft.setTextColor(TFT_YELLOW);
  else if (recvAlarm == "ALARM") tft.setTextColor(TFT_ORANGE);
  else if (recvAlarm == "CRIT")  tft.setTextColor(TFT_RED);
  else if (recvAlarm == "FAULT") tft.setTextColor(TFT_PURPLE);
  else                          tft.setTextColor(TFT_GREEN);
  tft.print(recvAlarm);
  tft.setTextColor(TFT_WHITE); // 恢复默认颜色

  // 5. 模式提示
  tft.setTextSize(1);
  tft.setCursor(20, 220);
  tft.print("模式: 实时监测 (A键切换 | C键拍照)");
}

// 绘制温度折线图界面（修复TFT_GRAY为TFT_DARKGREY）
void drawHistoryScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // 标题
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.print("温度变化趋势 (0-90℃)");

  // 绘制坐标轴（替换TFT_GRAY为TFT_DARKGREY）
  const int X_START = 40;    // 横轴起点
  const int X_END = 280;     // 横轴终点
  const int Y_START = 40;    // 纵轴起点
  const int Y_END = 220;     // 纵轴终点
  tft.drawLine(X_START, Y_END, X_END, Y_END, TFT_DARKGREY); // 横轴（修复颜色）
  tft.drawLine(X_START, Y_START, X_START, Y_END, TFT_DARKGREY); // 纵轴（修复颜色）

  // 纵轴刻度（0/45/90℃）
  tft.setTextSize(1);
  tft.setCursor(10, Y_START); tft.print("90℃");
  tft.setCursor(10, (Y_START+Y_END)/2); tft.print("45℃");
  tft.setCursor(10, Y_END-5); tft.print("0℃");

  // 绘制温度折线（适配90℃）
  int xStep = (X_END - X_START) / (MAX_POINTS - 1); // 横轴步长
  for (int i = 0; i < MAX_POINTS - 1; i++) {
    // 循环取数（解决数组索引循环）
    int idx1 = (pointIndex + i) % MAX_POINTS;
    int idx2 = (pointIndex + i + 1) % MAX_POINTS;
    
    // 跳过空数据
    if (tempPoints[idx1] == 0 || tempPoints[idx2] == 0) continue;

    // 温度值转屏幕坐标（纵轴反转：屏幕左上角是原点）
    int y1 = Y_END - map(tempPoints[idx1], TEMP_MIN, TEMP_MAX, 0, Y_END-Y_START);
    int y2 = Y_END - map(tempPoints[idx2], TEMP_MIN, TEMP_MAX, 0, Y_END-Y_START);
    int x1 = X_START + i * xStep;
    int x2 = X_START + (i + 1) * xStep;

    // 绘制连线和数据点
    tft.drawLine(x1, y1, x2, y2, TFT_CYAN);
    tft.fillCircle(x1, y1, 2, TFT_WHITE);
  }

  // 模式提示
  tft.setCursor(20, 230);
  tft.print("模式: 历史趋势 (A键切换)");
}

// 绘制图像管理界面
void drawImageScreen() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setCursor(60, 80);
  tft.print("图像管理");
  
  tft.setTextSize(1);
  tft.setCursor(40, 130);
  tft.print("B键：查看最新照片（模拟）");
  tft.setCursor(40, 160);
  tft.print("C键：触发XIAO拍照");
  tft.setCursor(40, 200);
  tft.print("模式: 图像管理 (A键切换)");
}

// 按键扫描（非阻塞，消抖，带反馈）
void checkButtons() {
  // A键：切换界面（消抖）
  if (digitalRead(KEY_A) == LOW) {
    delay(200); // 消抖
    currentMode = (ScreenMode)((currentMode + 1) % 3);
    Serial.printf("切换模式：%d\n", currentMode);
    // 等待按键释放，避免连续切换
    while (digitalRead(KEY_A) == LOW);
  }

  // B键：图像放大查看（仅图像模式生效）
  if (digitalRead(KEY_B) == LOW) {
    delay(200);
    if (currentMode == MODE_IMAGE) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(40, 120);
      tft.print("最新照片（模拟）");
      tft.setCursor(40, 160);
      tft.print("2秒后返回...");
      delay(2000); // 模拟显示
    }
    while (digitalRead(KEY_B) == LOW);
  }

  // C键：发送拍照指令给XIAO
  if (digitalRead(KEY_C) == LOW) {
    delay(200);
    // 发送指令（带错误处理）
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write('C'); // 发送拍照指令
    int err = Wire.endTransmission();
    if (err == 0) {
      Serial.println("发送拍照指令成功");
      // 界面提示
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(40, 120);
      tft.print("已触发拍照！");
      delay(1000);
    } else {
      Serial.printf("发送拍照指令失败：错误码%d\n", err);
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(20, 120);
      tft.print("拍照指令发送失败！");
      delay(1000);
    }
    while (digitalRead(KEY_C) == LOW);
  }
}

// ===================== 初始化函数 =====================
void setup() {
  // 串口初始化（调试）
  Serial.begin(115200);
  delay(1000); // 串口稳定

  // I2C主机初始化（指定Wio Terminal引脚）
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C主机初始化完成（地址：0x%02X）\n", I2C_SLAVE_ADDR);

  // 屏幕初始化
  initTFT();

  // 按键引脚初始化（上拉输入）
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  pinMode(KEY_C, INPUT_PULLUP);
  Serial.println("按键初始化完成");

  // 初始化折线图数组
  for (int i = 0; i < MAX_POINTS; i++) {
    tempPoints[i] = 25.0; // 初始值：室温25℃
  }
  lastRecordTime = millis();

  Serial.println("Wio Terminal 初始化完成（适配MQ7）");
}

// ===================== 主循环（非阻塞，优化刷新）=====================
void loop() {
  static unsigned long lastReadTime = 0;
  const long READ_INTERVAL = 1000; // 1秒读取一次数据（非阻塞）

  // 1. 非阻塞读取XIAO数据
  if (millis() - lastReadTime >= READ_INTERVAL) {
    lastReadTime = millis();
    readDataFromXiao();
  }

  // 2. 扫描按键（无阻塞）
  checkButtons();

  // 3. 绘制当前界面（根据模式）
  switch (currentMode) {
    case MODE_REAL_TIME:
      drawRealTimeScreen();
      break;
    case MODE_HISTORY:
      drawHistoryScreen();
      break;
    case MODE_IMAGE:
      drawImageScreen();
      break;
  }

  // 轻微延时，降低CPU占用（无阻塞）
  delay(10);
}