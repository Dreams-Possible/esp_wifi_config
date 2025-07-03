#include"sp_wifi.h"

//配置
#define WIFI_HOST_SSID "ESP_WIFI"
#define WIFI_HOST_PASS "00000000"
#define STA_MAX_SSID_LEN 32
#define STA_MAX_PASS_LEN 32
#define STA_MAX_OTHER_LEN 16
#define STA_UPDATE_CNT 10

//WiFi初始化
sp_t sp_wifi_init();
//WiFi硬件初始化
static sp_t wifi_hardware_init();
//WiFi服务器GET服务
static esp_err_t wifi_server_get_handler(httpd_req_t*req);
//WiFi服务器POST服务
static esp_err_t wifi_server_post_handler(httpd_req_t*req);
//WiFi服务器初始化
static sp_t wifi_server_init();
//WiFi连接
static void wifi_connect();
//WiFi保存
static void wifi_save();
//WiFi读取
static void wifi_read();
//WiFi事件回调
static void wifi_event_handle(void*arg,esp_event_base_t event_base,int32_t event_id,void*event_data);
//IP事件回调
static void ip_event_handle(void*arg,esp_event_base_t event_base,int32_t event_id,void*event_data);
//WiFi保存进程
static void wifi_save_process(void*arg);
//WiFi保持服务
static void wifi_keep_server(void*arg);

//WiFi数据
typedef struct wifi_data_t
{
    char ssid[STA_MAX_SSID_LEN];
    char pass[STA_MAX_PASS_LEN];
    u8 connect_state;
    u8 connect_update;
}wifi_data_t;
static wifi_data_t wifi_data={0};

//WiFi初始化
sp_t sp_wifi_init()
{
    esp_err_t ret=ESP_OK;
    //WiFi硬件初始化
    ret=wifi_hardware_init();
    //WiFi服务器初始化
    ret=wifi_server_init();
    //检查
    if(ret!=ESP_OK)
    {
        SP_LOG("init fail");
        return SP_FAIL;
    }
    SP_LOG("init ok");
    return SP_OK;
}

//WiFi硬件初始化
static sp_t wifi_hardware_init()
{
    esp_err_t ret=ESP_OK;
    //格式化NVS文件系统
    ret=nvs_flash_erase();
    //初始化NVS文件系统
    ret=nvs_flash_init();
    //初始化网络协议
    ret=esp_netif_init();
    //创建默认事件循环
    ret=esp_event_loop_create_default();
    //初始化WiFi
    wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();
    ret=esp_wifi_init(&cfg);
    //设置WiFi模式
    ret=esp_wifi_set_mode(WIFI_MODE_APSTA);
    //创建默认WiFi热点
    esp_netif_t*esp_netif_ap=esp_netif_create_default_wifi_ap();
    wifi_config_t ap_cfg=
    {
        .ap=
        {
            .ssid=WIFI_HOST_SSID,
            .ssid_len=strlen(WIFI_HOST_SSID),
            .password=WIFI_HOST_PASS,
            .max_connection=1,
            .authmode=WIFI_AUTH_WPA2_PSK,
            .pmf_cfg=
            {
                .required=true,
            },
        },
    };
    ret=esp_wifi_set_config(WIFI_IF_AP,&ap_cfg);
    //创建默认WiFi连接
    esp_netif_t*esp_netif_sta=esp_netif_create_default_wifi_sta();
    //创建WiFi和IP事件回调
    ret=esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handle,NULL,NULL);
    ret=esp_event_handler_instance_register(IP_EVENT,ESP_EVENT_ANY_ID,&ip_event_handle,NULL,NULL);
    //启动WiFi
    ret=esp_wifi_start();
    //WiFi读取
    wifi_read();
    //检查
    if(ret!=ESP_OK&&(esp_netif_ap&&esp_netif_sta))
    {
        SP_LOG("init fail");
        return SP_FAIL;
    }
    //WiFi保持服务
    if(xTaskCreate(wifi_keep_server,"wifi_keep_server",1024*4,NULL,8,NULL)!=pdPASS)
    {
        SP_LOG("server error");
        return SP_FAIL;
    }
    SP_LOG("init ok");
    return SP_OK;
}

//WiFi服务器GET服务
static esp_err_t wifi_server_get_handler(httpd_req_t*req)
{
    //提取前端页面
    u8*file=NULL;
    u32 len=0;
    if(sp_fs_read("/fs/esp_wifi.html",&file,&len)!=SP_OK)
    {
        SP_LOG("read fail");
        return ESP_FAIL;
    }
    char*str=malloc(len+1);
    if(!str)
    {
        SP_LOG("no memory");
        free(file);
        return ESP_FAIL;
    }
    snprintf(str,len,"%s",(char*)file);
    free(file);
    httpd_resp_send(req,str,HTTPD_RESP_USE_STRLEN);
    free(str);
    return ESP_OK;
}

//WiFi服务器POST服务
static esp_err_t wifi_server_post_handler(httpd_req_t*req)
{
    //获取长度
    u16 len=req->content_len;
    if(len>STA_MAX_SSID_LEN+STA_MAX_PASS_LEN+STA_MAX_OTHER_LEN)
    {
        SP_LOG("too long");
        return ESP_FAIL;
    }
    //准备资源
    char*data=malloc(STA_MAX_SSID_LEN+STA_MAX_PASS_LEN+STA_MAX_OTHER_LEN);
    if(!data)
    {
        SP_LOG("no memory");
        return ESP_FAIL;
    }
    memset(data,0,STA_MAX_SSID_LEN+STA_MAX_PASS_LEN+STA_MAX_OTHER_LEN);
    //接收数据
    httpd_req_recv(req,data,len);
    //解析数据
    // printf("data=%s",data);
    cJSON*root=cJSON_Parse(data);
    free(data);
    if(!root)
    {
        SP_LOG("no memory");
        return ESP_FAIL;
    }
    cJSON*ssid=cJSON_GetObjectItem(root,"ssid");
    cJSON*pass=cJSON_GetObjectItem(root,"pass");
    //保存数据
    if(ssid&&pass)
    {
        snprintf(wifi_data.ssid,STA_MAX_SSID_LEN,"%s",ssid->valuestring);
        snprintf(wifi_data.pass,STA_MAX_PASS_LEN,"%s",pass->valuestring);
    }else
    {
        SP_LOG("no data");
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    cJSON_Delete(root);
    //响应
    httpd_resp_send(req,"成功接收",HTTPD_RESP_USE_STRLEN);

    //WiFi保存
    if(xTaskCreate(wifi_save_process,"wifi_save_process",1024*4,NULL,8,NULL)!=pdPASS)
    {
        SP_LOG("save error");
    }
    
    //如果在连接状态
    if(wifi_data.connect_state)
    {
        //断开当前WiFi
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    //WiFi连接
    wifi_connect();
    return ESP_OK;
}

//WiFi服务器初始化
static sp_t wifi_server_init()
{
    esp_err_t ret=ESP_OK;
    //启动HTTP服务器
    httpd_handle_t server=NULL;
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();
    ret=httpd_start(&server,&config);
    //注册GET服务
    httpd_uri_t get=
    {
        .uri="/",
        .method=HTTP_GET,
        .handler=wifi_server_get_handler,
        .user_ctx=NULL
    };
    httpd_register_uri_handler(server,&get);
    //注册POST服务
    httpd_uri_t post=
    {
        .uri="/post",
        .method=HTTP_POST,
        .handler=wifi_server_post_handler,
        .user_ctx=NULL
    };
    httpd_register_uri_handler(server,&post);
    //检查
    if(ret!=ESP_OK)
    {
        SP_LOG("init fail");
        return SP_FAIL;
    }
    SP_LOG("init ok");
    return SP_OK;
}

//WiFi连接
static void wifi_connect()
{
    wifi_config_t sta_cfg={0};
    snprintf((char*)sta_cfg.sta.ssid,STA_MAX_SSID_LEN,"%s",wifi_data.ssid);
    snprintf((char*)sta_cfg.sta.password,STA_MAX_PASS_LEN,"%s",wifi_data.pass);
    esp_wifi_set_config(WIFI_IF_STA,&sta_cfg);
    esp_wifi_connect();
    return;
}

//WiFi保存
static void wifi_save()
{
    //转换json格式
    cJSON*root=cJSON_CreateObject();
    if(!root)
    {
        SP_LOG("no memory");
        return;
    }
    cJSON_AddStringToObject(root,"ssid",wifi_data.ssid);
    cJSON_AddStringToObject(root,"pass",wifi_data.pass);
    char*json_str=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    //保存到文件
    if(sp_fs_save("/fs/wifi_config.txt",(u8*)json_str,strlen(json_str))!=SP_OK)
    {
        free(json_str);
        SP_LOG("save fail");
        return;
    }
    free(json_str);
    SP_LOG("save succ");
    return;
}

//WiFi读取
static void wifi_read()
{
    u8*data=NULL;
    u32 len=0;
    //从文件读取
    if(sp_fs_read("/fs/wifi_config.txt",&data,&len)!=SP_OK)
    {
        SP_LOG("read fail");
        return;
    }
    char*json_str=malloc(len+1);
    if(!json_str)
    {
        SP_LOG("no mem");
        free(data);
        return;
    }
    snprintf(json_str,len+1,"%s",(char*)data);
    free(data);
    //解析json格式
    cJSON*root=cJSON_Parse(json_str);
    if(!root)
    {
        SP_LOG("no json\n");
        return;
    }
    cJSON*ssid=cJSON_GetObjectItem(root,"ssid");
    cJSON*pass=cJSON_GetObjectItem(root,"pass");
    if(ssid&&pass)
    {
        //提取到内存
        snprintf(wifi_data.ssid,STA_MAX_SSID_LEN,"%s",ssid->valuestring);
        snprintf(wifi_data.pass,STA_MAX_PASS_LEN,"%s",pass->valuestring);
    }else
    {
        SP_LOG("no data");
        cJSON_Delete(root);
        return;
    }
    cJSON_Delete(root);
    SP_LOG("read succ");
    return;
}

//WiFi事件回调
static void wifi_event_handle(void*arg,esp_event_base_t event_base,int32_t event_id,void*event_data)
{
    //断开连接
    if(event_id==WIFI_EVENT_STA_DISCONNECTED)
    {
        //更新WiFi状态为未连接
        wifi_data.connect_state=0;
        SP_LOG("dis connect");
    }
}

//IP事件回调
static void ip_event_handle(void*arg,esp_event_base_t event_base,int32_t event_id,void*event_data)
{
    //连接成功
    if(event_id==IP_EVENT_STA_GOT_IP)
    {
        //更新WiFi状态为已连接
        wifi_data.connect_state=1;
    }
}

//WiFi保存进程
static void wifi_save_process(void*arg)
{
    while(1)
    {
        //WiFi保存
        wifi_save();
        vTaskDelete(NULL);
    }
}

//WiFi保持服务
static void wifi_keep_server(void*arg)
{
    while(1)
    {
        //如果不在连接状态
        if(!wifi_data.connect_state)
        {
            //WiFi有效
            if(strlen(wifi_data.ssid))
            {
                //自动重连
                SP_LOG("try connect");
                //WiFi连接
                wifi_connect();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(STA_UPDATE_CNT*1000));
    }
}
