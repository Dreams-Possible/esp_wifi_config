#include"sp_sys.h"

//入口任务
void app_main(void);
//系统初始化任务
static void sys_main(void*arg);

//入口任务
void app_main(void)
{
    //创建系统初始化任务
    xTaskCreate(sys_main,"sys_main",1024*4,NULL,8,NULL);
}

//系统初始化任务
static void sys_main(void*arg)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    SP_LOG("sp_app_init");
    vTaskDelay(pdMS_TO_TICKS(100));
    sp_app();
    vTaskDelay(pdMS_TO_TICKS(100));
    SP_LOG("sp_app_ok");
    vTaskDelete(NULL);
}
