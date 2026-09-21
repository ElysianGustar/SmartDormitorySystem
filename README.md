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

1. 复制 `STM32/Core/Inc/net_config.h` 为同目录 `net_config.local.h`(已 git 忽略),填写 WiFi 与 OneNET 信息:
   ```c
   // net_config.local.h
   #define ESP8266_WIFI_INFO	"AT+CWJAP=\"你的WiFi名\",\"密码\"\r\n"
   #define PROID     "产品ID"
   #define AUTH_INFO "鉴权签名token"
   #define DEVID     "设备名称"
   ```
2. 用 Keil MDK-ARM 打开 `MDK-ARM` 下的工程,编译下载至开发板。

### APP 端配置

1. 复制 `APP程序/config.js` 为同目录 `config.local.js`(已 git 忽略),填写 OneNET 用户秘钥:
   ```js
   // config.local.js
   module.exports = {
       product_id: '产品ID',
       device_name: '设备名称',
       author_key: '你的用户秘钥',
       user_id: '你的用户id'
   }
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

> 说明:以下为代码评审发现的优化点,按优先级 P0(功能缺陷)→ P1(稳定性)→ P2(安全)→ P3(工程质量)排序,均已定位到具体文件与代码位置。其中标注的这些项在发现时尚未修改,后续部分已在下方「修复记录」中修复,仍存在的项以现状为准。

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
| 5 | 构建产物入库 | `APP程序/unpackage/`(实测 51MB,含 3 个 APK 约 46MB)、`.idea/` | 不应进版本库,且仓库当前没有 `.gitignore`。建议:补充 `.gitignore` 并清理已入库产物 |
| 6 | 无测试 / CI | `STM32/Core/Src/MqttKit.c`(1352 行自实现 MQTT 编解码) | 协议编解码完全无单元测试,出错代价高。建议:在 PC 环境编写解码单元测试,并建立 CI |

### 补充新发现（二次走查，尚未修改）

> 说明:以下为在既有 P0-P3 之外新发现的问题,标 "需硬件确认" 的项需要先对照原理图/实物接线后再改代码。

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| N1 | ADC 采样引脚配置不一致(需硬件确认) | `STM32/road.ioc` vs `STM32/Core/Src/adc.c` vs `STM32/Core/Src/main.c:132/206` | `.ioc` 配置为 `ADC1_IN4(PA4)`、`ADC2_IN6(PA6)`;但 `adc.c` 实际生成的是 `ADC1 channel1(PA1)`、`ADC2 channel4(PA4)`,且 `main.c` 只调用 `MX_ADC1_Init()` 并读 `hadc1`。当前代码实际采的是 **PA1**,若 MQ-2 接在 PA4/PA6,烟雾值会不可信。建议:确认 MQ-2 实际引脚后,统一 `.ioc`、生成代码与 `main.c` 的采样通道 |
| N2 | ADC 时钟超规格 | `STM32/Core/Src/main.c:279` | 当前 `RCC_ADCPCLK2_DIV2`,APB2=72MHz 时 ADCCLK=36MHz,超过 STM32F103 ADC 最大 14MHz;`.ioc` 中本为 `RCC_ADCPCLK2_DIV6`(12MHz),说明 CubeMX 配置与生成代码不同步。建议:改为不超过 14MHz 的分频并重新核对采样时间 |
| N3 | DHT11 校验和可能误杀合法帧 | `STM32/Core/Src/Dht11.c:181` | `buf[0]+buf[1]+buf[2]+buf[3] == buf[4]` 未取低 8 位;DHT11 校验和为 sum 的低 8 位,当前和超过 255 时会把合法数据判失败。建议:改为 `((buf[0]+buf[1]+buf[2]+buf[3]) & 0xFF) == buf[4]` |
| N4 | 按键上下拉配置与按下电平逻辑矛盾(需硬件确认) | `STM32/Core/Src/key.c:14`、`STM32/Core/Src/main.c:185/191` | `Key_Init()` 使用 `GPIO_PULLDOWN`,注释却写"按键接 GND";`main.c` 又把 `GPIO_PIN_RESET` 当按下。若按键按下接地,通常应使用上拉输入;当前配置可能一直误判按下或无法稳定检测。建议:按原理图确认按键接法,统一上下拉与有效电平 |
| N5 | APP token 生成依赖 Node `crypto`/`Buffer`,且固定一年有效 | `APP程序/key.js:1/4/8`、`APP程序/pages/index/index.vue:88-97` | uni-app 真机/小程序环境不一定存在 Node 内置 `crypto`/`Buffer`;`et` 固定为当前时间 +365 天,过期后无续期/重签/鉴权失败处理。建议:token 由自有后端签发,或前端改用纯 JS HMAC-SHA1,并处理 401/过期重建 |
| N6 | APP 未检查 OneNET 业务错误,折线图页无失败处理 | `APP程序/pages/index/index.vue:118-133`、`APP程序/pages/LineChart/LineChart.vue:82-107` | 只处理 `uni.request` 网络层 `fail`,未检查 HTTP 状态码与 OneNET 返回的 `errno/error`,也未防御 `res.data.data` 为空;token 失效或设备不存在时可能直接取 `data[x].value` 异常。`LineChart.vue` 甚至没有 `fail` 回调。建议:统一封装 API 请求,先判断状态码/`errno`,再按 `identifier` 取数 |
| N7 | MQTT 发布 topic 在协议层硬编码 | `STM32/Core/Src/MqttKit.c:8` | `#define str "$sys/xUHsdh4wh3/test/thing/property/post"` 与 `onenet.c` 的 `PROID/DEVID` 重复;只改 `onenet.c` 不改这里会把数据发到旧设备 topic。建议:发布 topic 由 `PROID/DEVID` 动态生成 |
| N8 | 自实现 MQTT 解析器存在指针/长度隐患 | `STM32/Core/Src/MqttKit.c` | 例:`MQTT_UnPacketPublish()` 中 `strchr((int8 *)topic, '+')` 应为 `strchr(*topic, ...)`;`if(pkt_id == 0)` 应为 `if(*pkt_id == 0)`;多处 `MQTT_ReadLength()` 返回值未检查;`MQTT_UnPacketCmd()` 的 `remain_len - 44` 在畸形包下可能下溢。建议:补齐长度/指针校验,并为 `MqttKit.c` 增加 PC 端单元测试 |
| N9 | QoS1 发布未确认 PUBACK,且发完即清空接收缓冲 | `STM32/Core/Src/MqttKit.c:432`、`STM32/Core/Src/main.c:228-230` | 数据发布使用 QoS1,服务器会回 PUBACK;当前未确认发布结果,`OneNet_SendData()` 后立即 `ESP8266_Clear()`,还可能清掉平台下发报文。建议:保留并解析接收缓冲,区分 PUBACK 与下发命令 |
| N10 | 蜂鸣器有效电平注释与代码不一致(需硬件确认) | `STM32/Core/Src/gpio.c:54`、`STM32/Core/Src/main.c:186/209` | `gpio.c` 注释称 PB10 高电平"不响",`main.c` 却用 `GPIO_PIN_SET` 触发蜂鸣器。建议:确认有源/无源蜂鸣器及触发电平,统一宏定义 |
| N11 | 启动时单独拉高 PB12,与风扇停止状态冲突 | `STM32/Core/Src/main.c:178-179`、`STM32/Core/Src/HS_F04A.c` | 风扇初始化后应为停止,`main.c` 又手动把 IN_A/PB12 拉高,可能造成上电瞬间风扇误动作。建议:删除该行的裸 GPIO 操作,风扇状态只由 `HS_F04A_Ctrl()` 管理 |
| N12 | `Delay.c` 直接改写 SysTick,存在破坏 HAL tick 的风险 | `STM32/Core/Src/Delay.c` | `Delay_ms()/Delay_us()` 直接修改 `SysTick->LOAD/VAL/CTRL`,若被调用会影响 `HAL_GetTick()`、`HAL_Delay()` 及各类超时。当前 DHT11 未使用它,但建议删除或改为不破坏 HAL tick 的实现 |
| N13 | 危险权限补充 | `APP程序/manifest.json:25-40` | 除已记录的权限过度问题外,还包含 `READ_LOGS`、`MOUNT_UNMOUNT_FILESYSTEMS` 等与本功能无关且敏感的权限。建议:裁剪到仅保留网络状态等必要权限 |
| N14 | ESP8266 初始化/等待接口缺少超时边界 | `STM32/Core/Src/esp8266.c:197-250`、`ESP8266_GetIPD()` | `ESP8266_Init()` 对每个 AT 命令都是 `while(ESP8266_SendCmd(...))` 无限重试;`ESP8266_GetIPD(0)` 这类调用在 `do...while(timeOut--)` 下会下溢成超长等待。建议:增加最大重试次数与超时失败返回 |

### 补充新发现（三次走查，尚未修改）

> 说明:以下为对既有清单逐条复核之外新发现的问题,STM32 端 X 系列、APP 端 M 系列。注意:该次复核结论("零修复")反映的是**修复实施前**的代码状态,后续经 `a788a1d`、`dc6203e` 两次提交后,既有 28 项中已有 15+ 项修复(见下方「修复记录」),且本次走查所列 X1、X6 已随 P0-1 一并解决,请勿据此误判现状。另经实测确认:`unpackage/` 入库 51MB(3 个 APK 约 46MB),仓库原本无 `.gitignore`(已通过 P3 修复补齐)。

#### STM32 端

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| X1 | 订阅函数从未调用,下行链路第二处断点 | `STM32/Core/Src/onenet.c:410` | `OneNET_Subscribe()` 定义后从未被调用(map 确认被剔除),即使修复 P0-1 轮询解析,设备也未订阅 `thing/property/set`。建议:MQTT 连接成功后调用订阅,与 P0-1 一并修复。✅ **已随 P0-1 修复**:`main.c:172` 连接成功后已调用 `OneNET_Subscribe()` |
| X2 | DHT11 校验失败静默沿用旧值 | `STM32/Core/Src/Dht11.c:181-188` | 校验和失败无 `else` 分支,函数仍返回 0,temp/humi 保留上周期旧值却按成功上报/显示,故障无感知(与 N3 叠加)。建议:校验失败返回非 0,调用方据返回值决定上报策略 |
| X3 | 无 UART 错误回调,接收通道可永久失效 | `STM32/Core/Src/main.c:93-104` | 仅实现 `HAL_UART_RxCpltCallback`,未实现 `HAL_UART_ErrorCallback`,USART2 发生 ORE/帧错误后 HAL 停止接收且无人重启。建议:实现错误回调,清除错误标志并重启 `HAL_UART_Receive_IT` |
| X4 | ESP8266 接收变量缺 `volatile` 且无互斥 | `STM32/Core/Src/esp8266.c:18-19`、`main.c:97-102` | ISR 写 `esp8266_buf`/`esp8266_cnt`,主循环 `ESP8266_Clear()` 清零,无 `volatile` 无互斥,存在丢字节、索引错乱、strstr 扫描中被改写的竞态。建议:加 `volatile`,清零时短暂关中断或改双缓冲 |
| X5 | 接收缓冲 128B 回绕 + `strstr` 越界读 | `main.c:73/97-100`、`esp8266.c:94` | 缓冲满时 `esp8266_cnt=0` 直接回绕致新旧帧混杂;缓冲全满无 NUL 结尾,`strstr` 越界读;AT 回显+IPD 突发易超 128B。建议:扩大缓冲、始终保证 NUL 结尾,或改环形队列 |
| X6 | `OneNet_RevPro` PUBACK 分支空指针解引用 | `onenet.c:299-316` | PUBACK 分支不设置 `result=-1`,`req_payload` 仍为 NULL 即进入 `strchr(req_payload,':')`,P0-1 修复后立即变 HardFault。建议:进入解析前先判空。✅ **已随 P0-1 修复**:PUBACK 现为独立 case(`onenet.c:341-346`),仅断言 `MQTT_UnPacketPublishAck` 返回值,不再解析 NULL 载荷 |
| X7 | 全工程无看门狗 | `main.c:161/175`、`Error_Handler`、`ESP8266_Init` 重试环 | IWDG 未启用,而卡死点全是 `while(1)`,无人值守设备死机需现场断电。建议:启用 IWDG 并在主循环喂狗 |
| X8 | `.ioc` 引脚缺失 + 用户代码写在生成区 | `STM32/road.ioc`、`gpio.c:54` | PB10(蜂鸣器)、PB13(风扇 IN_B)、PB14/15(按键)、PB11(DHT11)、PB6/7(OLED 电源)均不在 `.ioc` 中,且蜂鸣器初始电平写在 CubeMX 生成区(非 USER CODE 段),重新生成代码即静默丢失(比 N1/N2 范围更广)。建议:补齐 `.ioc` 配置,用户代码移入 USER CODE 段 |
| X9 | `OneNet_FillBuf` 无长度检查 | `onenet.c:96-111/219` | 对 `buf[128]` 连续 `strcpy/strcat/sprintf`,当前约 90 字节侥幸安全,`smoke_value` 为 int 无范围约束,字段扩展或异常值即溢出。建议:改 `snprintf` 并检查剩余长度 |
| X10 | `numBuf[10]` 数字解析无上限 | `onenet.c:322-326` | 按"连续数字"循环写入无上限,超长数字串溢出(latent,随 P0-1 修复激活)。建议:循环加写入上限 |
| X11 | MqttKit payload 无界扫描 | `MqttKit.c:896` | `while(payload[data_len_t++] != '}')` 无界扫描,遇畸形数据越界读(latent,仅未被调用的 SaveBinData 分支触发)。建议:加长度边界 |
| X12 | ADC 从未校准 | `adc.c` / `main.c` | F1 ADC 上电后未调用 `HAL_ADCEx_Calibration_Start`(map 显示被剔除),存在未校准偏移误差。建议:初始化后执行校准 |
| X13 | 对未初始化的 `hadc2` 调中断处理 | `stm32f1xx_it.c:213` | `hadc2.Instance == NULL`,中断一旦触发即空指针解引用;且轮询模式下 `adc.c:136` 使能 ADC 中断本就多余。建议:移除该中断处理或初始化 hadc2 |
| X14 | HAL 返回值普遍未检查 | `main.c:109-110`、`esp8266.c:84-105` 等 | `HAL_ADC_Start/PollForConversion` 超时则上报陈旧 DR 值;`RxCpltCallback` 里 `HAL_UART_Receive_IT` 重启失败被忽略;`ESP8266_SendCmd` 超时返回前不清缓冲,残留数据可致下一轮 `strstr` 误匹配旧关键字。建议:检查关键 HAL 调用返回值 |
| X15 | 魔数遍布 | `main.c:199/208`、`esp8266.c:87`、`MqttKit.c:530-538`、`OLED.c` | `timeCount>=100`、烟雾阈值 25、`timeOut=200`、`37/36/44`、`0x78` 等均无命名常量。建议:提取为带语义的宏/常量 |
| X16 | OLED 字库索引无范围检查 + GPIO 供电 | `OLED.c:164/15-19` | `OLED_F8x16[Char-' ']` 无字符范围检查(越界读);PB7/PB6 当 VDD/GND 给屏幕供电,GPIO 驱动能力受限且两引脚不在 `.ioc` 中。建议:加范围检查,供电改电源轨 |
| X17 | DHT11 微秒延时下溢隐患 | `Dht11.c:53-75` | `DHT11_Delay_us` 硬编码 72MHz/72000,`udelay` 约 >1ms 时 `72000+startval-delays` 下溢成死等(当前最大调用 40us 未触发,latent)。建议:限制入参范围或改定时器实现 |
| X18 | `ESP8266_IRQHandler` 死声明 | `main.c:91`、`esp8266.h:20` | 全工程无定义无调用,可并入 P3-1 清理 |

#### APP 端

| # | 问题 | 位置 | 说明与建议 |
|---|------|------|-----------|
| M1 | 图表数据未做数值转换 | `APP程序/pages/LineChart/LineChart.vue:6/101-102` | OneNET `value` 字段是 JSON 字符串,直接 push 进 series,uCharts 对字符串数据可能渲染异常/静默不画。建议:`Number()` 转换后再入列 |
| M2 | 轮询无节流,弱网请求堆积 | `pages/index/index.vue:101`、`LineChart.vue:65` | 轮询周期 2s/3s 远小于 `uni.request` 默认 60s 超时,弱网/服务端挂起时请求大量并发堆积。建议:请求显式设置超时,加 in-flight 去重 |
| M3 | 卡片跳转图表页不传参 | `index.vue:159-166`、`LineChart.vue:58` | 温/湿/烟雾三张卡片都跳同一图表页且不带参数,`onLoad(options)` 的 `options` 未使用,点"烟雾浓度"卡片看不到 MQ2 曲线。建议:跳转携带 `identifier` 参数,图表页按参数取数 |
| M4 | token 明文持久化跨页传递 | `index.vue:161`、`LineChart.vue:60` | `uni.setStorageSync('token')` 明文持久化一年期令牌(H5 端即 localStorage,XSS 可窃取);且 LineChart 只在 `onLoad` 读一次,直接启动/存储被清时 token 为空、请求静默失败。建议:token 不落地,改内存传递或每页重建,并配合 N6 的错误处理 |
| M6 | 折线图页标题为空 | `APP程序/pages.json:13` | `navigationBarTitleText` 为空串,`globalStyle` 里还留默认 "uni-app" 标题。建议:补齐页面标题 |
| M7 | 小程序关闭域名校验 | `APP程序/manifest.json:58-60` | `mp-weixin.setting.urlCheck: false` 属开发期便利配置,按微信小程序发布存在合规风险(与 N5 的 mp 环境不兼容叠加)。建议:发布前开启并配置合法域名 |
| M8 | 注释/调试残留 | `index.vue:42/50/120` | "二氧化碳设备卡片"实为烟雾浓度、`:50` 注释写湿度实为 MQ2;每 3 秒 `console.log(res.data)` 调试残留。建议:修正注释、删除调试输出 |
| M9 | `led`/`onLedSwitch` 死代码 | `index.vue:79/137-157` | data 与方法在模板中无任何绑定组件(P0-1 补充,此前仅指出"无对应 UI")。建议:删除或补全 UI |

> M5(澄清,非问题):`key.js` 的 HMAC-SHA1 签名与 OneNET token 算法(v2018-10-31:base64 解码 key → HMAC → base64 → urlencode 拼装)逐步比对**实现正确**,无新增签名 bug;问题仍仅限 N5 的运行环境依赖与固定一年有效期。

### 建议修复顺序

1. **先确认硬件接线与有效电平**:MQ-2 实际 ADC 引脚、按键接法、蜂鸣器触发电平;然后优先修 **N1/N2/N3/N4**
2. **P0 全部 5 项 + X1 + X6** — 投入小,打通远程控制链路(解析轮询 + 订阅 + 判空),恢复图表数据/在线状态/定时器;APP 侧顺手修 **M1/M3** 让图表页真正可用
3. **P1 第 1、2 项 + X2/X3/X4/X5/X7** — 断线重连 + MQTT 心跳 + 串口/缓冲/看门狗,是设备长期挂网的底线
4. **P2 第 1 项 + N5 + M4** — 至少先轮换密钥并把硬编码凭据移出仓库,APP token 改由后端签发或换纯 JS 实现、不落地存储
5. **N6/N7/N8/N9 + X9/X10/X11** — 统一 APP API 错误处理,修 MQTT topic 硬编码、协议解析器与缓冲区隐患
6. **X8 尽早单独处理** — 补齐 `.ioc` 引脚配置并把用户代码移入 USER CODE 段,避免 CubeMX 重新生成时静默丢代码
7. 其余项随日常维护逐步处理

## 修复记录

按上述顺序已完成以下修复:

### P0 — 功能缺陷(已修复)

| # | 修复内容 | 位置 |
|---|---------|------|
| 1 | 主循环轮询 `OneNet_RevPro`;连接成功后订阅 `thing/property/set` 与 `thing/property/desired/set`;`led` 属性下发执行至蜂鸣器 PB10,并回复 `set_reply`;APP 新增「远程蜂鸣器」开关 UI | `main.c`、`onenet.c`、`pages/index/index.vue` |
| 2 | 设备配置收敛到 `APP程序/config.js`,主页与折线图页查询同一台设备 | `config.js`、`index.vue`、`LineChart.vue` |
| 3 | 在线状态改为依据 OneNET 返回属性时间戳的最新值判断(>5 分钟视为离线),无时间戳时降级处理 | `index.vue` |
| 4 | 定时器移到 `onShow` 且带判重,`onHide`/`onUnload` 清理,避免累积 | `index.vue`、`LineChart.vue` |
| 5 | 数据解析改为按返回属性 `identifier`(`temp`/`humi`/`MQ2`)匹配 | `index.vue`、`LineChart.vue` |

### P1 — 稳定性(已修复 1、2 项)

| # | 修复内容 | 位置 |
|---|---------|------|
| 1 | 被动检测 `CLOSED`/`WIFI DISCONNECT` 等断线关键词,30s 后自动重连(关闭残留 TCP → 重连 WiFi/TCP → 重连 MQTT → 重新订阅) | `esp8266.c`、`onenet.c`、`main.c` |
| 2 | 主循环每 60s 发送 MQTT PINGREQ 心跳(`OneNet_KeepAlive`) | `onenet.c`、`main.c` |

### P2 — 安全(已修复第 1 项)

| 修复内容 | 位置 |
|---------|------|
| 明文凭据移出源码:STM32 真值(产品 ID/鉴权签名/WiFi 密码)放入 git 忽略的 `net_config.local.h`;APP 真值(`author_key`/`user_id`)放入 git 忽略的 `config.local.js`;新增 `.gitignore` | `STM32/Core/Inc/net_config.h`、`APP程序/config.js`、`.gitignore` |

> **重要**:以上真实凭据已进入 git 历史,仅移出源码不能消除历史泄露。请在 OneNET 控制台**轮换用户秘钥**,并更换 WiFi 密码,再在 `net_config.local.h` / `config.local.js` 中填入新值。

### P3 — 工程质量(已修复部分,随前序一并处理)

| 修复内容 | 位置 |
|---------|------|
| 已入库的 `unpackage/`(81 个构建产物)与 `STM32/MDK-ARM/road/`(Keil 输出)通过 `git rm --cached` 移出索引,新产物由 `.gitignore` 拦截 | `APP程序/unpackage/`、`STM32/MDK-ARM/` |
| 清理 `Data[5]`、`a_esp_buf` 等从未定义/引用的遗留 extern 声明 | `onenet.c`、`main.c` |

### P1 — 稳定性(已修复 3、5 项)

| # | 修复内容 | 位置 |
|---|---------|------|
| 3 | `DHT11_Read_Data` 时序读取置于 `__disable_irq/__enable_irq` 临界区,读取后丢弃 USART2 积压字节并清除 ORE 标志,避免 USART2 接收中断破坏微秒时序且不破坏 ESP8266 接收状态 | `Dht11.c` |
| 5 | 烟雾 ADC 多次采样取平均(16 次)+ 采样时间提到 239.5 周期;**单位统一为百分比**("ppm" 实为 `adc*100/4096`),OLED 与 APP 显示改为 `%` | `main.c`、`adc.c`、`index.vue` |

### P2 — 安全(已修复第 3 项)

| 修复内容 | 位置 |
|---------|------|
| Android 权限裁剪:CAMERA、READ_PHONE_STATE、GET_ACCOUNTS、WRITE_SETTINGS、FLASHLIGHT、READ_LOGS 等 11 项无关权限移除,仅保留网络相关 4 项 | `manifest.json` |

### P3 — 死代码清理

| 修复内容 | 位置 |
|---------|------|
| 删除 `onenet.c` 注释掉的旧函数(`OneNet_FillBuf_Temp/Light/MQ2`、`OneNet_SendData_Humi` 等) | `onenet.c` |
| 本地按键接入 `key.c` 状态机(短按切换、长按 1.2s 关闭),蜂鸣器仲裁统一为**烟雾报警 > 用户开关(buzzer_enable,本地按键与远程 led 共用)**,解决双方覆盖控制 | `main.c`、`onenet.c` |
