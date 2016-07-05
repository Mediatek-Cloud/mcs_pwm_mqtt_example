/****************************************************************************
 *
 * Header files.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <cli.h>
#include <toi.h>
#include <type_def.h>
#include <os.h>
#include <os_util.h>

#include <syslog_cli.h>

#if defined(MTK_BSPEXT_ENABLE)
#include "wifi_ex.h"
#endif

#ifdef MTK_HOMEKIT_ENABLE
#include "mfi_cli.h"
#include "wac_cli.h"
#include "HAP_test.h"
#include "mdns_cli.h"
#include "homekit_init.h"
#endif

#ifdef HALEX
#include "hal_ex_wdt.h"
#include "hal_ex_rtc.h"
#include "hal_ex_sys.h"
#include "hal_ex_irrx.h"
#include "hal_ex_uid.h"
#include "hal_ex_efuse.h"
#include "hal_ex_pcnt.h"
#include "spi_eeprom_app.h"
#include "hal_ex_adc.h"
#include "hal_ex_i2s.h"
#include "hal_ex_flash.h"
#include "hal_ex_irtx.h"
#include "hal_ex_i2c.h"
#include "hal_ex_gpio.h"
#endif

#include "inband_queue.h"
#include "connsys_util.h"

#include "wifi_scan.h"

#if defined(MTK_BSP_LOAD_WIFI_PROFILE_ENABLE)
#include "connsys_profile.h"
#include "get_profile_string.h"
#include "os.h"
#endif

#if defined(MTK_BSP_LOOPBACK_ENABLE)
#include "debug.h"
#endif

#include "lwip/netif.h"
#include "lwip/inet.h"

#include "dhcpd.h"

#define CACHE_API_TEST_CLI 0

#if defined(MTK_FOTA_ENABLE)
#include "fota_cli.h"
#endif

#include "os_cli.h"

#include "wifi_api.h"
#include "wifi_inband.h"
#include "wifi_inband_cli.h"
#if (CACHE_API_TEST_CLI == 1)
#include "hal_cache.h"
#endif

#include "hal_sys.h"
#include "connsys_driver.h"

#include "lwipopts.h"

#if defined(MTK_LWIP_ENABLE)
#include "lwip_cli.h"
#endif

#ifdef ENABLE_SENSOR
#include "sensor_cli.h"
#endif

#ifdef ENABLE_LIBLCD
#include "lcd_cli.h"
#endif

#include "nvdm.h"
#ifdef MTK_NVRAM_ENABLE
#include "nvram_cli.h"
#endif

#if defined(MTK_PING_OUT_ENABLE)
#include "ping.h"
#endif

#include "cli_def.h"
#include "io_def.h"


#ifdef __CC_ARM
#define MTK_FW_VERSION "Not supported by Keil"
#endif


/****************************************************************************
 * Types.
 ****************************************************************************/


/****************************************************************************
 * Forward Declarations.
 ****************************************************************************/


static uint8_t _sdk_cli_mem(uint8_t len, char *param[]);
static uint8_t _sdk_cli_reboot(uint8_t len, char *param[]);
static uint8_t _sdk_cli_ver(uint8_t len, char *param[]);


static uint8_t _cli_reg_r(uint8_t len, char *param[]);
static uint8_t _cli_reg_w(uint8_t len, char *param[]);

#if (CACHE_API_TEST_CLI == 1)
static uint8_t _cli_cache_set(uint8_t len, char *param[]);
static uint8_t _cli_cache_hit(uint8_t len, char *param[]);
static uint8_t _cli_cache_clr(uint8_t len, char *param[]);
#endif

static uint8_t _cli_show_lwip_stat(uint8_t len, char *param[]);



#if (CACHE_API_TEST_CLI == 1)
static cmd_t   _cli_cache_cmds[] = {
    { "set",          "0/8/16/32",              _cli_cache_set },
    { "rate",         "show hit rate",          _cli_cache_hit },
    { "clr",          "reset hit rate",         _cli_cache_clr },
    { NULL,           NULL,                     NULL           }
};
#endif


static cmd_t syslog_cli[] = {
    { "set",    "setup filter",  syslog_cli_set_filter,   NULL },
    { NULL,     NULL,            NULL,                    NULL }
};


static cmd_t   os_cli[] = {
    { "1",      "show FreeRtos task",   os_cli_show_task_info  },
    { "2",      "show cpu utilization", os_cli_cpu_utilization },
    { NULL }
};

#if defined(MTK_FOTA_ENABLE)
#define FOTA_CLI_ENTRY      { "fota", "storage mgmt", NULL, fota_cli },
#else
#define FOTA_CLI_ENTRY
#endif

#ifdef MTK_MINISUPP_ENABLE
#define INBAND_CLI_ENTRY    { "inband", "inband channel", NULL, (cmd_t *)inband_cmds },
#else
#define INBAND_CLI_ENTRY
#endif

#if defined(MTK_LWIP_ENABLE)
#define LWIP_CLI_ENTRY  { "l", "lwip",       NULL,        lwip_cli }, \
                        { "ip",   "ip config",  lwip_ip_cli, NULL     },
#else
#define LWIP_CLI_ENTRY
#endif

#ifdef ENABLE_SENSOR
#define SENSOR_CLI_ENTRY { "sensor", "sensor demo", NULL, sensor_cli },
#else
#define SENSOR_CLI_ENTRY
#endif

#ifdef ENABLE_LIBLCD
#define LIBLCD_CLI_ENTRY { "lcd", "lcd demo", NULL, liblcd_cli },
#else
#define LIBLCD_CLI_ENTRY
#endif

#ifdef MTK_HOMEKIT_ENABLE
#define MOD_HOMEKIT_CLI_ENTRY   MOD_MFI_CLI_ENTRY       \
                                MOD_WAC_CLI_ENTRY       \
                                MOD_HAP_CLI_ENTRY       \
                                MOD_MDNS_CLI_ENTRY
#else
#define MOD_HOMEKIT_CLI_ENTRY
#endif

#if defined(MTK_BSPEXT_ENABLE) && defined(MTK_HAL_LOWPOWER_ENABLE)
#define LP_CLI_ENTRY { "lp",  "low power",  NULL, lp_cli },
#else
#define LP_CLI_ENTRY
#endif

#ifdef HALEX
#define HAL_EX_ENTRY    { "wdt",   "wdt",   NULL, wdt_cli   }, \
                        { "sys",   "sys",   NULL, sys_cli   }, \
                        { "rtc",   "rtc",   NULL, rtc_cli   }, \
                        { "irrx",  "irrx",  NULL, irrx_cli  }, \
                        { "efuse", "efuse", NULL, efuse_cli }, \
                        { "pcnt",  "pcnt",  NULL, pcnt_cli  }, \
                        { "uid",   "uid",   NULL, uid_cli   }, \
                        { "spim",  "spim",  NULL, spim_cli  }, \
                        { "adc",   "adc",   NULL, adc_cli   }, \
                        { "i2s",   "i2s",   NULL, i2s_cli   }, \
                        { "flash", "flash", NULL, flash_cli }, \
                        { "irtx",  "irtx",  NULL, irtx_cli  }, \
                        { "i2c",   "i2c",   NULL, i2c_cli   }, \
                        { "gpio",  "gpio",  NULL, gpio_cli  },
#else
#define HAL_EX_ENTRY
#endif

#if defined(MTK_NVRAM_ENABLE)
#define NVRAM_CLI_ENTRY { "nvram", "nvram cmd", _nvram_usage, nvram_cli},
#else
#define NVRAM_CLI_ENTRY
#endif

#define SYSLOG_CLI_ENTRY    { "syslog", "syslog control", syslog_cli_show_config, syslog_cli },

#define OS_CLI_ENTRY        { "os", "os information", NULL, os_cli },

#define TEST_REG_CLI_ENTRY  { "r",            "r",                      _cli_reg_r    }, \
                            { "w",            "w",                      _cli_reg_w    },


#if (CACHE_API_TEST_CLI == 1)
#define TEST_CACHE_CLI_ENTRY { "cache",       "cache", NULL,    _cli_cache_cmds },
#else
#define TEST_CACHE_CLI_ENTRY
#endif

#define TEST_LWIP_CLI_ENTRY { "4", "show lwip stat",  _cli_show_lwip_stat, NULL },

#define SDK_CLI_ENTRY { "mem",    "memory status", _sdk_cli_mem    }, \
                      { "reboot", "reboot",        _sdk_cli_reboot }, \
                      { "ver",    "f/w ver",       _sdk_cli_ver    },

#if !defined(CLI_DISABLE_LINE_EDIT) && !defined(CLI_DISABLE_HISTORY)
#define HISTORY_LINE_MAX    (128)
#define HISTORY_LINES       (5)
#endif /* !CLI_DISABLE_LINE_EDIT && !CLI_DISABLE_HISTORY */


/****************************************************************************
 * Local Data
 ****************************************************************************/


#if !defined(CLI_DISABLE_LINE_EDIT) && !defined(CLI_DISABLE_HISTORY)
static char s_history_lines[HISTORY_LINES][HISTORY_LINE_MAX];
static char *s_history_ptrs[HISTORY_LINES];

static char s_history_input[HISTORY_LINE_MAX];
static char s_history_parse_token[HISTORY_LINE_MAX];
#endif /* !CLI_DISABLE_LINE_EDIT && !CLI_DISABLE_HISTORY */


static cmd_t   cmds[] = {
    HAL_EX_ENTRY
    FOTA_CLI_ENTRY
    INBAND_CLI_ENTRY
    LWIP_CLI_ENTRY
    SENSOR_CLI_ENTRY
    LIBLCD_CLI_ENTRY
#if defined(MTK_BSPEXT_ENABLE)
    WIFI_CLI_ALL_ENTRY
    LP_CLI_ENTRY
#endif /* MTK_BSPEXT_ENABLE */
    MOD_HOMEKIT_CLI_ENTRY
    TEST_REG_CLI_ENTRY
    SYSLOG_CLI_ENTRY
    OS_CLI_ENTRY
    TEST_CACHE_CLI_ENTRY
    TEST_LWIP_CLI_ENTRY
    NVRAM_CLI_ENTRY
    SDK_CLI_ENTRY
    { NULL, NULL, NULL, NULL }
};


GETCHAR_PROTOTYPE;
PUTCHAR_PROTOTYPE;


static cli_t    _cli_cb = {
    .state  = 1,
    .echo   = 0,
    .cmd    = &cmds[0],
    .get    = __io_getchar,
    .put    = __io_putchar,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint8_t _sdk_cli_mem(uint8_t len, char *param[])
{
    printf("Show memory info\n");
    printf("FreeRTOS heap:\n");
    printf("\tTotal: %u\n", configTOTAL_HEAP_SIZE);
    printf("\tCurrent Free: %u\n", xPortGetFreeHeapSize());
    printf("\tMinimum Free: %u\n", xPortGetMinimumEverFreeHeapSize());
    return 0;
}


static uint8_t _sdk_cli_ver(uint8_t len, char *param[])
{
    char fw_ver[32];
    char patch_ver[32];

    os_memset(fw_ver, 0, 32);
    os_memset(patch_ver, 0, 32);

    cli_puts("CM4 Image Ver: ");
    cli_puts(MTK_FW_VERSION);
    cli_putln();

    connsys_get_n9_fw_ver(fw_ver);
    connsys_get_patch_ver(patch_ver);

    cli_puts("N9 Image  Ver: ");
    cli_puts(fw_ver);
    cli_putln();

    cli_puts("HW Patch  Ver: %s\n");
    cli_puts(patch_ver);
    cli_putln();

    return 0;
}


static uint8_t _sdk_cli_reboot(uint8_t len, char *param[])
{
    cli_puts("Reboot Bye Bye Bye!!!!\n");

    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);

    return 0;
}


#if (CACHE_API_TEST_CLI == 1)
static uint8_t _cli_cache_set(uint8_t len, char *param[])
{
    uint8_t  type;
    uint32_t value;

    if (len != 1) {
        printf("need cache size (KB)\n");
        return 0;
    }

    value = toi(param[0], &type);

    if (type == TOI_ERR) {
        printf("not a valid number\n");
        return 0;
    }

    switch (value) {
        case 0:
            hal_cache_enable(ENABLE_CACHE_0KB);
            break;
        case 8:
            hal_cache_enable(ENABLE_CACHE_8KB);
            break;
        case 16:
            hal_cache_enable(ENABLE_CACHE_16KB);
            break;
        case 32:
            hal_cache_enable(ENABLE_CACHE_32KB);
            break;
        default:
            printf("invalid cache size\n");
    }

    return 0;
}

static uint8_t _cli_cache_hit(uint8_t len, char *param[])
{
    printf("cache hit rate: %u\n", (unsigned int)hal_cache_get_hit_rate());

    return 0;
}

static uint8_t _cli_cache_clr(uint8_t len, char *param[])
{
    if (hal_cache_clear_count() < 0) {
        printf("clear cache hit count failed\n");
    }

    return 0;
}
#endif


static uint8_t _cli_reg_r(uint8_t len, char *param[])
{
    uint32_t reg;
    uint32_t val;
    uint8_t  type;

    if (len != 1) {
        printf("reg#\n");
        return 0;
    }

    reg = toi(param[0], &type);

    if (type == TOI_ERR) {
        printf("reg#\n");
    } else {
        val = *((volatile uint32_t *)reg);
        printf("read register 0x%08x (%u) got 0x%08x\n", (unsigned int)reg, (unsigned int)reg, (unsigned int)val);
    }

    return 0;
}

static uint8_t _cli_reg_w(uint8_t len, char *param[])
{
    uint32_t reg;
    uint32_t val;
    uint8_t  type;

    if (len == 2) {
        reg = toi(param[0], &type);
        if (type == TOI_ERR) {
            printf("reg#\n");
            return 0;
        }
        val = toi(param[1], &type);
        if (type == TOI_ERR) {
            printf("val#\n");
            return 0;
        }

        *((volatile uint32_t *)reg) = val;
        printf("written register 0x%08x (%u) as 0x%08x\n", (unsigned int)reg, (unsigned int)reg, (unsigned int)val);
    }

    return 0;
}

#if (LWIP_STATS == 1)
#if (LWIP_STATS_DISPLAY == 1)
#include <lwip/stats.h>
#endif
#endif

static uint8_t _cli_show_lwip_stat(uint8_t len, char *param[])
{
#if (LWIP_STATS == 1)
#if (LWIP_STATS_DISPLAY == 1)
    stats_display();
#else
    printf("LWIP_STATS_DISPLAY is not compiled\n ");
#endif /* (LWIP_STATS_DISPLAY == 1) */
#else
    printf("LWIP_STATS is not compiled\n ");
#endif /* (LWIP_STATS == 1) */

    return 0;
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

void cli_def_create(void)
{
#if !defined(CLI_DISABLE_LINE_EDIT) && !defined(CLI_DISABLE_HISTORY)
    cli_history_t *hist = &_cli_cb.history;
    int i;

    for (i = 0; i < HISTORY_LINES; i++) {
        s_history_ptrs[i] = s_history_lines[i];
    }
    hist->history           = &s_history_ptrs[0];

    hist->input             = s_history_input;
    hist->parse_token       = s_history_parse_token;
    hist->history_max       = HISTORY_LINES;
    hist->line_max          = HISTORY_LINE_MAX;
    hist->index             = 0;
    hist->position          = 0;
    hist->full              = 0;
#endif /* !CLI_DISABLE_LINE_EDIT && !CLI_DISABLE_HISTORY */

    cli_init(&_cli_cb);
}

void cli_def_task(void *param)
{
    while (1) {
        cli_task();
    }
}
