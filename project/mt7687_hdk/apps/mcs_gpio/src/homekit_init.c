
#if defined(MTK_HOMEKIT_ENABLE)
// ==== FreeRTOS ====

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/netif.h"
#include "lwip/autoip.h"
#include "lwip/dhcp.h"
#include "inet.h"
#include "wifi_api.h"
#include "wifi_scan.h"
#include "wifi_inband.h"
#include "ethernetif.h"

#include "hal_sys.h"
#include "nvdm.h"
#include "homekit_init.h"
#include "dns_sd.h"

TaskHandle_t mDNSTaskHandler = NULL;

static wifi_event_t evt;
static int sta_itf_stat = 0;
//static unsigned char* payload;
//static unsigned int len;
TaskHandle_t g_dhcp_network_changed_task_handle = NULL;
void dhcp_network_changed_task(void *p);

int sta_itf_stat_get(void)
{
    return sta_itf_stat;
}

static void sta_itf_stat_set(int done)
{
    sta_itf_stat = done;
}

int dhcp_network_changed_event_handler(wifi_event_t evt1, unsigned char *payload1, unsigned int len1)
{
    printf("Enter dhcp_network_changed_event_handler\n");
    evt = evt1;
    //payload = payload1;
    //len = len1;
    uint8_t err = xTaskCreate(dhcp_network_changed_task, (signed char *) "net_change_task", 5 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, &g_dhcp_network_changed_task_handle);
    if (err != pdPASS) {
        printf("ERROR: create mDNSResponder task  failed\n");
    }
    return err;

}

void dhcp_network_changed_task(void *p)
{
    printf("Enter dhcp_network_changed_event_handler task\n");

    uint8_t opmode = 0;
    wifi_config_get_opmode(&opmode);
    if (opmode == 2) {
        goto exit;
    }
    struct netif *sta_if_local = NULL;
    sta_if_local = netif_find("st2");
    struct dhcp *dhcp = sta_if_local->dhcp;

    sta_itf_stat_set(0);
    if (evt == WIFI_EVENT_IOT_DISCONNECTED && dhcp->autoip_coop_state == DHCP_AUTOIP_COOP_STATE_ON) {
        printf("WIFI_EVENT_IOT_DISCONNECTED and COOP_STATE_ON\n");
        netif_set_link_down(sta_if_local);
        netif_set_down(sta_if_local);
    } else if (evt == WIFI_EVENT_IOT_CONNECTED && dhcp->autoip_coop_state == DHCP_AUTOIP_COOP_STATE_ON) {
        ip6_addr_t mld_address;
        printf("WIFI_EVENT_IOT_CONNECTED and COOP_STATE_ON\n");

        uint8_t link_status = 0;
        while (!link_status) {
            wifi_connection_get_link_status(&link_status);
            if (!link_status) {
                printf("Waiting for STA link up...");
                vTaskDelay(2 * 1000 / portTICK_PERIOD_MS);
            }
        }

        netif_set_link_up(sta_if_local);
        netif_set_up(sta_if_local);
#if LWIP_IPV6
        netif_create_ip6_linklocal_address(sta_if_local, 1); // 1 use hwadr to create link local addr. 0, use hwaddr directly as interface ID.
        sta_if_local->ip6_autoconfig_enabled = 1;
        ip6_addr_set_solicitednode(&mld_address, netif_ip6_addr(sta_if_local, 0)->addr[3]);
        mld6_joingroup(netif_ip6_addr(sta_if_local, 0), &mld_address);
#endif
        do {
            printf("wait until got ip address\n");
            vTaskDelay(2 * 1000 / portTICK_PERIOD_MS);
            /*wait until got ip address*/
        } while (sta_if_local->dhcp->state != DHCP_STATE_BOUND && sta_if_local->autoip->state != AUTOIP_STATE_BOUND);
        sta_itf_stat_set(1);
        if (mDNSTaskHandler) {
            printf(" mDNSTaskHandler enter while () \n");
            // Update mDNS's IP
            mDNS_UpdateInterface();
            printf(" mDNSTaskHandler exit while () \n");
        }
        /*
        else{
            printf("ERROR: mdnsd has not been started yet\n");
        }
        */
    } else if (evt == WIFI_EVENT_IOT_CONNECTED && dhcp->autoip_coop_state == DHCP_AUTOIP_COOP_STATE_OFF /*&& dhcp->pcb == NULL*/) {
        ip6_addr_t mld_address;
        printf("WIFI_EVENT_IOT_CONNECTED and COOP_STATE_OFF\n");

        netif_set_link_up(sta_if_local);
        netif_set_up(sta_if_local);

#if LWIP_IPV6
        netif_create_ip6_linklocal_address(sta_if_local, 1); // 1 use hwadr to create link local addr. 0, use hwaddr directly as interface ID.
        sta_if_local->ip6_autoconfig_enabled = 1;
        ip6_addr_set_solicitednode(&mld_address, netif_ip6_addr(sta_if_local, 0)->addr[3]);
        mld6_joingroup(netif_ip6_addr(sta_if_local, 0), &mld_address);
#endif
/////////////////////////////////////////////////////////
        int err;
        char result[32];
        uint32_t size = sizeof(result);
        err = nvdm_read_data_item("STA", "IpMode", (uint8_t *)result, &size);
        if (err) {
            printf("ERROR: nvram read\n");
            goto exit;
        }

        if (os_strcmp(result, "dhcp") == 0) {
            printf("DHCP_AUTOIP_COOP_STATE_OFF sta dhcp\n");
            dhcp_start(sta_if_local);
            do {
                printf("wait until got ip address\n");
                vTaskDelay(2 * 1000 / portTICK_PERIOD_MS);
                /*wait until got ip address*/
            } while (sta_if_local->dhcp->state != DHCP_STATE_BOUND && sta_if_local->autoip->state != AUTOIP_STATE_BOUND);
            sta_itf_stat_set(1);

            if (mDNSTaskHandler) {
                printf("mDNSTaskHandler is true\n");
                // Update mDNS's IP
                mDNS_UpdateInterface();
            }
        } else {
            printf("DHCP_AUTOIP_COOP_STATE_OFF sta static\n");
            ip_addr_t   v4;

            size = sizeof(result);
            err = nvdm_read_data_item("STA", "IpAddr", (uint8_t *)result, &size);
            if (err) {
                printf("ERROR: nvram read\n");
                goto exit;
            }
            if (inet_aton(result, &v4) == 0) {
                printf("invalid address: %s\n", result);
                return 1;
            }
            netif_set_ipaddr(sta_if_local, &v4);

            size = sizeof(result);
            err = nvdm_read_data_item("STA", "IpNetmask", (uint8_t *)result, &size);
            if (err) {
                printf("ERROR: nvram read\n");
                goto exit;
            }
            if (inet_aton(result, &v4) == 0) {
                printf("invalid address: %s\n", result);
                return 1;
            }
            netif_set_netmask(sta_if_local, &v4);

            size = sizeof(result);
            err = nvdm_read_data_item("STA", "IpGateway", (uint8_t *)result, &size);
            if (err) {
                printf("ERROR: nvram read\n");
                goto exit;
            }
            if (inet_aton(result, &v4) == 0) {
                printf("invalid address: %s\n", result);
                return 1;
            }
            netif_set_gw(sta_if_local, &v4);

            sta_itf_stat_set(1);
        }
////////////////////////////////////////////////////
    } else {
        printf("Event %d and coop state %d  Not Handled by dhcp_network_changed_event_handler\n", evt, dhcp->autoip_coop_state);
    }

exit:
    g_dhcp_network_changed_task_handle = NULL;
    vTaskDelete(NULL);
}


static void homekit_mdnsd_task(void *p)
{
    printf("homekit mDNSResponder!\n");
    MainDaemonTask(0, NULL);
    vTaskDelete(NULL);
}

void homekit_mdnsd_task_stop(void)
{
    printf("Stop homekit mDNSResponder!\n");
    SetStopBit();
    mDNSTaskHandler = NULL;
    //vTaskDelete(NULL);
}

int homekit_mdnsd_task_start(size_t stack_size)
{
    if (xTaskCreate(homekit_mdnsd_task, "mdnsd", stack_size * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                    &mDNSTaskHandler) != pdPASS) {
        printf("Failed to create mdnsd task...\n");
        return pdFAIL;
    }
    return pdPASS;
}

int homekit_wac_reset(void)
{
    if (xTaskCreate(wacserver_reset_task, "wac3", 2 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                    NULL) != pdPASS) {
        printf("Failed to create WACServer task...\n");
        return pdFAIL;
    }
    return pdPASS;
}

void homekit_task(void *args)
{
    TaskHandle_t wac_task, hap_task;
    int err;
    char result[3] = {0};
    uint32_t size = sizeof(result);
    uint8_t opmode = 0;

    char homekit_delay[8] = {0};
    uint32_t homekit_delay_size = sizeof(homekit_delay);
    err = nvdm_read_data_item("common", "HOMEKIT_DELAY", homekit_delay, &homekit_delay_size);
    if (err < 0) {
        printf("ERROR: nvram read\n");
        goto exit;
    }
    uint32_t delay = strtol(homekit_delay, NULL, 10);

    vTaskDelay(2 * delay * 1000 / portTICK_PERIOD_MS);
    printf("homekit init...\n");

    do {
        wifi_config_get_opmode(&opmode);
    } while (!opmode);

    err = nvdm_read_data_item("common", "WACDONE", (uint8_t *)result, &size);
    if (err < 0) {
        printf("ERROR: nvram read\n");
        goto exit;
    }

    if (os_strcmp(result, "0") == 0 && opmode == 2) {
        printf("homekit mdnsd start...\n");
#if 0
        // mDNS start
        if (xTaskCreate(homekit_mdnsd_task, "mdnsd", 15 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                        &mDNSTaskHandler) != pdPASS) {
            printf("Failed to create mdnsd task...\n");
            goto exit;
        }
#else
        Setnetif_mode(1); //ap mode
        homekit_mdnsd_task_start(15);
#endif

        // WAC start
        printf("homekit wac start...\n");
        vTaskDelay(delay * 1000 / portTICK_PERIOD_MS);

        QueueHandle_t wac_status_queue;
        int wac_status = -2; //default status - timeout

        wac_status_queue = xQueueCreate(1, sizeof(int));
        if (wac_status_queue == 0) {
            printf("Failed to create wac_status Queue...\n");
        }

        while (wac_status != 1) {
            if (xTaskCreate(wacserver_start_task, "wac1", 1 * 1024 / sizeof(portSTACK_TYPE), &wac_status_queue, 1, \
                            &wac_task) != pdPASS) {
                printf("Failed to create WACServer task...\n");
                goto exit;
            }

            if (xQueueReceive(wac_status_queue, &(wac_status), 60 * 60 * 1000 / portTICK_PERIOD_MS) != pdTRUE) {
                printf("Timeout to communicate with WAC task...\n");
            }

            printf("wac_status = %d\n", wac_status);

            switch (wac_status) {
                case 1:
                    break;
                case -1:
                    printf("WAC Error. Restarting WAC Server...\n");
#if 1
                    wifi_config_get_opmode(&opmode);
                    if (opmode != 2) {
                        // WAC reset
                        printf("homekit wac reset...\n");
                        vTaskDelay(delay * 1000 / portTICK_PERIOD_MS);
                        homekit_wac_reset();

                        goto exit;
                    }
#endif
                    break;
                case -2:
                    printf("WAC Timeout! Please reboot system to start over.\n");
                    goto exit;
                case -3:
                    printf("WAC Stopped! Please reboot system to start over.\n");
                    goto exit;
                default:
                    printf("Unexpected state. System rebooting...\n");
                    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
            }
        }

        vQueueDelete(wac_status_queue);
        wac_status_queue = NULL;

        printf("homekit hap start...\n");
        // HAP start
        if (xTaskCreate(hap_test, "hap", 5 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                        &hap_task) != pdPASS) {
            printf("Failed to create HAP task...\n");
            goto exit;
        }

    } else if (os_strcmp(result, "1") == 0 && opmode == 1) {
        uint8_t link_status = 0;
        while (!link_status) {
            err = wifi_connection_get_link_status(&link_status);
            if (err) {
                printf("ERROR: Get Link Status: %d\n", err);
                goto exit;
            }
            if (!link_status) {
                printf("Waiting for STA link up...\n");
                vTaskDelay(2 * 1000 / portTICK_PERIOD_MS);
            }
        }

        struct netif *sta_if = netif_find("st2");
        if (!sta_if) {
            printf("ERROR: try finding interface st2.");
            goto exit;
        }

        do {
            vTaskDelay(2 * 1000 / portTICK_PERIOD_MS);
            /*wait until got ip address*/
        } while (sta_if->dhcp->state != DHCP_STATE_BOUND && sta_if->autoip->state != AUTOIP_STATE_BOUND);

        printf("homekit mdnsd start...\n");
#if 0
        // mDNS start
        if (xTaskCreate(homekit_mdnsd_task, "mdnsd", 10 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                        &mDNSTaskHandler) != pdPASS) {
            printf("Failed to create mdnsd task...\n");
            goto exit;
        }
#else
        Setnetif_mode(2);
        homekit_mdnsd_task_start(10);
#endif

        printf("homekit hap start...\n");
        vTaskDelay(delay * 1000 / portTICK_PERIOD_MS);
        // HAP start
        if (xTaskCreate(hap_test, "hap", 5 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                        &hap_task) != pdPASS) {
            printf("Failed to create HAP task...\n");
            goto exit;
        }
    } else {
        // HAP reset
        printf("homekit hap reset...\n");
        if (xTaskCreate(hap_reset, "hap", 5 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, \
                        &hap_task) != pdPASS) {
            printf("Failed to create HAP task...\n");
            goto exit;
        }

        printf("homekit wac reset...\n");
        vTaskDelay(delay * 1000 / portTICK_PERIOD_MS);
        // WAC reset
        homekit_wac_reset();
    }
exit:
    vTaskDelete(NULL);
}

int homekit_init()
{
    char homekit_auto_start[2];
    uint32_t homekit_auto_start_size = sizeof(homekit_auto_start);

    wifi_connection_register_event_notifier(WIFI_EVENT_IOT_DISCONNECTED, dhcp_network_changed_event_handler);
    wifi_connection_register_event_notifier(WIFI_EVENT_IOT_CONNECTED, dhcp_network_changed_event_handler);

    nvdm_read_data_item("common", "HOMEKIT_AUTO_START", homekit_auto_start, &homekit_auto_start_size);
    if (os_strcmp(homekit_auto_start, "1") == 0) {
        if (xTaskCreate(homekit_task, "hmkit", 2 * 1024 / sizeof(portSTACK_TYPE), NULL, 1, NULL) != pdPASS) {
            printf("Failed to create HomeKit task...\n");
            return -1;
        }
    }
    return 0;
}
#endif
