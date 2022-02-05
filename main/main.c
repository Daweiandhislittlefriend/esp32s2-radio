#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "screen_driver.h"
#include "esp_log.h"

#include "lvgl_gui.h"

#include "board_button.h"

#include "play_living_stream.h"

#include "get_time.h"
#include <time.h>
#include <sys/time.h>

#include "rda5807m_app.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif



//static const char *TAG = "main";

//  void CPU_Task(void* parameter);

void app_main(void)
{

    /* 开机默认网络电台输出 */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << 42) | (1ULL << 41);//41使能扬声器工作引脚 1使能，42网络电台与调频电台切换引脚 1网络电台
    io_conf.pull_down_en = 0;   
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_42, 1);
    gpio_set_level(GPIO_NUM_41, 1);

    //xTaskCreatePinnedToCore(CPU_Task,"CPU_Task",4096,NULL,1,NULL,tskNO_AFFINITY);

     /* ssd1306 LVGL初始化 */
    SSD1306_init();

    /* 加载动画显示 */
    xTaskCreate(my_lvgl_load_anim, "my_lvgl_load_anim", 2048, NULL, 0, &anim_xHandle);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    #if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
        ESP_ERROR_CHECK(esp_netif_init());
    #else
        tcpip_adapter_init();
    #endif

    /* wifi连接 */
    wifi_connect();

    /* APP_GUI绘制 */
    my_lvgl_app();

    /* 独立按键初始化 */
    board_button_init();

    /* 开启网络电台 */
    play_living_stream_start();

   /* FM模块rda5807初始化 */
    rda5807m_app_init();

    /*初始化SNTP并获取时间,获取时间成功后开启更新时间线程*/
    obtain_time();
}


// void CPU_Task(void* parameter)
// {	
//   uint8_t CPU_RunInfo[400];		//保存任务运行时间信息
  
//   while (1)
//   {
//     memset(CPU_RunInfo,0,400);				//信息缓冲区清零
    
//     vTaskList((char *)&CPU_RunInfo);  //获取任务运行时间信息
    
//     printf("---------------------------------------------\r\n");
//     printf("task_name      task_status   priority    stack   task_id\r\n");
//     printf("%s", CPU_RunInfo);
//     printf("---------------------------------------------\r\n");
    
//     memset(CPU_RunInfo,0,400);				//信息缓冲区清零
    
//     vTaskGetRunTimeStats((char *)&CPU_RunInfo);
    
//     printf("task_name       run_cnt         usage_rate\r\n");
//     printf("%s", CPU_RunInfo);
//     printf("---------------------------------------------\r\n\n");
//     vTaskDelay(3000 / portTICK_PERIOD_MS);   /* 延时500个tick */		
//   }
// }
