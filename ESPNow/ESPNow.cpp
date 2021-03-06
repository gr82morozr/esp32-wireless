#include <ESPNow/ESPNow.h>

// Declare all static varibles
int ESPNow::role;
volatile int ESPNow::peer_status;
uint8_t ESPNow::data_sent[MAX_MSG_SIZE];
uint8_t ESPNow::data_recv[MAX_MSG_SIZE];
String ESPNow::data_sent_str;
String ESPNow::data_recv_str;
esp_now_peer_info_t ESPNow::peer;

/*
 * =================================================================
 * Contructor
 *  - Controllor  
 *  - Receiver
 * =================================================================
*/
ESPNow::ESPNow(int r) {
  memset(peer.peer_addr, 0, 6);
  memset(data_sent, 0, MAX_MSG_SIZE);
  memset(data_recv, 0, MAX_MSG_SIZE);
  data_sent_str = "";
  data_recv_str = "";
  role = r;
  debug("This Device RC Role = " + String(role) );
}


/*
 * =================================================================
 * Init Controller and Receiver 
 *   
 * =================================================================
*/
void ESPNow::init(void) {
  init_espnow();
  init_connection();
}



/*
 * =================================================================
 * Init ESPNow configuration 
 *   if failed, reboot
 * =================================================================
*/
void ESPNow::init_espnow() {
  switch (role) {
    case RC_TRANSMITTER:
      /* Master connects to Receiver  */
      WiFi.mode(WIFI_STA);
      break;
    case RC_RECEIVER:
      /* Receiver hosts an WIFI network as AP */
      WiFi.mode(WIFI_AP);
      config_ap();
      break;
    default:
      debug("Error: No Role is defined. Rebooting ...");
      delay(1000);
      ESP.restart();
      break;
  }

  WiFi.disconnect();

  if (esp_now_init() == ESP_OK) {
    debug("ESPNow Init Success.");
  } else {
    debug("ESPNow Init Failed, Rebooting ...");
    delay(1000);
    ESP.restart();
  }

  esp_now_register_send_cb(ESPNow::on_datasent);
  esp_now_register_recv_cb(ESPNow::on_datarecv);
  peer_status = PEER_NOT_FOUND;
}


/*
 * =================================================================
 * Init Connections between Controller and Receiver
 *
 * =================================================================
*/
void ESPNow::init_connection(void) {

  peer_status = PEER_NOT_FOUND;
  while (peer_status != PEER_READY) {
    debug ("init_connection : Role(" + String(role) + ") peer_status=" + String(peer_status) + ")" );
    switch (peer_status) {
      case PEER_NOT_FOUND:
        if (role == RC_TRANSMITTER) { 
          scan_network();
        };
        break;
      case PEER_FOUND:  
        pair_peer();
        break;
      case PEER_PAIRED:  
        do_handshake();
        break;
      case PEER_HANDSHAKE:  
        break;
      case PEER_READY:  
        break;
      case PEER_ERROR:  
        peer_status = PEER_NOT_FOUND;
        break;    
      default:
        peer_status = PEER_NOT_FOUND;
        break;    
    }
  }
}


/*
 * =================================================================
 * Detect Network Connection Status
 *  - If error detected, needs to re-connect 
 * =================================================================
*/
bool ESPNow::check_connection(void) {
  int retry = 0;
  while (peer_status != PEER_READY && retry <=6) {
    debug ("check_connection : Role(" + String(role) + ") peer_status=" + String(peer_status) + ")" );
    retry ++;
    switch (peer_status) {
      case PEER_NOT_FOUND:
        if (role == RC_TRANSMITTER) { 
          scan_network();
        };
        break;
      case PEER_FOUND:  
        pair_peer();
        break;
      case PEER_PAIRED:  
        do_handshake();
        break;
      case PEER_HANDSHAKE:  
        break;
      case PEER_READY:  
        break;
      case PEER_ERROR:  
        peer_status = PEER_NOT_FOUND;
        break;    
      default:
        peer_status = PEER_NOT_FOUND;
        break;    
    }
  }
  debug ("check_connection : End : Role(" + String(role) + ") peer_status=" + String(peer_status) + ")" );
  return (peer_status==PEER_READY); 
}


/* 
 * ========================================================
 *   Only invoked from Receiver (Slave)
 *   
 * 
 * 
 * ==========================================================
 */
void ESPNow::config_ap(void) {
  bool result = WiFi.softAP(ESPNOW_WLAN_SSID, ESPNOW_WLAN_PWD, ESPNOW_CHANNEL, 0);
  if (!result) {
    debug("AP Config failed.");
  } else {
    debug("AP Config Success. Broadcasting with AP: " + String(ESPNOW_WLAN_SSID));
    debug(WiFi.macAddress());
  }
}


void ESPNow::scan_network() {
/* 
 * ================================================================== 
 *  Only invoked from Controller (Master)
 *  to scan for the possible receivers
 * 
 * 
 * 
 * 
 * ==================================================================
 */
   
  int8_t scanResults = WiFi.scanNetworks();
  peer_status = PEER_NOT_FOUND;
  memset(&peer, 0, sizeof(peer));
  if (scanResults > 0) {
    debug("scan_network :"  + String(scanResults) + " devices found.");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      delay(10);
      // Check if the current device matches
      if (SSID.indexOf(ESPNOW_WLAN_SSID) == 0) {
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &peer.peer_addr[0], &peer.peer_addr[1], &peer.peer_addr[2], &peer.peer_addr[3], &peer.peer_addr[4], &peer.peer_addr[5] ) ) {
          peer_status = PEER_FOUND;
        }          
        break;
      }
    }
  } else {
    debug("scan_network : Failed - No receiver found");
    peer_status = PEER_NOT_FOUND;
  }
  
  // clean up ram
  WiFi.scanDelete();
}



/* 
 * ========================================================
 * Pairing ESP Now Controller and Receiver
 * 
 * 
 * ========================================================
 */
void ESPNow::pair_peer() {
  if ( ! esp_now_is_peer_exist(peer.peer_addr) || peer_status != PEER_READY ) { // if not exists
    
    //clean existing pairing
    if (esp_now_del_peer(peer.peer_addr) == ESP_OK ) {
		  debug("pair_peer : Pair cleaned success - " + String(peer_status));
    }

    // prepare for pairing - RC receiver needs to set ifidx
    peer.channel = ESPNOW_CHANNEL;  // pick a channel
    peer.encrypt = 0;               // no encryption
    if (role ==RC_RECEIVER ) { 
      peer.ifidx = ESP_IF_WIFI_AP;
    }

    // do pairing
    switch (esp_now_add_peer(&peer)) {
      case ESP_OK :
        peer_status = PEER_PAIRED;
        debug("pair_peer : Success.");
        break;
      default:
        peer_status = PEER_NOT_FOUND;
        debug("pair_peer : Failed." );
        break;
    }
  } 
}


/* 
 * ========================================================
 * Handshake two peers
 *  - Needs to be done after pairing 
 * ========================================================
 */
void ESPNow::do_handshake() {
  // assume here the peer_status is PEER_PAIRED
  if (peer_status != PEER_PAIRED) {
    return ;
  };

  // do the hand shake
  switch (role) {
    case RC_TRANSMITTER :
      send_data(HANDSHAKE_MSG);
      if (peer_status != PEER_ERROR) {
        peer_status = PEER_HANDSHAKE;
      } else {
        return ; // if ERROR
      }
      break;
    case RC_RECEIVER:
      peer_status = PEER_HANDSHAKE;
      break;
    default :
      peer_status = PEER_HANDSHAKE;
      break;
  }

  debug("do_handshake : status = " + String(peer_status) );
}



/* 
 * ========================================================
 *  Public send_data
 * ==========================================================
 */

void ESPNow::send_data(String message) {
  int data_length = message.length();
  esp_err_t result;

  // set max message length
  if (data_length > MAX_MSG_SIZE) {
    data_length = MAX_MSG_SIZE;
  };

  
  // convert to uint8_t type.
  for (int i = 0; i < data_length; i++) {
    data_sent[i] = (uint8_t)message[i];
  };

  // send message
  debug("send_data : [" + message + "] to (" + mac2str(peer.peer_addr) + ")");
  result = esp_now_send(peer.peer_addr, data_sent, data_length);
  if (result == ESP_OK) {
    data_sent_str = ((String) ((char*)data_sent)).substring(0,data_length);
    debug("send_data : Success");
  } else {
    peer_status = PEER_ERROR;
    debug("send_data : Failed");
  }
}

/* 
 * ========================================================
 *  Public recv_data
 * ==========================================================
 */
String ESPNow::recv_data() {
  //String str = (char*)data_recv;
  return data_recv_str;
}



/* 
 * ========================================================
 * Call back functions
 *    
 * 
 * 
 * ==========================================================
 */

void ESPNow::on_datasent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  debug("on_datasent : to (" + mac2str(mac_addr) + ")");
  if (status != ESP_NOW_SEND_SUCCESS) {
    peer_status = PEER_ERROR;
    debug("on_datasent : Failed.");
  } else {
    debug("on_datasent : Success.");
  }
}

void ESPNow::on_datarecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  int data_length = data_len;

  // truncate the message if longer than limit.
  if (data_length>MAX_MSG_SIZE) {
    data_length =  MAX_MSG_SIZE;
  }

  // set peer_addr
  if (!is_mac_set(peer.peer_addr) || peer_status == PEER_NOT_FOUND ) {
    memcpy( &peer.peer_addr, mac_addr, 6 );
    peer_status = PEER_FOUND;
  };



  // set data_recv buffer
  memcpy(data_recv,data,data_length);
  data_recv_str = ((String) ((char*)data_recv)).substring(0,data_length);

  debug("on_datarecv : [" + data_recv_str + "](len=" + String(data_length)  + ") from (" + mac2str(peer.peer_addr) + ")");

  // once received hand shake message - then ready.
  if (data_recv_str == HANDSHAKE_MSG ) {
    if (role == RC_RECEIVER ) { // if receiver, echo message back.
      pair_peer();
      send_data(HANDSHAKE_MSG);
      if (peer_status == PEER_ERROR) {
        return;
      }
    };
    peer_status = PEER_READY;
    debug("on_datarecv : peer_status = " + String(peer_status));
  };
}


/* 
 * ========================================================
 *   Common Utility Functions
 *   
 * 
 * 
 * ==========================================================
 */
void ESPNow::debug(String message) {
  if (_ESP32_ESPNOW_DEBUG) {
    Serial.println(String(millis()) + " : " + message);
  }
}

String ESPNow::mac2str(const uint8_t *mac_addr) {
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  return mac_str;
}


bool ESPNow::is_mac_set(const uint8_t *mac_addr) {
  // Check the MAC has been set
  int n = 6;
  while(--n>0 && mac_addr[n]==mac_addr[0]);
  return n!=0;
}