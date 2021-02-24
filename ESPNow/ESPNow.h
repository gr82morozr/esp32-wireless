#pragma once 
#include <Arduino.h>
#include <Common/Common.h>

/* 
 * ========================================================================
 * ESPNow Two Way Comminication Remote Libary
 * Updated : 
 *  - ver 1.0 : 2020-05-08 - initial build
 *    ver 1.1 : 2021-02-24 - migrated into ESP32 Wireless lib 
 * 
 * Transmitter -> ESPNOW Master
 * Receiver    -> ESPNOW Slave 
 *  
 * Notes:  
 *  - Allows two-way communications between Transmitter and Receiver
 *    So it doesn't really matter which ESP32 is Transmitter or Receover
 *    
 * 
 * Steps:
 *  - Receiver hosts an WIFI network
 * Ste.
 *  - Transmitter connects to the WIFI network.
 *  - Once paired (connected), Transmitter sends a 'handshake' message 
 *    for Receiver to get the Transmitter's MAC address.
 *  - Ready for two way communications.
 *  
 * 
 * Exception Handling -
 *  - During handshake, it ensures the peers are ready to send & receive.
 *    in case of exception detected in handshake, it went to loop of retry.
 *  - During real communication, if error detected, will reconnect
 * ============================================================
*/ 

#define _ESP32_ESPNOW_DEBUG  0

#include <esp_now.h>
#include <WiFi.h>

// Common marcos
#define RC_TRANSMITTER      1
#define RC_RECEIVER         2
#define MAX_MSG_SIZE        200


// ESPNOW Specific marcos
#define ESPNOW_CHANNEL      1
#define ESPNOW_WLAN_SSID    "ESP32-ESPNOW-WLAN"
#define ESPNOW_WLAN_PWD     "vdjfiend#d0%d"

//Peer handshake
#define HANDSHAKE_MSG       "ESPNOW_HELLO_RC"

// Peer status enum 
#define PEER_ERROR          -1
#define PEER_NOT_FOUND      0
#define PEER_FOUND          1   // pairing is done, ready to handshake
#define PEER_PAIRED         2   // pairing is done, ready to handshake
#define PEER_HANDSHAKE      3   // Handshake in progress
#define PEER_READY          4   // Handshake is done, ready to communicate


class ESPNow {
  public:
    ESPNow(int role);
    static void init(void);
    static bool check_connection(void);
    static void send_data(String data);
    static String recv_data();

  private:
    static int role;

    static volatile int peer_status;
    static esp_now_peer_info_t peer;
    static uint8_t data_sent[MAX_MSG_SIZE];
    static String data_sent_str;
    static uint8_t data_recv[MAX_MSG_SIZE];
    static String data_recv_str;

    static void init_espnow();
    static void init_connection();
    static void config_ap();
    static void scan_network();
    static void pair_peer();
    static void debug(String message);
    static String mac2str(const uint8_t *mac_addr);
    static bool is_mac_set(const uint8_t *mac_addr);
    static void on_datasent(const uint8_t *mac_addr, esp_now_send_status_t status) ;
    static void on_datarecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
    static void do_handshake();
};

