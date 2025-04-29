#include "DHT22.h"


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//================ Begin: ĐỌC CẢM BIẾN NHIỆT ĐỘ & ĐỘ ẨM ===================//
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void DHT2x::KhoiTaoDHT22(void) {
  // Cảm biến SHT31 đọc nhiệt độ và độ ẩm.
  // Chú ý: Khai báo sau các khai báo pinMode.
  //---------------------------------------------------------------
  DHT dht(DHTPIN, DHTTYPE);
  dht.begin();
  //---------------------------------------------------------------
}
void DHT2x::DocCamBienNhietDoVaDoAmDHT22() {
  NhietDo_DHT = dht.readTemperature();;
  NhietDo_DHT = double(round(NhietDo_DHT * 100) / 100);  // will round to 2 decimal places i.e. 0.00
  DoAm_DHT = dht.readHumidity();
  DoAm_DHT = double(round(DoAm_DHT * 100) / 100);  // will round to 2 decimal places i.e. 0.00
  if (isnan(NhietDo_DHT) || isnan(DoAm_DHT)) {
    Serial.println("Lỗi đọc cảm biến DHT22!");
    NhietDo_DHT = -1;
    DoAm_DHT = -1;
  }

#ifdef debug
  Serial.print("Nhiệt độ: ");
  Serial.println(NhietDo_DHT);
  Serial.print("Độ ẩm: ");
  Serial.println(DoAm_DHT);
#endif
}
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//================ End: ĐỌC CẢM BIẾN NHIỆT ĐỘ & ĐỘ ẨM =====================//
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
