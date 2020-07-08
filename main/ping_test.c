#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "ping/ping.h"
#include "esp_ping.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "ping_test.h"

#define BUF_SIZE (1024)

static const char *TAG = "Ping_test";

static EventGroupHandle_t s_internet_conn_group = NULL;

static uint32_t s_ping_count   = 2;   //how many pings per report
static uint32_t s_ping_timeout = 5000; //mS till we consider it timed out
static uint32_t s_ping_delay = 500;    //mS between pings

static esp_err_t ping_results(ping_target_id_t msgType, esp_ping_found * pf)
{
	ESP_LOGD(TAG,"AvgTime:%.1fmS Sent:%d Rec:%d Err:%d min(mS):%d max(mS):%d ", (float)pf->total_time/pf->recv_count, pf->send_count, pf->recv_count, pf->err_count, pf->min_time, pf->max_time );
	ESP_LOGD(TAG,"Resp(mS):%d Timeouts:%d Total Time:%d\n",pf->resp_time, pf->timeout_count, pf->total_time);

    if (pf->send_count == s_ping_count) {
        if (pf->recv_count == s_ping_count) {
            xEventGroupSetBits(s_internet_conn_group, BIT0);
        } else {
            xEventGroupSetBits(s_internet_conn_group, BIT1);
        }

    }

	
	return ESP_OK;
}

bool run_ping_test() {
    if (s_internet_conn_group == NULL) {
        s_internet_conn_group = xEventGroupCreate();
    }

    ip4_addr_t target_ip;
    IP4_ADDR(&target_ip, 8, 8, 8, 8);
    xEventGroupClearBits(s_internet_conn_group, BIT0 | BIT1);
    	
    esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &s_ping_count, sizeof(uint32_t));
    esp_ping_set_target(PING_TARGET_RCV_TIMEO, &s_ping_timeout, sizeof(uint32_t));
    esp_ping_set_target(PING_TARGET_DELAY_TIME, &s_ping_delay, sizeof(uint32_t));
    esp_ping_set_target(PING_TARGET_IP_ADDRESS, &target_ip.addr, sizeof(uint32_t));
    esp_ping_set_target(PING_TARGET_RES_FN, (void*)(&ping_results), 0);

    ESP_LOGD(TAG,"Pinging well-known IP %s ...",inet_ntoa(target_ip));
    ping_init();
        
    EventBits_t bits = xEventGroupWaitBits(s_internet_conn_group, BIT0 | BIT1, true, false, portMAX_DELAY);

    bool result;
    if ((bits & BIT0) != 0) {
        ESP_LOGI(TAG, "ping test successful");
        result = true;
    } else {
        ESP_LOGE(TAG, "ping test failed");
        result = false;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ping_deinit();

    return result;
}
