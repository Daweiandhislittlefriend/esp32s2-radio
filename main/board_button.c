#include "board_button.h"
#include "esp_log.h"
#include "get_time.h"
#include "play_living_stream.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "rda5807m_app.h"



#define GPIO_INPUT_IO_0    1
#define GPIO_INPUT_IO_1    2
#define GPIO_INPUT_IO_2    3
#define GPIO_INPUT_IO_3    6

static const char * TAG = "board_button";

button_handle_t g_buttons[BUTTON_NUM] = {0};



static uint8_t cnt=0;

static void button_num_0_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        if (lv_scr_act() == display_time_scr)
        {       
            lv_scr_load_anim(internet_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 400, 0, false);
            play_living_stream_restart();
            ESP_LOGI(TAG, "changed to internet_scr\n");
        }
        else if (lv_scr_act() == internet_scr&&cnt%2==0)
        {
            /* 停止living_stream */
            play_living_stream_end();
            lv_scr_load_anim(FM_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 400, 0, false);
             xTaskCreate(updata_rda5807m_info_task, "updata_rda5807m_task", 2048, NULL, 0, &updata_rda5807m_info_task_handle); 
            // 模拟开关切换至FM输出
            gpio_set_level(GPIO_NUM_42, 0);
   
            ESP_LOGI(TAG, "changed to FM_scr\n");
        }
        else if (lv_scr_act() == FM_scr)
        {
            // 模拟开关切换至网络电台输出
            gpio_set_level(GPIO_NUM_42, 1);  
            lv_scr_load_anim(display_time_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 400, 0, false);
             vTaskDelete(updata_rda5807m_info_task_handle);   // 删除FM屏幕上RDA5807M信息更新任务
            ESP_LOGI(TAG, "changed to display_time_scr\n");
        }
    }
}

uint32_t num_button=0;
static void button_num_1_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        if (internet_scr == lv_scr_act()&&cnt%2==0)
        {
            // 当按下按键，且当前屏幕为my_scr1时，才能切换电台
            // 切换下一个电台URL  参考mp3_control
            HLS_list_index++;
            if (HLS_list_index == MAX_HLS_URL_NUM)
                HLS_list_index = 0;
            audio_pipeline_stop(pipeline);
            audio_pipeline_wait_for_stop(pipeline);
            audio_pipeline_reset_ringbuffer(pipeline);
            audio_pipeline_reset_elements(pipeline);
            audio_element_set_uri(http_stream_reader, HLS_list[HLS_list_index].hls_url);          
            audio_pipeline_run(pipeline);	
            lv_label_set_text_fmt(radio_label, "%s", HLS_list[HLS_list_index].program_name);
        }
        else if (FM_scr == lv_scr_act())
        {
            rda5807m_app_add_frequency(100);
            lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)rda5807m_current_fre/1000));
        }
    }
    else if (iot_button_get_event((button_handle_t)arg) == BUTTON_LONG_PRESS_HOLD)
    {

        if (FM_scr == lv_scr_act())
        {
            if(num_button!=num_time)
            {
                num_button=num_time;
                rda5807m_app_add_frequency(100);
                lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)rda5807m_current_fre/1000));
            }
            
           
        }
    }
    
}


static void button_num_2_cb(void *arg)
{
    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        if ( internet_scr == lv_scr_act()&&cnt%2==0)
        {
            // 切换上一个电台URL  参考mp3_control
            HLS_list_index--;
            if (HLS_list_index == 255)
                HLS_list_index = MAX_HLS_URL_NUM - 1;
            audio_pipeline_stop(pipeline);
            audio_pipeline_wait_for_stop(pipeline);
            audio_pipeline_reset_ringbuffer(pipeline);
            audio_pipeline_reset_elements(pipeline);
            audio_element_set_uri(http_stream_reader, HLS_list[HLS_list_index].hls_url);
            audio_pipeline_run(pipeline);
            lv_label_set_text_fmt(radio_label, "%s", HLS_list[HLS_list_index].program_name);
        }
        else if(FM_scr==lv_scr_act())
        {
            rda5807m_app_reduce_frequency(100);
            lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)rda5807m_current_fre/1000));

        }
       
    }
    else if (iot_button_get_event((button_handle_t)arg) == BUTTON_LONG_PRESS_HOLD)
    {
        if(FM_scr==lv_scr_act())//400ms减少一次
        {
            if(num_button!=num_time)
            {
                num_button=num_time;
                rda5807m_app_reduce_frequency(100);
                lv_label_set_text_fmt(fre_label, "%.1fMHz", ((float)rda5807m_current_fre/1000));
            }
        }

       
    }
    
}
static void button_num_3_cb(void *arg)
{
    static bool mute=0;

    if (iot_button_get_event((button_handle_t)arg) == BUTTON_SINGLE_CLICK)
    {
        if ( internet_scr == lv_scr_act())
        {
                if(cnt%2==0)
                {
                    
                    audio_pipeline_stop(pipeline);
                    audio_pipeline_wait_for_stop(pipeline);
                    lv_label_set_text(int_sound_icon,LV_SYMBOL_MUTE);           
                }
                else
                {
                    
                    audio_pipeline_reset_ringbuffer(pipeline);
                    audio_pipeline_reset_elements(pipeline);
                    audio_pipeline_run(pipeline);
                    lv_label_set_text(int_sound_icon,LV_SYMBOL_VOLUME_MAX);
                }
                cnt++; 
        }
        else if(FM_scr==lv_scr_act())
        {
            rda5807m_get_mute(&rda5807m_dev, &mute);
            if(mute==false)
            {
                rda5807m_set_mute(&rda5807m_dev, true);
                lv_label_set_text(fm_sound_icon,LV_SYMBOL_MUTE);
            }
            else 
            {
                rda5807m_set_mute(&rda5807m_dev, false);
                lv_label_set_text(fm_sound_icon,LV_SYMBOL_VOLUME_MAX);
            }
        }
    }
    
}
/**
 * @brief 独立按键初始化
 * 
 */
void board_button_init()
{
    int32_t button_gpio_port[BUTTON_NUM] = {GPIO_INPUT_IO_0, GPIO_INPUT_IO_1, GPIO_INPUT_IO_2, GPIO_INPUT_IO_3};
    // create gpio button
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = button_gpio_port[i],
            .active_level = 0,
            },
        };
        g_buttons[i] = iot_button_create(&gpio_btn_cfg);
        if(NULL == g_buttons[i]) 
            ESP_LOGE(TAG, "Button create failed g_buttons[%d]", i);
        else
            ESP_LOGI(TAG, "Button[%d] created", i);
    }

    iot_button_register_cb(g_buttons[0], BUTTON_SINGLE_CLICK, button_num_0_cb);

    iot_button_register_cb(g_buttons[1], BUTTON_SINGLE_CLICK, button_num_1_cb);
    iot_button_register_cb(g_buttons[1], BUTTON_LONG_PRESS_HOLD, button_num_1_cb);

    iot_button_register_cb(g_buttons[2], BUTTON_SINGLE_CLICK, button_num_2_cb);
    iot_button_register_cb(g_buttons[2], BUTTON_LONG_PRESS_HOLD, button_num_2_cb);

    iot_button_register_cb(g_buttons[3], BUTTON_SINGLE_CLICK, button_num_3_cb);
}
