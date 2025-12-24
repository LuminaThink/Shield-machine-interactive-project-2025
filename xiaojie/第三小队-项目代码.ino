#define RED_PIN    3    // D3
#define GREEN_PIN  4    // D4
#define BLUE_PIN  6 // D15

void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  Serial.println("三LED独立测试开始");
  allLEDsOff();
}

void loop() {
  // 模式1：红灯单独亮
  Serial.println("模式1: 红灯亮");
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  delay(2000);

  // 模式2：绿灯单独亮
  Serial.println("模式2: 绿灯亮");
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);
  delay(2000);

  // 模式3：蓝灯单独亮
  Serial.println("模式3: 蓝灯亮");
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, HIGH);
  delay(2000);
  digitalWrite(BLUE_PIN, LOW);

  // 模式4：红灯闪烁（报警模式）
  Serial.println("模式4: 红灯闪烁");
  for(int i = 0; i < 6; i++) {
    digitalWrite(RED_PIN, HIGH);
    delay(200);
    digitalWrite(RED_PIN, LOW);
    delay(200);
  }

  // 模式5：三灯流水灯
  Serial.println("模式5: 流水灯");
  for(int i = 0; i < 3; i++) {
    digitalWrite(RED_PIN, HIGH);
    delay(200);
    digitalWrite(RED_PIN, LOW);

    digitalWrite(GREEN_PIN, HIGH);
    delay(200);
    digitalWrite(GREEN_PIN, LOW);

    digitalWrite(BLUE_PIN, HIGH);
    delay(200);
    digitalWrite(BLUE_PIN, LOW);
  }

  // 模式6：全灭
  Serial.println("模式6: 全灭");
  allLEDsOff();
  delay(2000);
}

void allLEDsOff() {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}