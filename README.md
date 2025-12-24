# 基于XIAO ESP32S3 Sense的智能鱼缸监控系统

## 一、项目介绍

- 本项目基于Seed Studio XIAO ESP32S3 Sense开发板，构建了一套完整的智能鱼缸监控系统。该系统集成了温湿度监测、实时视频监控、自动喂食、环境照明控制等功能，通过Wi-Fi接入Home Assistant智能家居平台，实现远程监控和自动化管理。项目采用模块化设计，结合Arduino IDE和SenseCraft AI平台，无需复杂编程即可快速部署AI视觉识别功能。

![项目图片](2.png)

## 二、主要功能

- **环境监测**：实时采集鱼缸外部温湿度数据，确保饲养环境稳定；
- **视频监控**：通过OV2640摄像头提供1600 * 1200分辨率的实时视频流，支持远程观察；
- **自动喂食**：定时或根据条件触发舵机控制喂食器；
- **智能照明**：根据时间自动调节LED灯带，模拟自然光照；
- **语音控制**：集成小爱同学语音助手，支持语音播报和指令控制；
- **远程管理**：通过Home Assistant实现手机APP远程监控和控制。

## 三、工作原理

- 系统采用树莓派400作为主控服务器，XIAO ESP32S3 Sense作为客户端设备。ESP32S3通过Wi-Fi连接家庭网络，将传感器数据上传至Home Assistant平台。摄像头模块通过M-JPEG协议传输视频流，温湿度传感器通过单总线协议采集数据。
- NodeRED自动化引擎负责处理逻辑判断，如温度异常报警、定时喂食等。SenseCraft AI平台用于训练和部署物体识别模型，实现鱼类行为监测和异常检测

## 四、所需组件

### （一）硬件模块

| 组件名称 | 型号/规格 | 数量 | 功能 |
|---------|----------|------|------|
| 主控开发板 | Seed Studio XIAO ESP32S3 Sense | 1 | 核心控制 |
| 温湿度传感器 | DHT11 | 1 | 检测环境 |
| 舵机 | SG90 | 1 | 控制喂食 |
| 电源模块 | 5V/3A | 1 | 供电 |
| 面包板/杜邦线 | - | 若干 | 连接 |

### （二）软件模块

- **Arduino IDE**：用于ESP32S3固件开发和烧录；
- **SenseCraft AI**：AI模型训练和部署平台；
- **Home Assistant**：智能家居控制中心；
- **NodeRED**：可视化自动化流程设计工具；
- **ESPHome**：ESP32设备配置管理工具。

## 五、构建步骤

### 步骤1：开发环境配置

- 1.1 安装Arduino IDE从Arduino官网下载最新版本IDE，安装完成后打开软件。
- 1.2 添加ESP32支持在"文件→首选项"中添加ESP32开发板管理器URL：
  - `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- 进入"工具→开发板→开发板管理器"，搜索"esp32"并安装最新版本。
- 1.3 配置开发板参数选择开发板为"ESP32S3 Dev Module"，关键配置如下：
  - Flash Mode: QIO
  - Flash Size: 8MB
  - PSRAM: Enabled
  - USB CDC On Boot: Enabled
  - CPU Frequency: 240MHz

### 步骤2：硬件连接

#### 2.1 传感器接线

- DHT11: DATA引脚接GPIO8，VCC接3.3V，GND接地
- 舵机：信号线接GPIO7，VCC接5V，GND接地
- LED灯带：数据线接GPIO21，VCC接5V，GND接地

#### 2.2 摄像头连接

XIAO ESP32S3 Sense已集成OV2640摄像头，无需额外接线。摄像头引脚配置如下：
- SDA: GPIO40
- SCL: GPIO39
- VSYNC: GPIO38
- HREF: GPIO47
- PCLK: GPIO13

### 步骤3：ESPHome配置

#### 3.1 创建配置文件

在Home Assistant中安装ESPHome插件，创建新设备配置文件：

```yaml
esphome:
  name: xiao-cam
  friendly_name: XIAO ESP32S3 Sense

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: arduino

# 启用日志记录
logger:

# 启用Home Assistant API
api:
  encryption:
    key: 'your-encryption-key'

# WiFi配置
wifi:
  ssid: "your-wifi-ssid"
  password: "your-wifi-password"

# 摄像头配置
esp32_camera:
  external_clock:
    pin: GPIO10
    frequency: 20MHz
  i2c_pins:
    sda: GPIO40
    scl: GPIO39
  data_pins: [GPIO15, GPIO17, GPIO18, GPIO16, GPIO14, GPIO12, GPIO11, GPIO48]
  vsync_pin: GPIO38
  href_pin: GPIO47
  pixel_clock_pin: GPIO13
  name: "Fish Tank Camera"

# 温湿度传感器
sensor:
  - platform: dht
    pin: GPIO8
    temperature:
      name: "Temperature"
    humidity:
      name: "Humidity"
    update_interval: 5s

# 舵机控制
servo:
  - id: my_servo
    output: pwm_output

output:
  - platform: ledc
    id: pwm_output
    pin: GPIO7
    frequency: 50 Hz

# LED灯带
light:
  - platform: status_led
    id: light0
    name: "Fish Tank Light"
    pin:
      number: GPIO21
      inverted: true

# 语音助手
voice_assistant:
  microphone: echo_microphone

# 麦克风配置
microphone:
  - platform: i2s_audio
    id: echo_microphone
    i2s_din_pin: GPIO41
    adc_type: external
    pdm: true
```

#### 3.2 烧录固件

将XIAO ESP32S3 Sense通过USB连接到电脑，在ESPHome中选择"Install"编译并烧录固件。烧录完成后设备会自动连接到Wi-Fi。

### 步骤4：Home Assistant集成

#### 4.1 添加设备

在Home Assistant的"配置→设备与服务"中，点击"添加集成"，搜索"ESPHome"，输入设备IP地址或使用自动发现功能添加设备。

#### 4.2 配置自动化

在NodeRED中创建自动化流程：

```json
[
  {
    "id": "feed-fish",
    "type": "cron-plus",
    "name": "定时喂食",
    "cron": "0 12 * * *",
    "payload": "",
    "payloadType": "date",
    "copy": "",
    "isTimeZone": false,
    "isEnabled": true,
    "wires": [["servo-control"]]
  },
  {
    "id": "servo-control",
    "type": "api-call-service",
    "name": "控制舵机",
    "service": "servo.write",
    "data": {"servo_id":"servo.my_servo","level":100},
    "dataType": "json",
    "mergecontext": "",
    "server": "homeassistant",
    "version": 1,
    "debugenabled": false,
    "outputs": 0,
    "wires": []
  }
]
```

## 六、使用说明

1. **启动系统**：接通电源后，系统会自动连接到Wi-Fi并初始化所有传感器。
2. **远程监控**：打开Home Assistant APP，在设备列表中找到鱼缸监控系统，即可查看实时视频和环境数据。
3. **自动喂食**：系统会按照预设时间自动喂食，也可以通过APP手动控制喂食。
4. **语音控制**：通过小爱同学语音助手，可以控制鱼缸照明和查询环境数据。
5. **异常报警**：当温度超出预设范围时，系统会通过APP推送报警信息。

## 七、注意事项

1. 确保所有硬件连接正确，避免短路和接反电源。
2. 首次使用前，需在Arduino IDE中正确配置开发板参数。
3. 定期检查设备状态，确保传感器和摄像头正常工作。
4. 为延长设备寿命，建议使用稳定的5V/3A电源适配器。
5. 在使用AI功能时，需确保设备已连接到互联网并完成模型部署。


