# 智慧宿舍系统 (Smart Dormitory System)

基于 **OneNET 云平台** 的宿舍环境监测与控制系统,由 **STM32 嵌入式终端** 与 **uni-app 移动端 APP** 两部分组成,实现温湿度、烟雾浓度的实时采集、云端上传与远程可视化。

## 系统架构

```
┌─────────────────┐   ┌──────────┐   ┌──────────┐   ┌──────────────┐   ┌──────────────┐
│ DHT11 温湿度     │   │          │   │          │   │              │   │              │
│ MQ-2 烟雾传感器  │──→│ STM32F103│──→│ ESP8266  │──→│ OneNET 云平台 │──→│ uni-app 手机 │
│ 按键 / 蜂鸣器    │   │  (HAL)   │   │  (WiFi)  │   │   (MQTT)     │   │  APP (HTTP)  │
│ 风扇 / OLED     │   │          │   │          │   │              │   │              │
└─────────────────┘   └──────────┘   └──────────┘   └──────────────┘   └──────────────┘
```

数据链路:传感器数据 → STM32 采集处理 → ESP8266 通过 MQTT 上报至 OneNET → APP 调用 OneNET HTTP API 轮询展示。

## 实现功能

- **温湿度测量**:DHT11 传感器采集环境温度、湿度
- **烟雾浓度检测**:MQ-2 烟雾传感器经 ADC 采样,浓度超标自动触发蜂鸣器报警
- **OneNET 云平台数据上传与可视化**:MQTT 协议上报物模型属性(temp / humi / MQ2)
- **多端数据传输**:APP 通过 OneNET HTTP API 实时获取设备数据
- **WiFi 连接功能**:ESP8266 AT 指令自动配网、连接 MQTT 服务器
- **按键开关功能**:本地按键控制蜂鸣器与风扇(HS-F04A 电机驱动)
- **本地显示**:OLED 屏实时刷新温湿度与烟雾浓度
- **历史曲线**:APP 端折线图展示温湿度变化趋势

## 项目结构

```
SmartDormitorySystem/
├── APP程序/                  # uni-app 移动端(Vue 2,HBuilderX 工程)
│   ├── pages/
│   │   ├── index/           # 主页面:设备状态 + 温湿度/烟雾卡片
│   │   └── LineChart/       # 折线图页面:温湿度实时曲线
│   ├── static/              # 图标资源
│   ├── uni_modules/         # qiun-data-charts 图表组件
│   ├── key.js               # OneNET 访问令牌生成(HMAC-SHA1)
│   ├── manifest.json        # 应用配置(已打包 Android APK)
│   └── unpackage/           # 构建产物
│
├── STM32/                    # STM32F103 嵌入式端(CubeMX + Keil MDK-ARM)
│   ├── Core/
│   │   ├── Inc/             # 头文件
│   │   └── Src/
│   │       ├── main.c       # 主程序:初始化与主循环
│   │       ├── esp8266.c    # ESP8266 AT 指令驱动
│   │       ├── onenet.c     # OneNET 连接与数据上报
│   │       ├── MqttKit.c    # MQTT 协议编解码(自实现)
│   │       ├── Dht11.c      # DHT11 温湿度驱动(软件时序)
│   │       ├── adc.c        # ADC 采样(MQ-2 烟雾)
│   │       ├── OLED.c       # OLED 显示驱动
│   │       ├── HS_F04A.c    # 风扇电机驱动
│   │       ├── key.c        # 按键扫描(支持短按/长按)
│   │       └── usart.c      # 串口配置
│   ├── Drivers/             # CMSIS 与 HAL 库
│   ├── MDK-ARM/             # Keil 工程文件
│   └── road.ioc             # CubeMX 配置文件
│
└── README.md
```

## 硬件资源分配

| 外设 | 引脚 | 说明 |
|------|------|------|
| USART1 | PA9 / PA10 | 调试串口(printf 输出) |
| USART2 | PA2 / PA3 | ESP8266 通信(AT 指令) |
| DHT11 DATA | PB11 | 温湿度传感器(单总线时序) |
| ADC1 | PA4 / PA6 | MQ-2 烟雾传感器模拟量采样 |
| OLED (模拟I2C) | PB8 / PB9 | 0.96 寸 OLED 显示 |
| 风扇电机 (HS-F04A) | PB12 / PB13 | 正转 / 反转 / 停止 |
| 蜂鸣器 | PB10 | 烟雾超标报警 |
| 按键 1 | PB14 | 控制蜂鸣器 |
| 按键 2 | PB15 | 控制风扇 |

## 软件说明

### STM32 端

- **MCU**:STM32F103 系列,72MHz(HSE + PLL),HAL 库开发
- **工作流程**:
  1. 初始化 GPIO / USART / ADC / OLED / 电机 / 按键
  2. ESP8266 复位 → AT 指令连接 WiFi → TCP 连接 `mqtts.heclouds.com:1883`
  3. MQTT CONNECT 鉴权接入 OneNET
  4. 主循环每约 1 秒:读取 DHT11 与烟雾 ADC 值 → OLED 刷新显示 → JSON 封装 `{"temp":..,"humi":..,"MQ2":..}` 通过 MQTT 属性上报 → 串口打印日志
  5. 烟雾浓度 ≥ 25 时蜂鸣器自动报警
  6. 接收并解析 OneNET 命令下发(`OneNet_RevPro`),预留远程控制接口
- **MQTT 协议**:`MqttKit.c` 为自实现的 MQTT 3.1.1 编解码,不依赖第三方库

### APP 端

- **框架**:uni-app(Vue 2),支持 Android / 小程序等多端
- **主页面**:设备在线状态卡片 + 温度 / 湿度 / 烟雾浓度三张数据卡片,每 3 秒通过 OneNET 物模型接口 `iot-api.heclouds.com/thingmodel/query-device-property` 刷新
- **折线图页面**:基于 `qiun-data-charts` 组件,每 2 秒刷新,最多保留最近 10 个数据点
- **鉴权**:`key.js` 使用用户密钥进行 HMAC-SHA1 签名,生成 OneNET 访问令牌

## 使用说明

### STM32 端配置

1. 修改 `Core/Src/esp8266.c` 中的 WiFi 信息:
   ```c
   #define ESP8266_WIFI_INFO  "AT+CWJAP=\"你的WiFi名\",\"密码\"\r\n"
   ```
2. 修改 `Core/Src/onenet.c` 中的 OneNET 设备信息:
   ```c
   #define PROID      "产品ID"
   #define AUTH_INFO  "鉴权token"
   #define DEVID      "设备名称"
   ```
3. 用 Keil MDK-ARM 打开 `MDK-ARM` 下的工程,编译下载至开发板。

### APP 端配置

1. 修改 `pages/index/index.vue` 中的产品号、设备号与用户密钥:
   ```js
   const my_product_id = "产品ID"
   const my_device_name = "设备名称"
   // author_key / user_id 填入自己的 OneNET 用户信息
   ```
2. 使用 HBuilderX 打开 `APP程序` 目录,运行到浏览器 / 手机模拟器,或云打包生成 APK。

## 技术栈

| 端 | 技术 |
|----|------|
| 嵌入式 | STM32F103 HAL 库、CubeMX、Keil MDK-ARM、C |
| 通信 | ESP8266 AT 指令、MQTT 3.1.1、TCP |
| 云平台 | 中国移动 OneNET(物模型、设备接入) |
| 移动端 | uni-app(Vue 2)、qiun-data-charts、OneNET OpenAPI |

---

## 待优化项记录

> 说明:以下为代码评审发现的优化点,按优先级 P0(功能缺陷)→ P1(稳定性)→ P2(安全)→ P3(工程质量)排序,均已定位到具体文件与代码位置,尚未修改。

### P0 — 功能缺陷(影响核心功能)

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| 1 | 远程控制链路是断的 | `STM32/Core/Src/onenet.c` `OneNet_RevPro` | 命令下发解析函数**从未在主循环被调用**,平台下发的属性设置收不到,设备只会上报、不受控;APP 端 `index.vue` 的 `onLedSwitch` 也没有对应 UI 组件。建议:主循环轮询解析 `OneNet_RevPro`,补全下发命令到 GPIO 的执行逻辑 |
| 2 | APP 两页查的不是同一台设备 | `APP程序/pages/LineChart/LineChart.vue:76-77` vs `pages/index/index.vue:64-65` | 主页用 `xUHsdh4wh3/test`,折线图页用 `WN0YwGS4TQ/d1`,图表页必然拿不到数据。建议:统一配置 |
| 3 | 在线判断恒为真 | `APP程序/pages/index/index.vue:121-127` | `lastUpdateTime` 刚被赋 `Date.now()` 就参与判断,`≤300000` 永远成立,设备掉线也显示"在线"。建议:改用 OneNET 返回的设备最后上报时间戳判断 |
| 4 | 定时器泄漏 | `index.vue:101-103`、`LineChart.vue:65-67` | `setInterval` 注册在 `onShow` 且从不清理,页面来回切换会叠加多个定时器、重复请求。建议:移到 `onLoad`,或在 `onHide`/`onUnload` 中 `clearInterval` |
| 5 | 数据索引硬编码 | 两个页面均用 `data[0..2].value` | OneNET 返回顺序不保证固定,且两页取索引不一致(主页 `[2][1][0]`,图表页 `[2][0]`)。建议:按返回数据的 `identifier` 字段匹配,而非数组下标 |

### P1 — 稳定性(设备长期运行)

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| 1 | 无断线重连机制 | `STM32/Core/Src/main.c` | ESP8266 TCP 断开、WiFi 掉线、MQTT 被踢均无检测与重连逻辑,只在启动时连一次,断电重启才能恢复。建议:周期检测连接状态并实现自动重连 |
| 2 | MQTT 心跳缺失 | `onenet.c`(连接时 keepalive=256) | 主循环从未发送 PINGREQ,空闲一段时间会被服务器断开。建议:按 keepalive 间隔定时发送心跳包 |
| 3 | DHT11 时序易受干扰 | `STM32/Core/Src/Dht11.c` | 用 SysTick 忙等实现微秒延时,USART2 中断接收 ESP8266 数据会打断时序导致偶发读取失败。建议:读取期间屏蔽中断,或改用定时器捕获实现精确时序 |
| 4 | 阻塞式架构 | `main.c` 主循环 | `HAL_Delay(10)` 轮询,ESP8266 发命令最多阻塞 2 秒,期间按键、OLED、上报全部停滞。建议:引入 FreeRTOS 任务划分,或改为状态机调度 |
| 5 | ADC 未滤波 | `main.c` `Get_ADC_Value`、`STM32/Core/Src/adc.c` | 烟雾值单次采样直接使用,MQ-2 噪声大且有预热期;且 `adc_value*100/4096` 实为百分比,OLED/APP 却标注 "ppm",单位有误导。建议:多次采样取平均,并统一数据单位 |

### P2 — 安全

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| 1 | 凭据泄露(最严重) | `APP程序/pages/index/index.vue:90`(`author_key`)、`STM32/Core/Src/esp8266.c:13`(WiFi 密码)、`onenet.c:20`(鉴权签名) | 密钥明文硬编码且已提交进 git 历史。建议:轮换密钥;APP 端由自有后端代理生成 token,密钥不放入客户端 |
| 2 | MQTT 明文传输 | `esp8266.c:15` 连接 1883 端口 | 无 TLS 加密,数据可被窃听/篡改。建议:切换 `mqtts` 加密连接(需评估 ESP8266 资源) |
| 3 | APP 权限过度申请 | `APP程序/manifest.json` | 申请了 CAMERA、READ_PHONE_STATE、GET_ACCOUNTS、WRITE_SETTINGS 等与功能无关的权限。建议:按需裁剪 |

### P3 — 工程质量

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| 1 | 死代码 / 矛盾代码 | `onenet.c`(约一半为注释掉的旧函数)、`STM32/Core/Src/key.c` vs `main.c` | `key.c` 写了完整的按键状态机(短按/长按)但 `main.c` 直接裸读 GPIO 且无消抖,按键代码白写、蜂鸣器还与烟雾报警互相覆盖控制;`Data[5]`、`a_esp_buf` 等遗留 extern 变量未清理。建议:统一按键处理逻辑,删除死代码 |
| 2 | 宏污染 | `STM32/Core/Src/HS_F04A.c:8` | `#define GPIO_Port GPIOB` 等通用名宏极易与 HAL 定义冲突。建议:改用带模块前缀的命名(如 `HS_F04A_GPIO_PORT`) |
| 3 | OLED 全屏清屏重绘 | `main.c` + `STM32/Core/Src/OLED.c` | 每秒 `OLED_Clear()` 后全量重画,屏幕会闪烁。建议:只更新变化区域 |
| 4 | 配置散落 | APP 端 URL、product_id、设备名在多个文件重复硬编码 | 建议:提取统一的 config 模块管理 |
| 5 | 构建产物入库 | `APP程序/unpackage/`(含 APK)、`.idea/` | 不应进版本库。建议:补充 `.gitignore` |
| 6 | 无测试 / CI | `STM32/Core/Src/MqttKit.c`(1352 行自实现 MQTT 编解码) | 协议编解码完全无单元测试,出错代价高。建议:在 PC 环境编写解码单元测试,并建立 CI |

### 建议修复顺序

1. **P0 全部 5 项** — 投入小,立刻恢复核心功能(远程控制、图表数据、在线状态、定时器)
2. **P1 第 1、2 项** — 断线重连 + MQTT 心跳,是设备长期挂网的底线
3. **P2 第 1 项** — 至少先轮换密钥并把硬编码凭据移出仓库
4. 其余项随日常维护逐步处理
