// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* FreeRTOS includes */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
/* LVGL includes */
#include "lvgl_gui.h"

/* 独立按键includes */
#include "board_button.h"

/* ADF includes */
#include "audio_element.h"
#include "audio_pipeline.h"
#include "periph_wifi.h"
#include "get_time.h"

#include "play_living_stream.h"
#include "rda5807m_app.h"

// wait for execute lv_task_handler and lv_tick_inc to avoid some widget don't refresh.
#define LVGL_TICK_MS 1

static const char *TAG = "lvgl_gui";

typedef struct {
    scr_driver_t *lcd_drv;
    touch_panel_driver_t *touch_drv;
} lvgl_drv_t;


static void lv_tick_timercb(void *timer)
{
    /* Initialize a Timer for 1 ms period and
     * in its interrupt call
     * lv_tick_inc(1); */
    lv_tick_inc(LVGL_TICK_MS);
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
static SemaphoreHandle_t xGuiSemaphore = NULL;
static void gui_task(void *args)
{
    esp_err_t ret;
    lvgl_drv_t *lvgl_driver = (lvgl_drv_t *)args;

    /* Initialize LittlevGL */
    lv_init();

    /* Display interface */
    ret = lvgl_display_init(lvgl_driver->lcd_drv);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "lvgl display initialize failed");
        // lv_deinit(); /**< lvgl should be deinit here, but it seems lvgl doesn't support it */
        vTaskDelete(NULL);
    }

#if defined(CONFIG_LVGL_DRIVER_TOUCH_SCREEN_ENABLE)
    if (NULL != lvgl_driver->touch_drv) {
        /* Input device interface */
        ret = lvgl_indev_init(lvgl_driver->touch_drv);
        if (ESP_OK != ret) {
            ESP_LOGE(TAG, "lvgl indev initialize failed");
            // lv_deinit(); /**< lvgl should be deinit here, but it seems lvgl doesn't support it */
            vTaskDelete(NULL);
        }
    }
#endif

    esp_timer_create_args_t timer_conf = {
        .callback = lv_tick_timercb,
        .name     = "lv_tick_timer"
    };
    esp_timer_handle_t g_wifi_connect_timer = NULL;
    esp_timer_create(&timer_conf, &g_wifi_connect_timer);
    esp_timer_start_periodic(g_wifi_connect_timer, LVGL_TICK_MS * 1000U);

    xGuiSemaphore = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Start to run LVGL");

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
}

esp_err_t lvgl_init(scr_driver_t *lcd_drv, touch_panel_driver_t *touch_drv)
{
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
      * Otherwise there can be problem such as memory corruption and so on.
      * NOTE: When not using Wi-Fi nor Bluetooth you can pin the gui_task to core 0 */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "GUI Run at %s Pinned to Core%d", CONFIG_IDF_TARGET, chip_info.cores - 1);

    static lvgl_drv_t lvgl_driver;
    lvgl_driver.lcd_drv = lcd_drv;
    lvgl_driver.touch_drv = touch_drv;

    xTaskCreatePinnedToCore(gui_task, "lv gui", 1024 * 4, &lvgl_driver, 20, NULL, chip_info.cores - 1);

    uint16_t timeout = 20;
    while (NULL == xGuiSemaphore) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout--;
        if (0 == timeout) {
            ESP_LOGW(TAG, "GUI Task Start Timeout");
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

LV_FONT_DECLARE(Chinese_characters) // 额外字体来支持中文

lv_obj_t* internet_scr = NULL;            // 网络收音机屏幕
lv_obj_t* FM_scr = NULL;            // FM收音机模式

lv_obj_t* radio_label = NULL;        // 网络节目名称标签
static lv_obj_t* radio_info_label = NULL;   // 网络节目信息标签
lv_obj_t* fre_label = NULL;          // 当前频段标签
static lv_obj_t* rssi_label = NULL;          // 当前rssi标签
lv_obj_t* data_time_label1 = NULL;          // 当前时间标签
lv_obj_t* data_time_label2 = NULL;

lv_obj_t* int_sound_icon=NULL;//网络收音机页面下声音图标
lv_obj_t* fm_sound_icon=NULL;//FM收音机页面下声音图标

TaskHandle_t updata_rda5807m_info_task_handle = NULL;   // 更新RDA5807M信息任务句柄

void updata_rda5807m_info_task(void *pvParameters)
{
    rda5807m_state_t state;
    
    while (1)
    {
        if (lv_scr_act() == FM_scr)
        {
            // 在标签已经创建后，并且当前屏幕是FM的情况下才进行更新
            rda5807m_app_get_state(&state);     // 获得5807目前状态
            lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)(state.frequency)/1000));
            lv_label_set_text_fmt(rssi_label, "RSSI:%d", state.rssi);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
}


static void internet_radio_scr_create()
{
    internet_scr = lv_obj_create(NULL);    //创建屏幕

    /*
     * 创建label
     */
    /* 节目信息 */
    radio_info_label = lv_label_create(internet_scr);
    lv_obj_align(radio_info_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_width(radio_info_label, 128);
    lv_label_set_long_mode(radio_info_label, LV_LABEL_LONG_SCROLL_CIRCULAR);       // 循环滑动显示
    lv_label_set_text_fmt(radio_info_label,"Receive music info from aac decoder, sample_rates=24000, bits=16, ch=2");


    /* 节目名称 */
    radio_label = lv_label_create(internet_scr);
    lv_obj_align(radio_label, LV_ALIGN_TOP_LEFT, 0, 12);
    lv_obj_set_width(radio_label, 128);
    lv_label_set_long_mode(radio_label, LV_LABEL_LONG_SCROLL_CIRCULAR);       // 循环滑动显示
    lv_obj_set_style_text_font(radio_label, &Chinese_characters, 0);      // 设置支持中文字体
    lv_label_set_text_fmt(radio_label, "%s", HLS_list[0].program_name);
    /* IP地址 */
    lv_obj_t* ip_address_label = lv_label_create(internet_scr); // 从当前活动屏幕创建
    lv_obj_align(ip_address_label, LV_ALIGN_TOP_LEFT, 0, 44);
    lv_obj_set_width(ip_address_label, 104);
    lv_label_set_text(ip_address_label, "47.97.51.210");


    /* 当前日期时间 */
    data_time_label1= lv_label_create(internet_scr);  
    lv_obj_align(data_time_label1, LV_ALIGN_TOP_LEFT, 0, 55);
    lv_obj_set_width(data_time_label1, 104);   
    lv_label_set_text(data_time_label1, data_time_label);

    /*声音图标*/
    int_sound_icon=lv_label_create(internet_scr);  
    lv_obj_align(int_sound_icon, LV_ALIGN_BOTTOM_LEFT, 104, 0);
    lv_obj_set_width(int_sound_icon, 23);
    lv_obj_set_style_text_font(int_sound_icon, &lv_font_montserrat_20, 0);
    lv_label_set_text(int_sound_icon,LV_SYMBOL_VOLUME_MAX);


}

static void FM_radio_scr_create()
{
    FM_scr = lv_obj_create(NULL);    //创建屏幕


    /*
     * 创建label
     */
    /* 当前频段 */
    fre_label = lv_label_create(FM_scr);
    lv_obj_align(fre_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(fre_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)rda5807m_current_fre/1000));

    /* 当前信号RSSI */
    rssi_label = lv_label_create(FM_scr);
    lv_obj_align(rssi_label, LV_ALIGN_TOP_LEFT, 0, 21);
    lv_obj_set_style_text_font(rssi_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(rssi_label, "RSSI:0");
    /* 当前日期时间 */
    data_time_label2 = lv_label_create(FM_scr);
    lv_obj_align(data_time_label2, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(data_time_label2, &lv_font_montserrat_20, 0);
    lv_obj_set_width(data_time_label2, 104);
    lv_label_set_text(data_time_label2, data_time_label);

    /*声音图标*/
    fm_sound_icon=lv_label_create(FM_scr);  
    lv_obj_align(fm_sound_icon, LV_ALIGN_BOTTOM_LEFT, 104, 0);
    lv_obj_set_width(fm_sound_icon, 23);
    lv_obj_set_style_text_font(fm_sound_icon, &lv_font_montserrat_20, 0);
    lv_label_set_text(fm_sound_icon,LV_SYMBOL_VOLUME_MAX);
}


/**
 * @brief 更新标签内容 显示当前节目的名称、采样率等信息
 * 
 * @param music_info 
 */
void updata_radio_info_label(audio_element_info_t music_info)
{
    if (radio_info_label != NULL)
    {
        lv_label_set_text_fmt(radio_info_label, "Sample=%d, Bits=%d, Ch=%d", 
                          music_info.sample_rates, music_info.bits, music_info.channels);
    }
    
}



/**
 * @brief LVGL GUI程序
 * 
 */
void my_lvgl_app()
{
    internet_radio_scr_create();
    FM_radio_scr_create();
}





// /**
//  * @brief 开机加载动画显示
//  * 
//  */
void my_lvgl_load_anim(void *pvParameters)
{
     static uint8_t x=0;
    lv_obj_t* start_scr = lv_obj_create(NULL);    //创建屏幕
    lv_obj_t* symbol_label1 = lv_label_create(start_scr);
    lv_obj_t* symbol_label2 = lv_label_create(start_scr);
    lv_obj_set_style_text_font(symbol_label1, &lv_font_montserrat_20, 0);
    lv_obj_align(symbol_label1, LV_ALIGN_TOP_LEFT, 30, 8);
    lv_obj_align(symbol_label2, LV_ALIGN_BOTTOM_RIGHT, 0, -8);
    lv_label_set_text(symbol_label1, "START");
    lv_label_set_text(symbol_label2,">>>>>>>>>>>>>>>>>>>>>>>>");
    lv_scr_load(start_scr);
    while(1)
    {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        lv_obj_set_pos(symbol_label2,x+=1,-8);
        if(x%20==0) {
            lv_label_set_text(symbol_label1, "START");
        }
        else if(x%20==5){
            lv_label_set_text(symbol_label1, "START.");
        }
        else if(x%20==10){
            lv_label_set_text(symbol_label1, "START..");
        }       
        else if(x%20==15){
            lv_label_set_text(symbol_label1, "START...");
        }
        if(x>40)
        x=0;
    }
    
}

void SSD1306_init()
{
    scr_driver_t g_lcd;         // A screen driver
    esp_err_t ret = ESP_OK;

    spi_bus_handle_t bus_handle = NULL;
    spi_config_t bus_conf = {
        .miso_io_num = -1,      // 不用设置为-1
        .mosi_io_num = 35,      // LCD_SDA
        .sclk_io_num = 36,      // LCD_SCK
    }; // spi_bus configurations
    bus_handle = spi_bus_create(SPI2_HOST, &bus_conf);
    
    scr_interface_spi_config_t spi_ssd1306_cfg = {
        .spi_bus = bus_handle,    /*!< Handle of spi bus */
        .pin_num_cs = 38,           /*!< SPI Chip Select Pin 找个不用的引脚 */
        .pin_num_dc = 33,           /*!< Pin to select Data or Command for LCD */
        .clk_freq = 80*1000*1000,   /*!< SPI clock frequency */
        .swap_data = false,          /*!< Whether to swap data */
    };
    
    scr_interface_driver_t *iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &spi_ssd1306_cfg, &iface_drv);
    /** Find screen driver for SSD1306 */
    ret = scr_find_driver(SCREEN_CONTROLLER_SSD1306, &g_lcd);
    if (ESP_OK != ret) {
        return;
        ESP_LOGE(TAG, "screen find failed");
    }
    
    /** Configure screen controller */
    scr_controller_config_t lcd_cfg = {
        .interface_drv = iface_drv,
        .pin_num_rst = 34,      // The reset pin is 34
        .pin_num_bckl = -1,     // The backlight pin is not connected
        .rst_active_level = 0,
        .bckl_active_level = 1,
        .offset_hor = 0,
        .offset_ver = 0,
        .width = 128,
        .height = 64,
        .rotate = SCR_DIR_RLBT,
    };

    /** Initialize SSD1306 screen */
    g_lcd.init(&lcd_cfg);

    lvgl_init(&g_lcd, NULL);    /* Initialize LittlevGL */
}