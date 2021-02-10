#include <Arduino.h>
#include <WLAN/WLAN.h>


WLAN::WLAN(char * ssid, char * pwd) {
  this->ssid = ssid;
  this->pwd  = pwd;
  this->status = WLAN_STATUS_DISCONNECTED;
};



void WLAN::connect() {
  WiFi.begin(this->ssid, this->pwd);
  while (WiFi.status() != WL_CONNECTED)  {
    this->show();
  }

  this->local_ip =  WiFi.localIP().toString().c_str();
  this->status = WLAN_STATUS_CONNECTED;
  this->show();

  if (_WLAN_DEBUG_) {
    Serial.println("WLAN NAME=" + String(this->ssid));
    Serial.println("Local IP =" + this->local_ip);
  };

};


void WLAN::connect(bool show_status) {
  this->show_status = show_status;
  if (this->show_status) {
    pinMode(this->led_status_pin, OUTPUT);
  };
  this->connect();
};


void WLAN::show() {
  if (this->show_status) {
    switch (this->status) {
      case WLAN_STATUS_CONNECTED :
        digitalWrite(this->led_status_pin, digitalRead(this->led_status_pin) == HIGH);
        delay(1000);
        digitalWrite(this->led_status_pin, LOW);
        break;
      case WLAN_STATUS_DISCONNECTED:
        digitalWrite(this->led_status_pin, digitalRead(this->led_status_pin) == LOW);
        delay(50);
        break;
      default:
        digitalWrite(this->led_status_pin, LOW);
        break;  
    }
  }
};