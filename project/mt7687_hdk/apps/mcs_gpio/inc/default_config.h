#ifndef __DEFAULT_CONFIG_H__
#define __DEFAULT_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

// start of wifi profile
#define DEF_WIFI_STA_MAC_ADDR   "00:0C:43:76:87:28"
#define DEF_WIFI_AP_MAC_ADDR    "00:0C:43:76:87:30"

#define DEF_WIFI_SSID           "DontConnectMe2G"

#if 1
// open-none
#define DEF_WIFI_AUTH_MODE      (0) //0:Ndis802_11AuthModeOpen, 7:Ndis802_11AuthModeWPA2PSK
#define DEF_WIFI_ENCRYPT_TYPE   (1) //1: Ndis802_11EncryptionDisabled 5:Ndis802_11Encryption3Enabled
#else
// wpa2-psk
#define DEF_WIFI_AUTH_MODE      (7) //0:Ndis802_11AuthModeOpen, 7:Ndis802_11AuthModeWPA2PSK
#define DEF_WIFI_ENCRYPT_TYPE   (5) //1: Ndis802_11EncryptionDisabled 5:Ndis802_11Encryption3Enabled
#endif

#define DEF_WIFI_WIRELESS_MODE  (9) //PHY_11BGN_MIXED,  // if check 802.11b.      9
#define DEF_WIFI_CHANNEL        (1)

#define DEF_WIFI_BSS_TYPE       (1) //BSS_INFRA
#define DEF_WIFI_BW             (0) //BW_20
#define DEF_WIFI_MCS            (33) //MCS_AUTO
#define DEF_WIFI_COUNTRY_REGION (5) //REGION_5_BG_BAND
#define DEF_WIFI_COUNTRY_REGION_A_BAND  (7) //REGION_7_A_BAND
#define DEF_WIFI_DBG_LEVEL      (1) //RT_DEBUG_ERROR
// end of wifi profile

// start of system config
#define DEF_STA_IP_ADDR         "192.168.0.28"
#define DEF_STA_IP_NETMASK      "255.255.255.0"
#define DEF_STA_IP_GATEWAY      "192.168.0.1"

#define DEF_AP_IP_ADDR          "192.168.1.12"
#define DEF_AP_IP_NETMASK       "255.255.255.0"
#define DEF_AP_IP_GATEWAY       "192.168.1.254"
// end of system config

// start of wpa_supplicant config
#define DEF_SUPP_KEY_MGMT       (0x100) // 0x100: WPA_KEY_MGMT_NONE, 0x10: WPA_KEY_MGMT_PSK
#define DEF_SUPP_SSID           "DontConnectMe2G"
#define DEF_SUPP_PASSPHRASE     "12345678"
// end of wpa_supplicant config

#ifdef __cplusplus
}
#endif

#endif // #ifndef __DEFAULT_CONFIG_H__

