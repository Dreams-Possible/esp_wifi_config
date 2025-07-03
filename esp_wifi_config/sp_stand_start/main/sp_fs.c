#include"sp_fs.h"


//初始化文件系统
sp_t sp_fs_init();
//读取文件
sp_t sp_fs_read(char*path,u8**file,u32*len);
//保存文件
sp_t sp_fs_save(char*path,u8*data,u32 len);


//初始化文件系统
sp_t sp_fs_init()
{
    //初始化littleFS
    esp_vfs_littlefs_conf_t cfg=
    {
        .base_path = "/fs",
        .partition_label = "storage",
    };
    esp_err_t ret=ESP_OK;
    ret=esp_vfs_littlefs_register(&cfg);
    size_t total=0,used=0;
    ret=esp_littlefs_info(cfg.partition_label,&total,&used);
    //检查
    if(ret!=ESP_OK)
    {
        SP_LOG("init fail");
        return SP_FAIL;
    }
    SP_LOG("total:%d,used:%d",total,used);
    SP_LOG("init ok");
    return SP_OK;
}

//读取文件
sp_t sp_fs_read(char*path,u8**file,u32*len)
{
    //打开文件
    FILE*f=fopen(path,"r");
    if(!f)
    {
        SP_LOG("no file");
        return SP_FAIL;
    }
    //获取文件大小
    fseek(f,0,SEEK_END);
    u32 file_size=ftell(f);
    rewind(f);
    *len=file_size;
    //申请资源
    // printf("file_size=%ld",file_size);
    u8*file_data=malloc(file_size);
    if(!file_data)
    {
        SP_LOG("no memory");
        fclose(f);
        return SP_FAIL;
    }
    //读取文件
    fread(file_data,1,file_size,f);
    *file=file_data;
    fclose(f);
    return SP_OK;
}

//保存文件
sp_t sp_fs_save(char*path,u8*data,u32 len)
{
    FILE*f=fopen(path,"w");
    if(!f)
    {
        SP_LOG("save fail\n");
        return SP_FAIL;
    }
    if(fwrite(data,1,len,f)!=len)
    {
        fclose(f);
        SP_LOG("save fail\n");
        return SP_FAIL;
    }
    fclose(f);
    return SP_OK;
}
