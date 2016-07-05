ifeq ($(PROJ_PATH),)
include feature.mk
else
include $(PROJ_PATH)/feature.mk
endif
MTK_HIF_GDMA_ENABLE 				= n
MTK_IPERF_ENABLE                    = n
MTK_PING_OUT_ENABLE                 = n
MTK_SMTCN_ENABLE                    = n
MTK_WIFI_WPS_ENABLE                 = n
MTK_WIFI_REPEATER_ENABLE            = n
MTK_MBEDTLS_CONFIG_FILE             = config-mtk-homekit.h
MTK_TFTP_ENABLE                     = n
MTK_HOMEKIT_ENABLE					= y
