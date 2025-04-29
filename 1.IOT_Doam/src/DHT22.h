#ifndef _DHT22_h
#define _DHT22_h


#include "IoTVision.h"  // Có define debug để bật/tắt các debug ra Serial.


// Thư viện cho cảm biến nhiệt độ và độ ẩm DHT22
#include "DHT.h"

//-----------------------------------------------------------------
#define DHTPIN 4   //Chân data
#define DHTTYPE DHT22 //Chọn loại cảm biến
//-----------------------------------------------------------------


class DHT2x {
private:
    DHT dht;

public:
  double NhietDo_DHT;
  double DoAm_DHT;
  DHT2x() : dht(DHTPIN, DHTTYPE) {}

public:
  void KhoiTaoDHT22(void);
  void DocCamBienNhietDoVaDoAmDHT22(void);
};


#endif
