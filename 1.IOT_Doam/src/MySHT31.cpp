#include "MySHT31.h"


//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//================ Begin: ĐỌC CẢM BIẾN NHIỆT ĐỘ & ĐỘ ẨM ===================//
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void SHT3x::KhoiTaoSHT31(void) {
  // Cảm biến SHT31 đọc nhiệt độ và độ ẩm.
  // Chú ý: Khai báo sau các khai báo pinMode.
  //---------------------------------------------------------------
  sht31.begin(SHT31_ADDRESS, dataPin, clockPin);
  //---------------------------------------------------------------
}
void SHT3x::DocCamBienNhietDoVaDoAmSHT31() {
  sht31.read();
  NhietDo = sht31.getTemperature();
  Serial.print("Nhiệt độ doc duoc : ");
  Serial.println(NhietDo);
  NhietDo = double(round(NhietDo * 100) / 100);  // will round to 2 decimal places i.e. 0.00
  Serial.print("Nhiệt độ converter : ");
  Serial.println(NhietDo);
  DoAm = sht31.getHumidity();
  Serial.print("Độ ẩm doc duoc : ");
  Serial.println(DoAm);
  DoAm = double(round(DoAm * 100) / 100);  // will round to 2 decimal places i.e. 0.00
  Serial.print("Độ ẩm converter : ");
  Serial.println(DoAm);


  // Trường hợp không có kết nối cảm biến thì giá trị trả về là 
  // nhiệt độ = 130 (hoặc -45) & // độ ẩm = 100, lúc này trả về -1 
  // cho cả 2 thông số nhiệt độ & độ ẩm để trên app biết mà hiển thị 
  // trạng thái không có cảm biến kết nối với board để cho user biết.
  if (NhietDo >= 130 || NhietDo <= -45) {
    NhietDo = -1;
    DoAm = -1;
  }


#ifdef debug
  Serial.print("Nhiệt độ: ");
  Serial.println(NhietDo);
  Serial.print("Độ ẩm: ");
  Serial.println(DoAm);
#endif
}
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//================ End: ĐỌC CẢM BIẾN NHIỆT ĐỘ & ĐỘ ẨM =====================//
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
