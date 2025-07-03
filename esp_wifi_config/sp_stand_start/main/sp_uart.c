#include"sp_uart.h"

//配置
#define TX_PIN UART_PIN_NO_CHANGE
#define RX_PIN UART_PIN_NO_CHANGE
#define UART_PORT UART_NUM_0
#define UART_RATE 115200
#define MAX_TX_LEN 1*1024
#define MAX_RX_LEN 1*1024

//串口数据
typedef struct uart_data_t
{
    u8 rx_data[MAX_RX_LEN+1];
    char tx_data[MAX_TX_LEN+1];
    QueueHandle_t uart_event;
    SemaphoreHandle_t rx_event;
    u16 rx_len;
}uart_data_t;
static uart_data_t uart_data={0};

//串口接收服务
static void uart_rx_server(void*arg);
//初始化串口
uint8_t sp_uart_init();
//串口接收
char*sp_uart_rx();
//串口接收字节
void sp_uart_rx_bin(u8**data,u16*len);
//串口发送
void sp_uart_tx(char*data,...);
//串口发送字节
void sp_uart_tx_bin(u8*data,u16 len);

//串口接收服务
static void uart_rx_server(void*arg)
{
    while(1)
    {
        //阻塞直到有串口事件
        uart_event_t event={0};
        if(xQueueReceive(uart_data.uart_event,&event,portMAX_DELAY))
        {
            //如果是收到数据
            if(event.type==UART_DATA)
            {
                //重新读取
                memset(uart_data.rx_data,0,uart_data.rx_len);
                uart_data.rx_len=event.size;
                uart_read_bytes(UART_PORT,uart_data.rx_data,event.size,portMAX_DELAY);
                //收到新串口数据
                xSemaphoreGive(uart_data.rx_event);
            }
        }
    }
}

//初始化串口
sp_t sp_uart_init()
{
    //初始化资源
    uart_data.rx_event=xSemaphoreCreateBinary();
    uart_data.rx_len=0;
    esp_err_t ret=ESP_OK;
    //配置串口参数
    uart_config_t config=
    {
        .baud_rate=UART_RATE,
        .data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE,
        .stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,
        .source_clk=UART_SCLK_DEFAULT,
    };
    //安装串口驱动
    ret=uart_driver_install(UART_PORT,MAX_RX_LEN,MAX_RX_LEN,1,&uart_data.uart_event,0);
    //配置串口
    ret=uart_param_config(UART_PORT,&config);
    //配置引脚
    ret=uart_set_pin(UART_PORT,TX_PIN,RX_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
    //等待配置完成
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("\n");
    //检查
    if(ret!=ESP_OK)
    {
        SP_LOG("init fail");
        return SP_FAIL;
    }
    //创建串口接收服务
    if(xTaskCreate(uart_rx_server,"uart_rx_server",1024*2,NULL,8,NULL)!=pdPASS)
    {
        SP_LOG("server error");
        return SP_FAIL;
    }
    SP_LOG("init ok");
    return SP_OK;
}

//串口接收
char*sp_uart_rx()
{
    //等待接收
    xSemaphoreTake(uart_data.rx_event,portMAX_DELAY);
    //返回读取的数据
    return (char*)uart_data.rx_data;
}

//串口接收字节
void sp_uart_rx_bin(u8**data,u16*len)
{
    //等待接收
    xSemaphoreTake(uart_data.rx_event,portMAX_DELAY);
    //返回读取的数据
    *data=(u8*)&uart_data.rx_data;
    //返回读取的数据长度
    *len=uart_data.rx_len;
}

//串口发送
void sp_uart_tx(char*data,...)
{
    //更新发送缓冲区
    memset(uart_data.tx_data,0,MAX_TX_LEN);
    va_list val;
    va_start(val,data);
    vsnprintf(uart_data.tx_data,MAX_TX_LEN,data,val);
    va_end(val);
    //串口发送
    uart_write_bytes(UART_PORT,(const void*)uart_data.tx_data,strlen(uart_data.tx_data));
}

//串口发送字节
void sp_uart_tx_bin(u8*data,u16 len)
{
    //串口发送
    uart_write_bytes(UART_PORT,(const void*)data,len);
}
