
#include "play_living_stream.h"

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

static const char *TAG = "PLAY_LIVING_STREAM";
uint8_t HLS_list_index = 0;
    HLS_INFO_t HLS_list[MAX_HLS_URL_NUM] = {
    {.hls_url = "http://open.ls.qingting.fm/live/386/64k.m3u8?format=aac", .program_name = "CNR中国之声"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/4985/64k.m3u8?format=aac", .program_name = "CNR中国交通广播"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/274/64k.m3u8?format=aac", .program_name = "上海动感101"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/4804/64k.m3u8?format=aac", .program_name = "怀集音乐之声"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/1005/64k.m3u8?format=aac", .program_name = "CRI环球资讯"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/387/64k.m3u8?format=aac", .program_name = "CNR经济之声"},  
    {.hls_url = "http://open.ls.qingting.fm/live/398/64k.m3u8?format=aac", .program_name = "阅读之声"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/1006/64k.m3u8?format=aac", .program_name = "轻松调频EZFM"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/5022405/64k.m3u8?format=aac", .program_name = "亚洲音乐台"},  
    {.hls_url = "http://open.ls.qingting.fm/live/20091/64k.m3u8?format=aac", .program_name = "中国校园之声"},  
    {.hls_url = "http://open.ls.qingting.fm/live/4900/64k.m3u8?format=aac", .program_name = "太原交通广播"},  
    {.hls_url = "http://open.ls.qingting.fm/live/20006/64k.m3u8?format=aac", .program_name = "太原综合广播"},  
    {.hls_url = "http://open.ls.qingting.fm/live/1654/64k.m3u8?format=aac", .program_name = "石家庄音乐广播"}, 
    {.hls_url = "http://open.ls.qingting.fm/live/20485/64k.m3u8?format=aac", .program_name = "山西文艺广播"},
    {.hls_url = "http://open.ls.qingting.fm/live/4900/64k.m3u8?format=aac", .program_name = "太原交通广播"},
    {.hls_url = "http://open.ls.qingting.fm/live/5022396/64k.m3u8?format=aac", .program_name = "大同交通广播"},
    {.hls_url = "http://open.ls.qingting.fm/live/15318393/64k.m3u8?format=aac", .program_name = "河南电台网络戏曲广播"},
    {.hls_url = "http://open.ls.qingting.fm/live/1649/64k.m3u8?format=aac", .program_name = "河北音乐广播"},  
    {.hls_url = "http://open.ls.qingting.fm/live/4932/64k.m3u8?format=aac", .program_name = "山西音乐广播"},  
    {.hls_url = "http://open.ls.qingting.fm/live/1007/64k.m3u8?format=aac", .program_name = "CRI HIT FM"},  
};

audio_pipeline_handle_t pipeline;
audio_element_handle_t http_stream_reader, output_stream_writer, aac_decoder;
audio_event_iface_handle_t evt;
esp_periph_set_handle_t set;

static int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    if (msg->event_id == HTTP_STREAM_RESOLVE_ALL_TRACKS) {
        return ESP_OK;
    }

    if (msg->event_id == HTTP_STREAM_FINISH_TRACK) {
        return http_stream_next_track(msg->el);
    }
    if (msg->event_id == HTTP_STREAM_FINISH_PLAYLIST) {
        return http_stream_fetch_again(msg->el);
    }
    return ESP_OK;
}

static void living_stream_task()
{
     audio_event_iface_msg_t msg;
    while (1) {
       
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) aac_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(aac_decoder, &music_info);
            ESP_LOGI(TAG, "Receive music info from aac decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);
            updata_radio_info_label(music_info);       
            audio_element_setinfo(output_stream_writer, &music_info);
            pwm_stream_set_clk(output_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        /* restart stream when the first pipeline element (http_stream_reader in this case) receives stop event (caused by reading errors) */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) http_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_ERROR_OPEN) {
            ESP_LOGW(TAG, "Restart stream");
            audio_pipeline_stop(pipeline);
            audio_pipeline_wait_for_stop(pipeline);
            audio_element_reset_state(aac_decoder);
            audio_element_reset_state(output_stream_writer);
            audio_pipeline_reset_ringbuffer(pipeline);
            audio_pipeline_reset_items_state(pipeline);
            audio_pipeline_run(pipeline);
            continue;
        }
    }
}

// static void event_handler(void *arg, esp_event_base_t event_base,
//                           int32_t event_id, void *event_data){
//     if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ESP_LOGI(TAG, "Notification of a wifi connect");
//     }
// }

void wifi_connect()
{
    ESP_LOGI(TAG, "Start and wait for Wi-Fi network");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    set = esp_periph_set_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
   // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);
}

 //audio_board_handle_t board_handle={0};
void play_living_stream_start(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    
    ESP_LOGI(TAG, "[1.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[1.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.event_handle = _http_stream_event_handle;
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.enable_playlist_parser = true;
    http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(TAG, "[2.2] Create PWM stream to write data to codec chip");
    pwm_stream_cfg_t pwm_cfg = PWM_STREAM_CFG_DEFAULT();
    pwm_cfg.pwm_config.gpio_num_left = 17;
    pwm_cfg.pwm_config.gpio_num_right = 18;
    output_stream_writer = pwm_stream_init(&pwm_cfg);


    ESP_LOGI(TAG, "[2.3] Create aac decoder to decode aac file");
    aac_decoder_cfg_t aac_cfg = DEFAULT_AAC_DECODER_CONFIG();
    aac_decoder = aac_decoder_init(&aac_cfg);

    ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, aac_decoder,        "aac");
    audio_pipeline_register(pipeline, output_stream_writer,  "output");

    ESP_LOGI(TAG, "[2.5] Link it together http_stream-->aac_decoder-->pwm_stream-->[codec_chip]");
    const char *link_tag[3] = {"http", "aac", "output"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[2.6] Set up  uri (http as http_stream, aac as aac decoder, and  output is  PWM)");
    audio_element_set_uri(http_stream_reader, HLS_list[0].hls_url);

    

    ESP_LOGI(TAG, "[ 3 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[3.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[3.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "[ 4 ] Start audio_pipeline");
   // audio_pipeline_run(pipeline);

    xTaskCreate(living_stream_task, "living_stream_task", 4096, NULL, 25, NULL);
    //play_living_stream_end();
    


}

void play_living_stream_end()
{
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
}

void play_living_stream_restart()
{
    //ESP_LOGW(TAG, "start living_stream, current URL: %s", HLS_list[HLS_list_index].hls_url);
    audio_element_set_uri(http_stream_reader, HLS_list[HLS_list_index].hls_url);          
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_run(pipeline);
}