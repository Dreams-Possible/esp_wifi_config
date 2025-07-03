#pragma once

//头文件
#include"sp_sys.h"
#include<stdio.h>
#include<string.h>
#include"esp_wifi.h"
#include"nvs_flash.h"
#include<esp_http_server.h>
#include"sp_fs.h"
#include"cJSON.h"

//WiFi初始化
sp_t sp_wifi_init();
