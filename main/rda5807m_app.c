#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rda5807m.h"
#include <string.h>
#include <esp_log.h>
#include "lvgl_gui.h"

#define I2C_PORT I2C_NUM_0

#if defined(CONFIG_IDF_TARGET_ESP8266)
#define SDA_GPIO 4
#define SCL_GPIO 5
#else
#define SDA_GPIO 5
#define SCL_GPIO 4
#endif

//static const char *TAG = "rda5807m_app";

// #define MAX_STA 30     //最多允许的电台数
// uint8_t stations = 0;     //当前电台
// uint32_t fmstation[MAX_STA]={0};
uint32_t rda5807m_current_fre = 103300;   // 当前FM模块频率


rda5807m_t rda5807m_dev = { 0 };
// static const char *states[] = {
//     [RDA5807M_SEEK_NONE]     = "---",
//     [RDA5807M_SEEK_STARTED]  = ">>>",
//     [RDA5807M_SEEK_COMPLETE] = "***",
//     [RDA5807M_SEEK_FAILED]   = "...",
// };








/**
 * @brief 获取5807目前状态，给其他文件使用
 * 
 * @param state 要写入的状态变量
 */
void rda5807m_app_get_state(rda5807m_state_t * state)
{
    ESP_ERROR_CHECK(rda5807m_get_state(&rda5807m_dev, state));
    // ESP_LOGI(TAG, "[ %3d.%d MHz ] [ %s ] [ %c ] [ %6s ] [ %3s ] [ RSSI: %3d ]",
    //             (*state).frequency / 1000, ((*state).frequency % 1000) / 100,
    //             states[(*state).seek_status],
    //             (*state).station ? 'S' : ' ',
    //             (*state).stereo ? "Stereo" : "Mono",
    //             (*state).rds_ready ? "RDS" : "",
    //             (*state).rssi);
}

/**
 * @brief rda5807初始化
 * 
 */
void rda5807m_app_init()
{
    /* IIC初始化 */
    ESP_ERROR_CHECK(i2cdev_init());

    rda5807m_dev.i2c_dev.cfg.scl_pullup_en = true;
    rda5807m_dev.i2c_dev.cfg.sda_pullup_en = true;

    ESP_ERROR_CHECK(rda5807m_init_desc(&rda5807m_dev, I2C_PORT, SDA_GPIO, SCL_GPIO));
    ESP_ERROR_CHECK(rda5807m_init(&rda5807m_dev, RDA5807M_CLK_32768HZ));

    rda5807m_set_frequency_khz(&rda5807m_dev, rda5807m_current_fre);
    rda5807m_set_volume(&rda5807m_dev,RDA5807M_VOL_MAX);
}

/**
 * @brief 频率增加100KHz
 * 
 */
void rda5807m_app_add_frequency(uint32_t delta)
{
    rda5807m_current_fre += delta;
    if (rda5807m_current_fre > 108000)
        rda5807m_current_fre = 87000;
    rda5807m_set_frequency_khz(&rda5807m_dev, rda5807m_current_fre);
}

/**
 * @brief 频率减少100KHz
 * 
 */
void rda5807m_app_reduce_frequency(uint32_t delta)
{
    rda5807m_current_fre -= delta;
    if (rda5807m_current_fre < 87000)
        rda5807m_current_fre = 108000;
    rda5807m_set_frequency_khz(&rda5807m_dev, rda5807m_current_fre);
}

// //搜索收音机的台  从8700~10800 MZH
// void search() {
//   uint8_t i = 0;
//   uint8_t num= 0;
//   rda5807m_state_t state;
//   rda5807m_set_frequency_khz(&rda5807m_dev, rda5807m_current_fre);    //从8700MHZ开始扫描
//   for(i=0;i<210;i++)
//   {
//    vTaskDelay(100 / portTICK_PERIOD_MS);   
//    rda5807m_app_get_state(&state);     // 获得5807目前状态
//    if(state.rssi>35)
//    {
//        fmstation[num++]=state.frequency;
//    }
//    rda5807m_current_fre+=100;
//    rda5807m_set_frequency_khz(&rda5807m_dev, rda5807m_current_fre);   
//    if (num >= MAX_STA) return;
//   }
//   ESP_LOGI(TAG,"found %d stations",num);

// }
