#include <BLEDevice.h>
#include <TFT_eSPI.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>

#define TFT_GREY      0x5AEB
#define TFT_BG_RED     10240
#define TFT_BG_ORANGE  12512
#define TFT_BG_GREEN     384
#define TFT_BG_BLUE        5
#define TFT_BG_GREY    10565


static BLEUUID serviceUUID("0000180d");
static BLEUUID charUUID("00002a37");

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice*     myDevice;

static boolean doConnect    = false;
static boolean connected    = false;
static boolean doScan       = false;

static uint16_t ppi_latest  = 1;
static uint8_t  bpm_latest  = 255;
static uint16_t ts_latest;

static uint16_t bufferIndex = 0;
static uint16_t PPIBuffer[16384];
static uint8_t  BPMBuffer[16384];

const char dayOfTheWeek[7][5] = {"Zon", "Maa", "Din", "Woe", "Don", "Vrij", "Zat"};
const char monthLong[12][10]  = {
  "januari",
  "februari",
  "maart",
  "april",
  "mei",
  "juni",
  "juli",
  "augustus",
  "september",
  "oktober",
  "november",
  "december" };

unsigned long millis_c = 0;
unsigned long millis_p = 0;
boolean  blink1;
uint8_t  seconds_c, seconds_p, minute_c, minute_p, halfMin_c, halfMin_p, hour_c, hour_p, hour1, hour2;
uint8_t  bpm_min[321], bpm_max[321], bpm_avg[321];
uint8_t  bpm           = 125;
uint16_t bpmColor      = TFT_BLUE;
uint16_t bufferIndex_p = 0;
uint16_t t             = 0;

TFT_eSPI     display     = TFT_eSPI();
TFT_eSprite  graph       = TFT_eSprite(&display);
TFT_eSprite  klok        = TFT_eSprite(&display);
TFT_eSprite  klok_s      = TFT_eSprite(&display);
TFT_eSprite  label0      = TFT_eSprite(&display);
TFT_eSprite  label1      = TFT_eSprite(&display);
TFT_eSprite  label2      = TFT_eSprite(&display);
TFT_eSprite  label_rm    = TFT_eSprite(&display);
TFT_eSprite  rate        = TFT_eSprite(&display);
TFT_eSprite  ratebar     = TFT_eSprite(&display);
TFT_eSprite  ratebarleds = TFT_eSprite(&display);


// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks class


void setup() {
  BLEDevice::init("");
  
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);


  Wire.begin(27,22);
  Serial.begin(115200);
  display.init();
  display.setRotation(1);
  display.invertDisplay(true);
  display.fillScreen(TFT_BLACK);
  display.setTextSize(1);
  display.setTextColor(TFT_WHITE,TFT_BLACK);
  display.setCursor(24, 0, 2);
  display.print("BPM:");

  setSyncProvider(RTC.get);
  //if (timeStatus() == timeSet) Serial.println("RTC has set the system time");      
  //else                         Serial.println("Unable to sync with the RTC");
 
  label0.setColorDepth(1);
  label0.setTextColor(TFT_WHITE,TFT_BLACK);
  label0.createSprite(30,8);
  label0.fillSprite(TFT_BLACK);
  label1.setColorDepth(1);
  label1.setTextColor(TFT_WHITE,TFT_BLACK);
  label1.createSprite(30,8);
  label1.fillSprite(TFT_BLACK);
  label2.setColorDepth(1);
  label2.setTextColor(TFT_WHITE,TFT_BLACK);
  label2.createSprite(30,8);
  label2.fillSprite(TFT_BLACK);
  label_rm.createSprite(30,8);
  label_rm.fillSprite(TFT_BLACK);

  graph.createSprite(320,150);

  klok.setColorDepth(1);
  klok.createSprite(163,53);
  //klok.createSprite(132,56);
  klok.setTextSize(1);
  klok.setTextColor(TFT_WHITE,TFT_BLACK);
  klok.fillSprite(TFT_BLACK);
  klok.drawChar(':', 58, 16, 6);

  klok_s.setColorDepth(1);
  klok_s.createSprite(30,20);
  klok_s.setTextSize(1);
  klok_s.setTextColor(TFT_WHITE,TFT_BLACK);

  rate.setColorDepth(8);
  rate.createSprite(96,48);
  rate.fillSprite(TFT_BLACK);
  rate.setTextSize(0);
  rate.setTextColor(TFT_WHITE,TFT_BLACK);

  ratebar.createSprite(16,75);
  ratebar.fillSprite(TFT_BLACK);
  ratebar.drawRect(1,1,13,73,TFT_WHITE);
  ratebar.drawRect(0,0,15,75,TFT_GREY);

  ratebarleds.createSprite(11,71);
  ratebarleds.fillSprite(TFT_BLACK);
  ratebarleds.fillRect(0, 0,11,8,TFT_BG_RED);
  ratebarleds.fillRect(0, 9,11,8,TFT_BG_RED);
  ratebarleds.fillRect(0,18,11,8,TFT_BG_ORANGE);
  ratebarleds.fillRect(0,27,11,8,TFT_BG_GREEN);
  ratebarleds.fillRect(0,36,11,8,TFT_BG_GREEN);
  ratebarleds.fillRect(0,45,11,8,TFT_BG_GREEN);
  ratebarleds.fillRect(0,54,11,8,TFT_BG_BLUE);
  ratebarleds.fillRect(0,63,11,8,TFT_BG_BLUE);
  ratebarleds.pushToSprite(&ratebar,2,2);
  
  ratebar.pushSprite(104,1);
 
  Serial.println();
  Serial.print("Custom color calc: ");
  Serial.println(display.color565(40, 40, 40));

  minute_c  = minute();
  halfMin_c = (minute_c * 2) + (seconds_c / 30);

  hour_c                = hour();
  if (hour_c < 1) hour1 = 23;
  else            hour1 = hour_c - 1;
  if (hour1 < 1)  hour2 = 23;
  else            hour2 = hour1 - 1;
}



void loop() {
  millis_c=millis();
  seconds_c = second();

  char   val_char[10];
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("-= Connected to BLE server =-");
    } else {
      Serial.println("-= Connection to BLE server failed =-");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (!connected) {
    if(doScan) BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect,
                                                // most likely there is better way to do it in Arduino
  }

  // =====================================================
  // Run this section every time a measurement is received
  // =====================================================
  if (bufferIndex != bufferIndex_p) {
    bufferIndex_p = bufferIndex;

    float bpm_calc = (float(60000) / float(ppi_latest)) + 0.05;

    dtostrf(bufferIndex, 6, 0, val_char);
    Serial.print("        index: ");
    Serial.println(val_char);

    dtostrf(ppi_latest, 5, 0, val_char);
    Serial.print("          PPI: ");
    Serial.println(val_char);

    dtostrf(bpm_latest, 5, 0, val_char);
    Serial.print("          BPM: ");
    Serial.println(val_char);

    dtostrf(bpm_calc, 5, 1, val_char);
    Serial.print("          BPM: ");
    Serial.println(val_char);

    if ( bpm_min[t] == 0)          bpm_min[t] = 65535;
    if ( bpm_latest < bpm_min[t] ) bpm_min[t] = bpm_latest;
    if ( bpm_latest > bpm_max[t] ) bpm_max[t] = bpm_latest;
    
  }


  // ===========================
  // Run this section every hour
  // ===========================
  if (hour_c != hour_p ) {
    hour_p = hour_c;
    klok.drawNumber(hour_c / 10,   0, 16, 6);
    klok.drawNumber(hour_c % 10,  29, 16, 6);

    label0.fillSprite(1);
    label0.setCursor(0, 0, 1);
    if (hour_c < 10)
      label0.print(" ");
    label0.print(hour_c);
    label0.print(":00");

    label1.fillSprite(1);
    label1.setCursor(0, 0, 1);
    if (hour1 < 10)
      label1.print(" ");
    label1.print(hour1);
    label1.print(":00");

    label2.fillSprite(1);
    label2.setCursor(0, 0, 1);
    if (hour2 < 10)
      label2.print(" ");
    label2.print(hour2);
    label2.print(":00");
  }

  // =============================
  // Run this section every minute
  // =============================
  if (minute_c != minute_p ) {
    minute_p = minute_c;
    klok.drawNumber(minute_c / 10,  75, 16, 6);
    klok.drawNumber(minute_c % 10, 104, 16, 6);
    klok.setCursor(0, 0, 2);
    klok.print(" ");
    klok.print(dayOfTheWeek[weekday()-1]);
    klok.print(" ");
    klok.print(day());
    klok.print(" ");
    klok.print(monthLong[month()-1]);
    klok.print(" ");
    klok.print(year());
    klok.pushSprite(157,0);
    klok_s.pushSprite(291,33);
  }

  // ==================================
  // Run this section every half minute
  // ==================================
  if (halfMin_c != halfMin_p ) {
    halfMin_p = halfMin_c;
    t++;
    
    graph.fillRect(0, 0,320, 30, TFT_BG_RED);
    graph.fillRect(0,30,320, 15, TFT_BG_ORANGE);
    graph.fillRect(0,45,320, 30, TFT_BG_GREEN);
    graph.fillRect(0,75,320, 75, TFT_BG_BLUE);
    if (halfMin_c > 59)
      graph.drawFastVLine(379-halfMin_c,0,150,TFT_BG_GREY);
    graph.drawFastVLine(319-halfMin_c,0,150,TFT_GREY);
    graph.drawFastVLine(259-halfMin_c,0,150,TFT_BG_GREY);
    graph.drawFastVLine(199-halfMin_c,0,150,TFT_GREY);
    graph.drawFastVLine(139-halfMin_c,0,150,TFT_BG_GREY);
    if (halfMin_c < 80)
      graph.drawFastVLine(79-halfMin_c,0,150,TFT_GREY);
    if (halfMin_c < 20)
      graph.drawFastVLine(19-halfMin_c,0,150,TFT_BG_GREY);

    for (uint16_t i=t; i--; i<1) {
      //graph.drawPixel(319-i,199-bpm_max[t-i],TFT_WHITE);
      //graph.drawPixel(319-i,199-bpm_min[t-i],TFT_WHITE);
      graph.drawFastVLine(319-i,199-bpm_max[t-i],bpm_max[t-i]-bpm_min[t-i],TFT_GREY);
    }

    graph.pushSprite(0,90);

    if (halfMin_c < 20)   label0.pushSprite(285,           79);
    else                  label0.pushSprite(305-halfMin_c, 79);
                          label1.pushSprite(185-halfMin_c, 79);
    if (halfMin_c < 80) {
      if (halfMin_c > 65) label2.pushSprite(0,             79);
      else                label2.pushSprite(65-halfMin_c,  79);
    }
    else                  label_rm.pushSprite(0,           79);
  }

  // =============================
  // Run this section every second
  // =============================
  if (seconds_c != seconds_p ) {

    seconds_p = seconds_c;
    minute_c  = minute();
    halfMin_c = (minute_c * 2) + (seconds_c / 30);
    hour_c    = hour();

    if (hour_c < 1) hour1 = 23;
    else            hour1 = hour_c - 1;
    if (hour1 < 1)  hour2 = 23;
    else            hour2 = hour1 - 1;

    klok_s.fillSprite(TFT_BLACK);
    klok_s.drawNumber(seconds_c / 10, 0, 0, 4);
    klok_s.drawNumber(seconds_c % 10,15, 0, 4);
    klok_s.pushSprite(291,33);
    
    if (bpm_latest < 120 ) rate.setTextColor(TFT_BLUE,TFT_BLACK);
    else {
      if (bpm_latest >= 120) rate.setTextColor(TFT_GREEN,TFT_BLACK);
      if (bpm_latest >= 150) rate.setTextColor(TFT_ORANGE,TFT_BLACK);
      if (bpm_latest >= 160) rate.setTextColor(TFT_RED,TFT_BLACK);
    }

    if   (bpm_latest > 100 ) rate.drawNumber(bpm_latest,  0, 0,7);
    else {
      if (bpm_latest >  10 ) {
        rate.fillRect(0,0,32,48,TFT_BLACK);
        rate.drawNumber(bpm_latest, 32, 0,7);
      }
      else {
        rate.fillRect(0,0,64,48,TFT_BLACK);
        rate.drawNumber(bpm_latest, 64, 0,7);
      }
    }
    rate.pushSprite(0,19);
  }

  // =======================================
  // Run this section every part of a second
  // =======================================
  if ( (millis_c - millis_p) >  250 ) {
    millis_p = millis_c;

    blink1=!blink1;
    if ( blink1 ){
      if (bpm_latest < 90) ratebar.fillRect(2,65,11,8, TFT_BLUE);
      else if (bpm_latest < 120) ratebar.fillRect(2,56,11,8, TFT_BLUE);
           else if (bpm_latest < 130) ratebar.fillRect(2,47,11,8, TFT_GREEN);
                else if (bpm_latest < 140) ratebar.fillRect(2,38,11,8, TFT_GREEN);
                     else if (bpm_latest < 150) ratebar.fillRect(2,29,11,8, TFT_GREEN);
                          else if (bpm_latest < 160) ratebar.fillRect(2,20,11,8, TFT_ORANGE);
                               else if (bpm_latest < 170) ratebar.fillRect(2,11,11,8, TFT_RED);
                                    else ratebar.fillRect(2,2,11,8, TFT_RED);
    }
    else ratebarleds.pushToSprite(&ratebar,2,2);
    ratebar.pushSprite(104,1);
  }
}


static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  uint16_t ppi = 1;
  //Serial.print("data: ");
  //Serial.print(length);
  //Serial.println(" [bytes]");
  //Serial.print("rate: ");
  //Serial.print(pData[1]);
  //Serial.println(" [bpm]");
  for (uint8_t i=2; i<length; i+=2) {
    ppi = pData[i]+(pData[i+1]*256);
    PPIBuffer[bufferIndex]=ppi;
    BPMBuffer[bufferIndex]=pData[1];
    bufferIndex++;
    //Serial.print("ppi : ");
    //Serial.print(ppi);
    //Serial.println(" [ms]");
  }
  if (length > 2) ppi_latest = ppi;
  bpm_latest = pData[1];
  //Serial.println();
} // notifyCallback


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
}; // MyClientCallback class


bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
    
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if(pRemoteCharacteristic->canRead()) {
    //std::string value = pRemoteCharacteristic->readValue();
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
  connected = true;
  return true;
} // connectToServer() function
