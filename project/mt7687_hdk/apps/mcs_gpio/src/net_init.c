#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <os.h>

#include <top.h>
#include <hal_gpio.h>
#include <syslog.h>

#define CFG_FPGA 0
#if defined(MTK_HAL_LOWPOWER_ENABLE)
#include <hal_lp.h>
#endif

#include <sys.h>
#include <connsys_driver.h>

#include "io_def.h"
#include "task_def.h"
#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif

#include <nvdm.h>
#ifdef MTK_NVRAM_ENABLE
#include <nvram_handler.h>
#endif

#include "net_init.h"

#if defined(MTK_SYS_TRNG_ENABLE)
#include <hal_trng.h>
#endif

#ifdef MTK_NVDM_ENABLE
extern void nvdm_port_check_default_value(void);
#endif

log_create_module(main, PRINT_LEVEL_ERROR);

#if !defined (MTK_DEBUG_LEVEL_NONE)
LOG_CONTROL_BLOCK_DECLARE(main);
LOG_CONTROL_BLOCK_DECLARE(common);
LOG_CONTROL_BLOCK_DECLARE(hal);
LOG_CONTROL_BLOCK_DECLARE(lwip);
LOG_CONTROL_BLOCK_DECLARE(minisupp);
LOG_CONTROL_BLOCK_DECLARE(inband);

log_control_block_t *syslog_control_blocks[] = {
    &LOG_CONTROL_BLOCK_SYMBOL(main),
    &LOG_CONTROL_BLOCK_SYMBOL(common),
    &LOG_CONTROL_BLOCK_SYMBOL(hal),
    &LOG_CONTROL_BLOCK_SYMBOL(lwip),
    &LOG_CONTROL_BLOCK_SYMBOL(minisupp),
    &LOG_CONTROL_BLOCK_SYMBOL(inband),
    NULL
};
#endif /* MTK_DEBUG_LEVEL_NONE */

static void SystemClock_Config(void)
{
    top_xtal_init();
}

/* CACHE Init */
#include "mt7637_cm4_hw_memmap.h"
#include "hal_cache.h"
hal_cache_region_config_t region_cfg_tbl[] = {
    /* cache_region_address, cache_region_size */
    { CM4_EXEC_IN_PLACE_BASE, 0x1000000}
    /* add cache regions below if you have any */
};

static int32_t cache_enable(hal_cache_size_t cache_size)
{
    uint8_t region, region_number;

    region_number = sizeof(region_cfg_tbl) / sizeof(region_cfg_tbl[0]);
    if (region_number > HAL_CACHE_REGION_MAX) {
        return -1;
    }
    /* If cache is enabled, flush and invalidate cache */
    hal_cache_init();
    hal_cache_set_size(cache_size);
    for (region = 0; region < region_number; region++) {
        hal_cache_region_config(region, &region_cfg_tbl[region]);
        hal_cache_region_enable(region);
    }
    for (; region < HAL_CACHE_REGION_MAX; region++) {
        hal_cache_region_disable(region);
    }
    hal_cache_enable();
    return 0;
}

static void prvSetupHardware(void)
{
#if defined(MTK_HAL_LOWPOWER_ENABLE)
    if (1 == hal_lp_get_wic_wakeup()) {
        /* N9 MUST be active for clock switch and pinmux config*/
        /* Wakeup N9 by connsys ownership */
        connsys_open();
        connsys_close();
    }
#endif

    if (cache_enable(HAL_CACHE_SIZE_32KB) < 0) {
        LOG_E(common, "cache enable failed");
    }

#if (CFG_FPGA == 0)
    /* Enable MCU clock to 192MHz */
    cmnCpuClkConfigureTo192M();

    /*Enable flash clock to 64MHz*/
    cmnSerialFlashClkConfTo64M();

#if !defined(MTK_MAIN_CONSOLE_UART2_ENABLE)
    /* Set Pinmux to UART0  */
    hal_pinmux_set_function(HAL_GPIO_0, 7);
    hal_pinmux_set_function(HAL_GPIO_1, 7);
    hal_pinmux_set_function(HAL_GPIO_2, 7);
    hal_pinmux_set_function(HAL_GPIO_3, 7);
#else
    /* Set Pinmux to N9 JTAG */
    hal_pinmux_set_function(HAL_GPIO_0, 0);
    hal_pinmux_set_function(HAL_GPIO_1, 0);
    hal_pinmux_set_function(HAL_GPIO_2, 0);
    hal_pinmux_set_function(HAL_GPIO_3, 0);
#endif

    /* UART 1 init. */
    hal_pinmux_set_function(HAL_GPIO_36, 7);
    hal_pinmux_set_function(HAL_GPIO_37, 7);
    hal_pinmux_set_function(HAL_GPIO_38, 7);
    hal_pinmux_set_function(HAL_GPIO_39, 7);
#endif /* CFG_FPGA = 0 */

    io_def_uart_init();
}

#if (CFG_INFO_TASK_EN == 1)
static void print_system(void)
{
    unsigned int total_seconds = (unsigned int)sys_now() / configTICK_RATE_HZ;
    unsigned int total_minutes = total_seconds / 60;

    printf("\n(%02d:%02d:%02d)", total_minutes / 60,
           total_minutes % 60,
           total_seconds % 60);

    printf(" %8u\n", (unsigned int)connsys_get_stat_int_count());
}

static void prvInfoTask(void *pvParameters)
{
    /* Loop */
    for (;;) {
        print_system();
        vTaskDelay(CFG_INFO_TASK_PERIOD * configTICK_RATE_HZ);
    }
}

static portBASE_TYPE info_task_init()
{
    return xTaskCreate(prvInfoTask,
                       INFO_TASK_NAME,
                       INFO_TASK_STACKSIZE,
                       NULL,
                       INFO_TASK_PRIO,
                       NULL);
}
#endif /* (CFG_INFO_TASK_EN == 1) */

#if !defined (MTK_DEBUG_LEVEL_NONE)

static char syslog_filter_buf[SYSLOG_FILTER_LEN] = {0};

static void syslog_config_save(const syslog_config_t *config)
{
    syslog_convert_filter_val2str((const log_control_block_t **)config->filters, syslog_filter_buf);
    nvdm_write_data_item("common", "syslog_filters", \
                         NVDM_DATA_ITEM_TYPE_STRING, (const uint8_t *)syslog_filter_buf, os_strlen(syslog_filter_buf));
}

static uint32_t syslog_config_load(syslog_config_t *config)
{
    uint32_t sz = SYSLOG_FILTER_LEN;

    nvdm_read_data_item("common", "syslog_filters", (uint8_t *)syslog_filter_buf, &sz);
    syslog_convert_filter_str2val(config->filters, syslog_filter_buf);

    return 0;
}

#endif /* MTK_DEBUG_LEVEL_NONE */

/**
 * Initialize C library random function using HAL TRNG.
 */
static void _main_sys_random_init(void)
{
#if defined(MTK_SYS_TRNG_ENABLE)
    uint32_t            seed;
    hal_trng_status_t   s;

    s = hal_trng_init();

    if (s == HAL_TRNG_STATUS_OK) {
        s = hal_trng_get_generated_random_number(&seed);
        printf("seed %lx\n", seed);
    }

    if (s == HAL_TRNG_STATUS_OK) {
        srand((unsigned int)seed);
    }

    if (s != HAL_TRNG_STATUS_OK) {
        printf("trng init failed\n");
    } else {
        printf("random %d\n", rand());
    }
#endif /* MTK_SYS_TRNG_ENABLE */
}

void system_init(void)
{
    time_t      t       = 12345;

    /* SystemClock Config */
    SystemClock_Config();

#if defined(MTK_HAL_LOWPOWER_ENABLE)
    /* Handle low power interrupt */
    hal_lp_handle_intr();
#endif

    /* Configure the hardware ready to run the test. */
    prvSetupHardware();

    log_init(syslog_config_save, syslog_config_load, syslog_control_blocks);

#ifdef MTK_NVDM_ENABLE
    nvdm_init();
    nvdm_port_check_default_value();
#endif

#ifdef ENABLE_RTOS_TRACE
    trace_init(); // init appended FreeRTOS trace function.
#endif /* #ifdef ENABLE_RTOS_TRACE */

    /* workaround for NSTP */
    ctime(&t);

    _main_sys_random_init();

#if (CFG_INFO_TASK_EN == 1)
    info_task_init();
#endif


    LOG_I(common, "FreeRTOS Running");
}


