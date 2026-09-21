#ifndef _NET_CONFIG_H_
#define _NET_CONFIG_H_

/* 本文件为 OneNET 接入与 WiFi 的默认占位配置(可入库,不含真实凭据)。
 * 真实值请填写到 net_config.local.h(已加入 .gitignore,不会提交),本文件会自动加载它。 */

#if defined(__has_include)
#  if __has_include("net_config.local.h")
#    include "net_config.local.h"
#  endif
#endif

#ifndef ESP8266_WIFI_INFO
#define ESP8266_WIFI_INFO	"AT+CWJAP=\"WIFI_SSID\",\"WIFI_PASSWORD\"\r\n"
#endif

#ifndef PROID
#define PROID      "PRODUCT_ID"
#endif

#ifndef AUTH_INFO
#define AUTH_INFO  "鉴权签名,参考 net_config.local.h"
#endif

#ifndef DEVID
#define DEVID      "DEVICE_NAME"
#endif

#endif