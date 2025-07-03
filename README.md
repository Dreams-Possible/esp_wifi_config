# esp_wifi_config
一个简易的通过ESP32的热点建立服务器并通过前端UI实现WiFi配网的例程。
ESP开机后自动开启热点，名称为“ESP_WIFI”，密码为“00000000”，任意设备连接后，访问192.168.4.1进入ESP配网前端界面，输入要连接的WiFi和密码，即可实现配网。
使用LittleFS文件系统，前端界面储存在/fs/esp_wifi.html，WiFi信息会自动保存，以便下次开机时自动连接。
支持热切换网络，重新在前端配置WiFi信息即可立即刷新连接。
如果目前没有连接WiFi，每隔十秒会尝试连接。
![image](https://github.com/user-attachments/assets/07a6466a-781f-4f37-a217-4d41823b96f8)
