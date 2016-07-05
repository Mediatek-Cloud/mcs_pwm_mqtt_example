#ifndef __FLASH_MAP_H__
#define __FLASH_MAP_H__

/*
 *       MT7637_CM4_common.ld
 *       build_cm4_flash.sh
 *       legacy/include/flash_map.h
 */

#ifdef __EVB_E1__ // E1

#define FLASH_LOADER_SIZE           0x8000        /*  32KB */
#define FLASH_STA_CONF_SIZE         0x2000        /*   8KB */
#define FLASH_AP_CONF_SIZE          0x1000        /*   4KB */
#define FLASH_EEPROM_CONF_SIZE      0x1000        /*   4KB */
#define FLASH_N9_RAM_CODE_SIZE      0x60000       /* 384KB */
#define FLASH_CM4_XIP_CODE_SIZE     0x40000       /* 256KB */
#define FLASH_TMP_SIZE              0x54000       /* 336KB */

#define CM4_FLASH_LOADER_ADDR       0x0
#define CM4_FLASH_STA_CONF_ADDR     (CM4_FLASH_LOADER_ADDR      + FLASH_LOADER_SIZE)
#define CM4_FLASH_AP_CONF_ADDR      (CM4_FLASH_STA_CONF_ADDR    + FLASH_STA_CONF_SIZE)
#define CM4_FLASH_EEPROM_CONF_ADDR  (CM4_FLASH_AP_CONF_ADDR     + FLASH_AP_CONF_SIZE)
#define CM4_FLASH_N9_RAMCODE_ADDR   (CM4_FLASH_EEPROM_CONF_ADDR + FLASH_EEPROM_CONF_SIZE)
#define CM4_FLASH_CM4_ADDR          (CM4_FLASH_N9_RAMCODE_ADDR  + FLASH_N9_RAM_CODE_SIZE)
#define CM4_FLASH_TMP_ADDR          (CM4_FLASH_CM4_ADDR         + FLASH_CM4_SIZE)

#else // E2

#define FLASH_LOADER_SIZE           0x8000        /*  32KB */
#define FLASH_COMM_CONF_SIZE        0x1000        /*   4KB */
#define FLASH_STA_CONF_SIZE         0x1000        /*   4KB */
#define FLASH_AP_CONF_SIZE          0x1000        /*   4KB */
#define FLASH_N9_RAM_CODE_SIZE      0x71000       /* 452KB */
#define FLASH_CM4_XIP_CODE_SIZE     0xBF000       /* 764KB */
#define FLASH_TMP_SIZE              0xBF000       /* 764KB */
#define FLASH_USR_CONF_SIZE         0x5000       /*  20KB */
#define FLASH_EEPROM_SIZE           0x1000       /*  4KB */

#define CM4_FLASH_LOADER_ADDR       0x0
#define CM4_FLASH_COMM_CONF_ADDR    (CM4_FLASH_LOADER_ADDR     + FLASH_LOADER_SIZE)
#define CM4_FLASH_STA_CONF_ADDR     (CM4_FLASH_COMM_CONF_ADDR  + FLASH_COMM_CONF_SIZE)
#define CM4_FLASH_AP_CONF_ADDR      (CM4_FLASH_STA_CONF_ADDR   + FLASH_STA_CONF_SIZE)
#define CM4_FLASH_N9_RAMCODE_ADDR   (CM4_FLASH_AP_CONF_ADDR    + FLASH_AP_CONF_SIZE)
#define CM4_FLASH_CM4_ADDR          (CM4_FLASH_N9_RAMCODE_ADDR + FLASH_N9_RAM_CODE_SIZE)
#define CM4_FLASH_TMP_ADDR          (CM4_FLASH_CM4_ADDR        + FLASH_CM4_XIP_CODE_SIZE)
#define CM4_FLASH_USR_CONF_ADDR     (CM4_FLASH_TMP_ADDR        + FLASH_TMP_SIZE)
#define CM4_FLASH_EEPROM_ADDR       (CM4_FLASH_USR_CONF_ADDR   + FLASH_USR_CONF_SIZE)

#endif // end of E2

#endif // __FLASH_MAP_H__

