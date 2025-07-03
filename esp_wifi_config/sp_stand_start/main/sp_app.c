#include"sp_app.h"
#include"sp_wifi.h"
#include"sp_fs.h"

//用户应用
void sp_app();

//用户应用
void sp_app()
{
    //串口初始化
    while(sp_uart_init()!=SP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // while(1)
    // {
    //     // //从串口输入
    //     // char*data=sp_uart_rx();
    //     // printf("data=%s\n",data);

    //     //从串口输入
    //     u8*data=NULL;
    //     u16 len=0;
    //     sp_uart_rx_bin(&data,&len);
    //     sp_uart_tx_bin(data,len);
    // }
    
    //文件系统初始化
    while(sp_fs_init()!=SP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    //WiFi初始化
    while(sp_wifi_init()!=SP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }


}
