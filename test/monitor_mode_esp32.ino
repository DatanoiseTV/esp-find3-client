/* Part of https://github.com/DatanoiseTV/esp-find3-client/ */

#include <WiFi.h>
#include <esp_wifi.h>

const wifi_promiscuous_filter_t filt={
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct { // or this
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct {
  int16_t fctl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;


String maclist[64][4];
int listcount = 0;

String defaultTTL = "15";

long previousMillis = 0; 
long interval = 1000; 


#define maxCh 13 // max Channel -> US = 11, EU = 13, Japan = 14

int curChannel = 1;

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  int len = p->rx_ctrl.sig_len;
  WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;
  int fctl = ntohs(wh->fctl);
  len -= sizeof(WifiMgmtHdr);
  if (len < 0){
    return;
  }

  if(type == 0) // WIFI_PKT_CTRL
  {
    wifi_pkt_rx_ctrl_t pkt = p->rx_ctrl;
    String packet, mac, mac_eui;
    for(int i=8;i<=p->rx_ctrl.sig_len;i++){
     packet += String(p->payload[i],HEX);
    }
    for(int i=4;i<=15;i++){
      mac += packet[i];
    }

    for(int i=0;i<mac.length();i=i+2)
    {
      if((i+2)<mac.length())
       mac_eui+=mac.substring(i, i+2)+":";
      if((i+2)==mac.length())
      {
       mac_eui+=mac.substring(i, i+2);
      }
    }
    

    // Serial.println(String(": ") + mac_eui + String(": RSSI ") + pkt.rssi + String(" channel ") + pkt.channel);

  int added = 0;
  for(int i=0;i<=63;i++){ // checks if the MAC address has been added before
    if(mac_eui == maclist[i][0]){
      maclist[i][1] = defaultTTL;
      maclist[i][3] = pkt.rssi;
      if(maclist[i][2] == "OFFLINE"){
        maclist[i][2] = "0";
      }
      added = 1;
    }
  }
  
  if(added == 0){ // If its new. add it to the array.
    maclist[listcount][0] = mac_eui;
    maclist[listcount][1] = defaultTTL;
    maclist[listcount][3] = pkt.rssi;
    //Serial.println(mac);
    listcount ++;
    if(listcount >= 64){
      Serial.println("Too many addresses");
      listcount = 0;
    }
  }


  }
}

void purge(){ // This maanges the TTL
  for(int i=0;i<=63;i++){
    if(!(maclist[i][0] == "")){
      int ttl = (maclist[i][1].toInt());
      ttl --;
      if(ttl <= 0){
        //Serial.println("OFFLINE: " + maclist[i][0]);
        maclist[i][2] = "OFFLINE";
        maclist[i][1] = defaultTTL;
      }else{
        maclist[i][1] = String(ttl);
      }
    }
  }
}

void updatetime(){ // This updates the time the device has been online for
  for(int i=0;i<=63;i++){
    if(!(maclist[i][0] == "")){
      if(maclist[i][2] == "")maclist[i][2] = "0";
      if(!(maclist[i][2] == "OFFLINE")){
          int timehere = (maclist[i][2].toInt());
          timehere ++;
          maclist[i][2] = String(timehere);
      }
      
      
    }
  }
}


void setup() {
  Serial.begin(115200);
  
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
}

void loop() {
    updatetime();
    purge();
    
    if(curChannel > maxCh){ 
      curChannel = 1;
    }
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
    
    unsigned long currentMillis = millis();
    
    if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
      previousMillis = currentMillis;   
      curChannel++;
    }
 

    for(int i=0; i < listcount; i++)
  {
    Serial.println(maclist[i][0] + String("(") + maclist[i][3] + String("), "));

  }
  Serial.println();
  Serial.println(listcount + String (" known BSSIDs"));

  
}
