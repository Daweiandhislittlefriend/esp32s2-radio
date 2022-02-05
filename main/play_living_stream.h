#pragma once 
#include "audio_element.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "pwm_stream.h"
#include "aac_decoder.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"




#include "lvgl_gui.h"

#define MAX_HLS_URL_NUM     (20)
typedef struct
{
    const char * hls_url;
    const char * program_name;
} HLS_INFO_t;

extern HLS_INFO_t HLS_list[MAX_HLS_URL_NUM];
extern uint8_t HLS_list_index;
extern audio_pipeline_handle_t pipeline;
extern audio_element_handle_t http_stream_reader, output_stream_writer, aac_decoder;
// extern audio_board_handle_t board_handle;
void wifi_connect(void);

/**
 * @brief 网络电台开始
 * 
 */
void play_living_stream_start(void);

/**
 * @brief 网络电台暂时结束，没有完全结束，仍占用部分资源
 * 
 */
void play_living_stream_end(void);

/**
 * @brief play_living_stream_end之后重新开始使用
 * 
 */
void play_living_stream_restart(void);