#pragma once
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "lvgl_gui.h"
#include "play_living_stream.h"


#define BUTTON_NUM  4
/**
 * @brief 独立按键初始化
 * 
 */
void board_button_init(void);

