#pragma once

#include <Arduino.h>
#include <WiFi.h>

/*
  ESP32 wrapper class for WLAN (WiFi) connection

  // ========================================================
  //    Sample code:
  // ========================================================
  // include customized headers

  #include <WLAN/WLAN.h>
  WLAN wlan = WLAN("MyWiFi", "WiFiPassword");
  
  
  void setup() {
    wlan.connect(true);  // show status while connecting
    //or 
    //wlan.connect(true);
  }

  void loop() {
    delay(1000);
  }
  // ========================================================

*/



#define _WLAN_DEBUG_        0
#define _BUILTIN_LED_       2

#define WLAN_STATUS_DISCONNECTED  0
#define WLAN_STATUS_CONNECTED     1


class WLAN {
  public:
    WLAN(char * ssid, char * pwd);
    void connect();
    void connect(bool show_status);
    

  private:
    int status = WLAN_STATUS_DISCONNECTED;
    int led_status_pin = _BUILTIN_LED_;
    bool show_status = false;
    String local_ip;
    char *ssid;
    char *pwd;
    void show();

};