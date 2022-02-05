#ifndef _COM_GUI_LVGL_H
#define _COM_GUI_LVGL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "sdkconfig.h"

#include "lvgl.h"
#include "lvgl_adapter.h"
#include "audio_element.h"
/**
 * @brief Initialize lvgl and register driver to lvgl
 * 
 * @note  This function will create a task to run the lvgl handler. The task will always Pinned on the APP_CPU
 * 
 * @param lcd_drv Pointer of lcd driver
 * @param touch_drv  Pointer of touch driver. If you don't have a touch panel, set it to NULL
 * 
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_TIMEOUT Operation timeout
 */
esp_err_t lvgl_init(scr_driver_t *lcd_drv, touch_panel_driver_t *touch_drv);

/**
 * @brief SSD1306 SPI初始化
 * 
 */
 void SSD1306_init(void);

/**
 * @brief 更新标签内容 显示当前节目的名称、采样率等信息
 * 
 * @param music_info 
 */
void updata_radio_info_label(audio_element_info_t music_info);
extern lv_obj_t* radio_label;        // 网络节目名称标签
extern lv_obj_t* internet_scr;            // 网络收音机屏幕
extern lv_obj_t* FM_scr;                  // FM收音机模式
extern lv_obj_t* fre_label;          // 当前频段标签
extern TaskHandle_t updata_rda5807m_info_task_handle;   // 更新RDA5807M信息任务句柄
extern lv_obj_t* data_time_label1;          // 当前时间标签
extern lv_obj_t* data_time_label2;
extern lv_obj_t* int_sound_icon;//网络收音机页面下声音图标
extern lv_obj_t* fm_sound_icon;//FM收音机页面下声音图标

/**
 * @brief 开机加载动画显示
 * 
 */
void my_lvgl_load_anim(void *pvParameters);

/**
 * @brief LVGL GUI程序
 * 
 */
void my_lvgl_app();
/**
 * @brief 更新rda5807m信息的函数
 * 
 */
void updata_rda5807m_info_task(void *pvParameters);

#ifdef __cplusplus
}
#endif



#endif /* _COM_GUI_LVGL_H */