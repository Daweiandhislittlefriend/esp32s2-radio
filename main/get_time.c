#include <time.h>
#include <sys/time.h>
#include "get_time.h"
#include "esp_sntp.h"
#include <stdio.h>

#define LED_GPIO 16

const char * TAG = "GET_TIME";
TaskHandle_t anim_xHandle = NULL;//开机动画任务句柄

char data_time_label[9]={0};    
lv_obj_t* display_time_scr= NULL;
lv_obj_t* time_label1 = NULL;//显示年月日
lv_obj_t* time_label2 = NULL;//显示小时分钟秒
lv_obj_t* time_label3 = NULL;//显示星期
uint32_t num_time=0;
void time_task(void *pvParameters)
{

    time_t now = 0;
    struct tm timeinfo = { 0 };
    char strftime_buf[64];
    gpio_reset_pin(LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    setenv("TZ", "CST-8", 1);
    tzset();
    vTaskDelete( anim_xHandle ); //删除开机动画任务
    lv_scr_load(display_time_scr);
       
    while (1)
    {

        if(num_time%10==0)
        {
            time(&now);     // update 'now' variable with current time
            localtime_r(&now, &timeinfo);
            strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d#%H:%M:%S#%A", &timeinfo); //例2022-01-28#20:59:59#Friday
            //ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);  
            lv_label_set_text(time_label1, strtok(strftime_buf, "#"));//分割字符串
            strcpy(data_time_label,strtok(NULL, "#"));
            lv_label_set_text(time_label2,data_time_label);
            lv_label_set_text(data_time_label1, data_time_label);
            lv_label_set_text(data_time_label2, data_time_label);
            lv_label_set_text(time_label3, strtok(NULL, "#"));
        }
        num_time++;
        gpio_set_level(LED_GPIO,(num_time/10)%2);
        vTaskDelay(pdMS_TO_TICKS(100));
 

    }
      
}


void time_sync_notification_cb(struct timeval *tv)
{
    display_time_scr = lv_obj_create(NULL);    //创建屏幕
    time_label1 = lv_label_create(display_time_scr); //显示年月日
    time_label2 = lv_label_create(display_time_scr);//显示小时分钟秒
    time_label3 = lv_label_create(display_time_scr);//显示星期

    lv_obj_set_style_text_font(time_label1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_font(time_label2, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_font(time_label3, &lv_font_montserrat_20, 0);

    lv_obj_align(time_label1, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_align(time_label2, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_align(time_label3, LV_ALIGN_BOTTOM_MID, 0, 0);

    ESP_LOGI(TAG, "Notification of a time synchronization event,Start the time synchronization thread");
    /* sntp获取时间线程 */ 
    xTaskCreate(time_task, "time_task", 4096, NULL, 0, NULL);
}


static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);  // 设置回调函数
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}


void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    if(retry==10)
    esp_restart();
}