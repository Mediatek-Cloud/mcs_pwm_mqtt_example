#ifndef __HOMEKIT_H__
#define __HOMEKIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "cli.h"
#ifndef __HAP_TEST_H__
#define __HAP_TEST_H__
extern cmd_t hap_cli[];
extern void hap_test(void *pvParameters);
extern void hap_reset(void *pvParameters);
#endif

#ifndef __WAC_TEST_H__
#define __WAC_TEST_H__
extern cmd_t    wac_cli[];
#endif

#ifndef __MDNS_TEST_H__
#define __MDNS_TEST_H__
extern cmd_t    mdns_cli[];
extern TaskHandle_t mDNSTaskHandler;
#endif

#ifndef __WACSERVER_H__
#define __WACSERVER_H__
extern void wacserver_start_task(void *args);
extern void wacserver_stop_task(void *args);
extern void wacserver_reset_task(void *args);
#endif

#ifndef __POSIXDAEMON_H__
#define __POSIXDAEMON_H__
extern void mDNS_UpdateInterface();
extern void SetStopBit();
extern int MainDaemonTask(int argc, char **argv);
#endif

#ifndef __MFI_CLI_H__
#define __MFI_CLI_H__
extern cmd_t mod_mfi_cli[];
#endif /* __AACP_CLI_H__ */

extern int homekit_init();

#ifdef __cplusplus
}
#endif

#endif
