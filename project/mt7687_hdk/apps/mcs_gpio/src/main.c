#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "os.h"
#include "net_init.h"
#include "network_init.h"
#include "wifi_api.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ethernetif.h"
#include "lwip/sockets.h"
#include "netif/etharp.h"
#include "mcs.h"

/* gpio */
#include "hal_gpio.h"

#define GPIO_ON "switch,1"
#define GPIO_OFF "switch,0"
int startSmart = 0;

void wifi_connect_init(void *args)
{
    LOG_I(common, "enter connect init.");
    uint8_t opmode  = WIFI_MODE_STA_ONLY;
    uint8_t port = WIFI_PORT_STA;

    /* ssid */
    char ssid[15];
    int nvdm_ssid_len = sizeof(ssid);
    nvdm_read_data_item("STA", "Ssid", (uint8_t *)ssid, (uint32_t *)&nvdm_ssid_len);

    /* password */
    char pwd[15];
    int nvdm_pwd_len = sizeof(pwd);
    nvdm_read_data_item("STA", "Password", (uint8_t *)pwd, (uint32_t *)&nvdm_pwd_len);

    // nvram set STA Ssid mcs
    // nvram set STA password mcs12345678

    wifi_auth_mode_t auth = WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK;
    wifi_encrypt_type_t encrypt = WIFI_ENCRYPT_TYPE_TKIP_AES_MIX;

    uint8_t nv_opmode;

    if (wifi_config_init() == 0) {
        wifi_config_get_opmode(&nv_opmode);
        if (nv_opmode != opmode) {
            wifi_config_set_opmode(opmode);
        }
        wifi_config_set_ssid(port, ssid ,strlen(ssid));
        wifi_config_set_security_mode(port, auth, encrypt);
        wifi_config_set_wpa_psk_key(port, pwd, strlen(pwd));
        wifi_config_reload_setting();

        network_dhcp_start(opmode);
    }
    vTaskDelete(NULL);
}

void mcs_mqtt_callback(char *rcv_buf) {

    int pin = 35;

    hal_pinmux_set_function(pin, 8);

    hal_gpio_status_t ret;
    ret = hal_gpio_init(pin);
    ret = hal_gpio_set_direction(pin, HAL_GPIO_DIRECTION_OUTPUT);

    if (NULL != strstr(rcv_buf, GPIO_ON)) {
        ret = hal_gpio_set_output(pin, 1);
    } else if (NULL != strstr(rcv_buf, GPIO_OFF)) {
        ret = hal_gpio_set_output(pin, 0);
    }
    ret = hal_gpio_deinit(pin);
    printf("rcv_buf: %s\n", rcv_buf);
}

void wifi_connected_task(void *parameter) {
    char data_buf [100] = {0};
    strcat(data_buf, "status");
    strcat(data_buf, ",,connect wifi");
    mcs_upload_datapoint(data_buf);

    mcs_mqtt_init(mcs_mqtt_callback);
    for (;;) {
        ;
    }
}

void wifi_connected_init(const struct netif *netif) {
  xTaskCreate(wifi_connected_task, "wifiConnectedTask", 8048, NULL, 10, NULL);
}

void gpio_init () {
    hal_pinmux_set_function(HAL_GPIO_0, 8);
    hal_gpio_data_t data_pull_up;
    hal_gpio_data_t data_pull_down;
    hal_gpio_status_t ret;
    hal_pinmux_status_t ret_pinmux_status;

    ret = hal_gpio_init(HAL_GPIO_0);

    /* set pin to work in GPIO mode.*/
    ret_pinmux_status = hal_pinmux_set_function(HAL_GPIO_0, 8);

    /* set direction of GPIO is input.*/
    ret = hal_gpio_set_direction(HAL_GPIO_0, HAL_GPIO_DIRECTION_INPUT);

    /* configure the pull state to pull-up.*/
    ret = hal_gpio_pull_up(HAL_GPIO_0);

    /* get input data of the pin for further validation.*/
    ret = hal_gpio_get_input(HAL_GPIO_0, &data_pull_up);

    if (1 == data_pull_up && 0 == startSmart) {
        startSmart = 1;
        char param[0] = "connect\0";
        _smart_config_test(1, param);
    }

    /* configure the pull state to pull-down.*/
    ret = hal_gpio_pull_down(HAL_GPIO_0);

    /* get input data of the pin for further validation.*/
    ret = hal_gpio_get_input(HAL_GPIO_0, &data_pull_down);

    // printf("down: %d\n", data_pull_down);
    // printf("up: %d\n", data_pull_up);

    ret = hal_gpio_deinit(HAL_GPIO_0);

}

void gpio_task(void *parameter) {
  for (;;) {
    gpio_init();
    vTaskDelay(200);
  }
}

int main(void)
{
    system_init();
    wifi_register_ip_ready_callback(wifi_connected_init);
    network_init();
    xTaskCreate(wifi_connect_init, "wifiConnect", 1024, NULL, 3, NULL);
    smart_config_if_enabled();
    vTaskStartScheduler();
    while (1) {
    }
    return 0;
}


