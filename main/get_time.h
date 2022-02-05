#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_gui.h"

extern TaskHandle_t anim_xHandle;//开机动画任务句柄
extern lv_obj_t* display_time_scr;
extern char data_time_label[9];
extern uint32_t num_time;

/**
 * @brief 更新时间与LED指示灯线程
 * 
 */
void time_task(void *pvParameters);

/**
 * @brief 初始化SNTP并获取时间
 * 
 */
void obtain_time(void);

