#pragma once

//头文件
#include"sp_sys.h"
#include<stdio.h>
#include<string.h>
#include"esp_littlefs.h"

//初始化文件系统
sp_t sp_fs_init();
//读取文件
sp_t sp_fs_read(char*path,u8**file,u32*len);
//保存文件
sp_t sp_fs_save(char*path,u8*data,u32 len);
