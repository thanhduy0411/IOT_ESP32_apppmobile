#include "ChuongTrinhChinh.h"
#include "WIFI1.h"
#include "POSTGET.h"
#include "RTC.h"
#include "MySHT31.h"
#include "Relay.h"


#define CoKetNoiServer // Khai báo để dùng tính năng POST/GET với server.
#define xTaskPOSTDuLieuVeCloud // Để thực thi task POST dữ liệu 
#define xTaskLangNgheLenhAppGuiXuongBoard    // Để thực thi task lắng nghe lệnh gửi từ app xuống board (chạy ở core 1).
//------------------------------------------------------------------------
#define CORE_1 1     // core 1 
#define CORE_0 0     // core 0
#define TASK_POST 1  //
#define TASK_GET 0   //
//------------------------------------------
// Định nghĩa các mã code tác vụ
#define CODEUserChonONOFF 11 // Mã lệnh điều khiển ON|OFF trên app gửi xuống
//------------------------------------------------------------------------
WIFI _WiFi;       // Kết nối WiFi cho board.
POSTGET _POSTGET; // Các hàm thực thi POST - GET giữa device và serve.
RTC _RTC;         // Mô-đun thời gian thực DS3231.
SHT3x _SHT31;     // Cảm biến nhiệt độ và độ ẩm SHT21.
Relay _Relay;     // Điều khiển rơ-le
int _LenhONOFFK1 = 0; // Lệnh điều khiển ON/OFF K1 gửi từ app xuống board.
int _LenhONOFFK2 = 0; // Lệnh điều khiển ON/OFF K2 gửi từ app xuống board.
//------------------------------------------------------------------------
String _ID; // Số ID của ESP32, đây là số IMEI của board.
int _CODE;  // Mã CODE gửi từ app xuống board để thực thi các tác vụ.
TaskHandle_t _POST, _GET;  // Xác định địa chỉ task để thao tác như xóa task.
SemaphoreHandle_t _mutexPostGet; // Khóa không cho post get cùng lúc.
unsigned long _tickPOSTDuLieu = 0; // Đếm để xác định thời điểm gọi hàm POSTDuLieuVeCloudDeHienThiTrenAPP()
bool _ChoPhepPOSTDuLieu = false; // Cờ cho phép gọi hàm POSTDuLieuVeCloudDeHienThiTrenAPP()
//--------------------------------------------------------------------------//
  //  Dữ liệu gửi xuống board bao gồm:
  //  String  ID;     // Số IMEI của board.
  //  int     K1;     // 0: Kênh 1 tắt; 1: Kênh 1 mở
  //  int     K2;     // 0: Kênh 2 tắt; 1: Kênh 2 mở
  //  int     K3;     // 0: Kênh 3 tắt; 1: Kênh 3 mở
  //  int     K4;     // 0: Kênh 4 tắt; 1: Kênh 4 mở
  //  int     MODE;   // Gửi chế độ điều khiển xuống board là MANUAL hay AUTO
  //  int     CODE:   // Gửi mã CODE xuống board để board biết mà thực thi tác vụ tương ứng
//--------------------------------------------------------------------------//

//------------------------------------------------------------------------


#pragma region Tasks
//========================================================================
#pragma region TaskPOSTDuLieuVeCloud
#ifdef xTaskPOSTDuLieuVeCloud
void TaskPOSTDuLieuVeCloud(void* pvParameter) {
  for (;;) {
    if (xSemaphoreTake(_mutexPostGet, portMAX_DELAY) == pdTRUE) {
      // Nếu đếm đủ thời điểm đã thiết lập cho phép POST dữ liệu 
      // lên cloud (ex: 60000 => 1 phút)
      if (_tickPOSTDuLieu >= 6000 || _ChoPhepPOSTDuLieu == true) {
        _tickPOSTDuLieu = 0;
        _ChoPhepPOSTDuLieu = false;
        POSTDuLieuVeCloudDeHienThiTrenAPP();
      }
      _tickPOSTDuLieu++;


      xSemaphoreGive(_mutexPostGet);
      vTaskDelay(pdMS_TO_TICKS(1));  // 1ms


#ifdef debug
Serial.printf("Kích thước task POST %u\n", uxTaskGetStackHighWaterMark(NULL));
      Serial.printf("_tickPOSTDuLieu %u\n", _tickPOSTDuLieu);
#endif
    }
  }
}
#endif
#pragma endregion TaskPOSTDuLieuVeCloud
//========================================================================//
#pragma region TaskLangNgheLenhAppGuiXuongBoard
#ifdef xTaskLangNgheLenhAppGuiXuongBoard
void TaskLangNgheLenhAppGuiXuongBoard(void* pvParameter) {
  for (;;) {
    if (xSemaphoreTake(_mutexPostGet, portMAX_DELAY) == pdTRUE) {
      LangNgheLenhAppGuiXuongBoard();
      xSemaphoreGive(_mutexPostGet);
      vTaskDelay(pdMS_TO_TICKS(100));


#ifdef debug
      Serial.printf("Kích thước task GET %u\n", uxTaskGetStackHighWaterMark(NULL));
#endif
    }
  }
}
#endif
#pragma endregion TaskLangNgheLenhAppGuiXuongBoard

//========================================================================//
#pragma endregion Tasks


void KhoiTao(void) {
  #pragma region KhoiTao
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
      
  // Trước tiên cần phải khởi tạo các PINs.
  _Relay.KhoiTaoCacChan();
  //---------------------------------------------------------------
  // Khi vừa cấp nguồn cho board thì tất cả rơle điều khiển các
  // thiết bị trong hệ thống đều phải OFF để đảm bảo các thiết bị
  // OFF ngay thời điểm mở máy lên.
  _Relay.OFFCacRole();


  _Relay.MODE = _MANUAL; // Với board ví dụ này thì MODE là MANUAL.
  //---------------------------------------------------------------


  //---------------------------------------------------------------
  // Khởi tạo giao tiếp SERIAL
  Serial.begin(9600);
  


  //---------------------------------------------------------------
  // Để có thể kết nối với các mô-đun đọc cảm biến SHT3x,
  // mô-đun thời gian thực, mô-đun mở rộng port PCF8574
  // thì trước tiên cần phải khởi tạo giao tiếp I2C
  // (Start the I2C interface firtly).
  Wire.setPins(pinSDA, pinSCL);
  Wire.begin();
  //---------------------------------------------------------------


  // Số ID (ID) của ESP32.
  _ID = _WiFi.LaySoMAC(); 


  #ifdef debug
  Serial.print("ID: ");
  Serial.println(_ID);
  #endif


  //----------------------------------------------------------------
  // Cảm biến SHT31 đọc nhiệt độ và độ ẩm.
  // Chú ý: Khai báo sau các khai báo pinMode ở trên.
  _SHT31.KhoiTaoSHT31();
  //----------------------------------------------------------------


  //---------------------------------------------------------------
  #pragma endregion Các khởi tạo cơ bản của máy


  #ifdef CoKetNoiServer
  #pragma region Khởi tạo WIFI
  //======================================================================
  //------ Begin: Khởi tạo để có thể cấu hình kết nối WiFi tự động -----//
  //======================================================================
  // Dành 10s để kết nối WiFI
  // Lưu ý: Phải có thời gian chờ cho việc kết nối WIFI nếu không sẽ
  // gây ra tình trạng board bị reset và không thể phát access point (AP).
  // Hàm thực hiện kết nối WiFi đồng thời khởi động chạy Task để sau mỗi
  // chu kỳ t(ms) thì sẽ gọi hàm KiemTraKetNoiWiFi một lần.
  _WiFi.KetNoiWiFi(10);
  //======================================================================
  //------ End: Khởi tạo để có thể cấu hình kết nối WiFi tự động -------//
  //======================================================================
  #pragma endregion Khởi tạo WIFI
  #endif


  #pragma region TaskMultiCore
  // khởi tạo khóa không cho post get cùng lúc
  _mutexPostGet = xSemaphoreCreateMutex();  
  delay(100);


  //--------------------------------------------------------------------------
  #ifdef xTaskPOSTDuLieuVeCloud
  xTaskCreatePinnedToCore(
    TaskPOSTDuLieuVeCloud,                    /* Task function. */
    "Task POSTDuLieuVeCloudDeHienThiTrenAPP", /* name of task. */
    10000,                                    /* Stack size of task */
    NULL,                                     /* parameter of the task */
    6,                                        /* priority of the task */
    &_POST,                                   /* Task handle to keep track of created task */
    CORE_1);                                  /* pin task to core 1 */
  delay(500);
  #endif
  //--------------------------------------------------------------------------
  #ifdef xTaskLangNgheLenhAppGuiXuongBoard
  xTaskCreatePinnedToCore(
    TaskLangNgheLenhAppGuiXuongBoard,    /* Task function. */
    "Task LangNgheLenhAppGuiXuongBoard", /* name of task. */
    10000,                               /* Stack size of task */
    NULL,                                /* parameter of the task */
    1,                                   /* priority of the task */
    &_GET,                               /* Task handle to keep track of created task */
    CORE_0);                             /* pin task to core 0 */
  #endif // xTaskLangNgheLenhAppGuiXuongBoard

  #pragma endregion TaskMultiCore


  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  Serial.println("Khoi tao xong");
  #pragma endregion KhoiTao
}


void ChayChuongTrinhChinh(void) {
  delay(1);
  //POSTDuLieuVeCloudDeHienThiTrenAPP();
}
#ifdef xTaskPOSTDuLieuVeCloud
// Cập nhật dữ liệu về cloud để hiển thị trên APP.
void POSTDuLieuVeCloudDeHienThiTrenAPP(void) {
#pragma region POSTDuLieuVeCloudDeHienThiTrenAPP


#ifdef CoKetNoiServer
  if (WiFi.status() == WL_CONNECTED) {
#pragma region Chuỗi dữ liệu sẽ gửi về server
    // Lấy thời gian thực trên board.
    _RTC.LayRTCTuDS3231();

    Serial.println("Doc cam bien");
    // Đọc giá trị cảm biến nhiệt độ và độ ẩm SHT31.
    _SHT31.DocCamBienNhietDoVaDoAmSHT31();


    //===================================================================
    // Chuỗi dulieu sẽ gồm: K1K2K3K4MODERSSI;NhietDo;DoAm;RTC
    // Ví dụ: 1000077;23.5;82.9;12:26:13 12/04/2024
    // Nghĩa là:
    //  K1 = 1, K2 = 0, K3 = 0, K4 = 0, MODE = 0 (MAN)
    // RSSI = 77
    // Nhiệt độ = 23,5
    // Độ ẩm = 82,9
    // RTC = 12/04/2024 12:26:13
    //===================================================================
    //===================================================================
    // s00: K1 = 1 : Relay 1 điều khiển ON/OFF K1 đang ON
    // s01: K2 = 0 : Relay 2 điều khiển ON/OFF K2 đang OFF
    // s02: K3 = 0 : Relay 2 điều khiển ON/OFF K3 đang OFF
    // s03: K4 = 0 : Relay 2 điều khiển ON/OFF K4 đang OFF
    // s04: MODE = 0 : Chế độ điều khiển đang là MANUAL (bằng tay).
    // s05: Độ mạnh WiFI = 77%
    // s1: Nhiệt độ = 23.5 (độ C)
    // s2: Độ ẩm = 82.9 (%RH)
    // s3: Thời gian thực RTC trên board = 12:26:13 12/04/2024
    String s00 = String(_Relay.K1);
    String s01 = String(_Relay.K2);
    String s02 = "0"; // Do board này không có relay K3
    String s03 = "0"; // Do board này không có relay K4
    String s04 = String(_Relay.MODE);
    String s05 = String(_WiFi.TinhDoManhCuaWiFi());
    String s1 = String(_SHT31.NhietDo);
    String s2 = String(_SHT31.DoAm);
    String s3 = _RTC.ChuanHoaChuoiRTCDeGuiVeServer();
    String data = s00+s01+s02+s03+s04+s05+";"+s1+";"+s2+";"+s3;
    //-----------------------------------------------------------------------------------------
#pragma endregion Chuỗi dữ liệu sẽ gửi về server


#ifdef debug
    Serial.println("Dữ liệu POST lên server:");
    Serial.println(data);
#endif


    _POSTGET.POSTDuLieuBoard(_ID, data);
  } else {
#ifdef debug
    Serial.println("Không có WIFI để POST dữ liệu lên server...");
#endif
  }
#endif  // CoKetNoiServer


#pragma endregion POSTDuLieuVeCloudDeHienThiTrenAPP
}
#endif  // xTaskPOSTDuLieuVeCloud
//=========================================================================//
// Trả về true nếu có thay đổi trạng thái làm việc của các K1, K2.
// Ngược lại sẽ trả về false.
bool BoardLamViec(void) {
  #pragma region BoardLamViec
    bool ONOFF_K1 = false; // Xác định K1 có chuyển trạng thái ON|OFF hay không.
    bool ONOFF_K2 = false; // Xác định K2 có chuyển trạng thái ON|OFF hay không.
  
  
      // Chỉ thực thi lệnh điều khiển cho kênh 1 khi chế độ là MANUAL.
      //-----------------------------------------------------------------------
      if (_Relay.MODE == _MANUAL) {
        ONOFF_K1 = _Relay.ONOFFBangTayK1(_LenhONOFFK1);
    ONOFF_K2 = _Relay.ONOFFBangTayK2(_LenhONOFFK2);
      }
      //-----------------------------------------------------------------------
  
  
  #ifdef debug
    Serial.println("===============================================");
    Serial.println("Board làm việc:");
    Serial.print("_Relay.K1: ");
    Serial.println(_Relay.K1);
    Serial.print("_Relay.K2: ");
    Serial.println(_Relay.K2);
    Serial.print("_Relay.MODE: ");
    Serial.println(_Relay.MODE);
    Serial.println("===============================================");
  #endif
  
  
    // Trả về TRUE khi có sự thay đổi trạng thái đóng/ngắt của 1 trong 2 rơle.
    if (ONOFF_K1 == true || ONOFF_K2 == true) {
      return true;
    }
    return false;
    #pragma endregion BoardLamViec
    }
    //=========================================================================//
    void CapNhatCODETrenServer(String code) {
    #pragma region CapNhatCODETrenServer
      // Định dạng của chuỗi dữ liệu gửi lên cloud là:
      // K1K2K3K4MODECODE, ex: 11000 nghĩa là:
      // K1 = 1, K2 = 1, K3 = 0, K4 = 0, MODE = 0 (MAN), CODE = 0
      String str = String(_Relay.K1) + String(_Relay.K2) + "0" + "0" + String(_Relay.MODE) + code;
      _POSTGET.CapNhatCODETrongDatabaseTrenServer(_ID, str);
    
    
    #ifdef debug
      Serial.println(str);
    #endif
    #pragma endregion CapNhatCODETrenServer
    }
    //=========================================================================//
    void ThucThiTacVuTheoCODE(void) {
    #pragma region ThucThiTacVuTheoCODE
    //-----------------------------------------------------------------//
    #pragma region CODE = CODEUserChonONOFF : Thực hiện điều khiển chạy / dừng bằng nút ON / OFF trên app.
      if (_CODE == CODEUserChonONOFF) {
        _CODE = 0;
    
    
        // Gửi CODE=0 về server để app biết board đã nhận được lệnh mã CODE thành công.
        String CODEGuiVeServe = "0";
        CapNhatCODETrenServer(CODEGuiVeServe);
    
    
    #ifdef debug
        Serial.println("User bấm nút chọn ON|OFF trên app!");
    #endif
    
    
        // Trả về TRUE khi có sự thay đổi trạng thái đóng/ngắt của 1 trong 2 rơle, hoặc mode1 hoặc mode2 thay đổi.
        bool CoChuyenTrangThaiONOFF = BoardLamViec();
        if (CoChuyenTrangThaiONOFF == true) {
          // Bật cờ cho phép POST dữ liệu lên cloud để hàm POSTDuLieuVeCloudDeHienThiTrenAPP() được phép
          // thực thi trong TaskPOSTDuLieuVeCloud
          _ChoPhepPOSTDuLieu = true;
    
    
    #ifdef debug
          Serial.print("Đang POST dữ liệu lên server ");
          Serial.println("do user bấm ON/OFF trên app và có sự thay trạng thái ON/OFF của các rơ-le...");
    #endif
        }
      }
    #pragma endregion CODE = CODEUserChonONOFF : Thực hiện điều khiển chạy / dừng bằng nút ON / OFF trên app.
    //-----------------------------------------------------------------//
    #pragma endregion ThucThiTacVuTheoCODE
    }
    //=========================================================================//
    void LangNgheLenhAppGuiXuongBoard(void) {
    #pragma region LangNgheLenhAppGuiXuongBoard
    #ifdef debug
      Serial.println("Lắng nghe lệnh từ app xuống board...");
      #endif


  // GET dữ liệu từ app xuống board, gồm trạng thái các nút điều khiển, chế độ làm việc cho từng kênh, mã CODE.
  String strLenhGuiXuongBoard = _POSTGET.GETLenhGuiXuongBoard(_ID);


  if (strLenhGuiXuongBoard.length() > 0) {
#pragma region Lấy lệnh gửi từ app xuống board thành công


    JSONVar json = JSON.parse(strLenhGuiXuongBoard);
    String S = (const char*)json["S"];
    int CODE = S.substring(5).toInt();


#ifdef debug
    Serial.print("CODE gửi xuống board: ");
    Serial.println(CODE);
#endif


    // Lưu ý: Những mã code 20, 21, 30, 31, 40, 41, 50, 51 là trên app IoTVision có định nghĩa, các em không cần quan tâm đến các mã code này nhé.
    if (CODE != 0 & CODE != 20 & CODE != 21 & CODE != 30 & CODE != 31 & CODE != 40 & CODE != 41 & CODE != 50 & CODE != 51) {
      _CODE = CODE;


_LenhONOFFK1 = S.substring(0, 1).toInt(); //Lấy lệnh điều khiển đóng ngắt K1.
_LenhONOFFK2 = S.substring(1, 2).toInt(); //Lấy lệnh điều khiển đóng ngắt K2.


#ifdef debug
      Serial.print("Lệnh điều khiển K1 gửi từ app xuống board: ");
      Serial.println(_LenhONOFFK1);
      Serial.print("Lệnh điều khiển K2 gửi từ app xuống board: ");
      Serial.println(_LenhONOFFK2);
#endif


      ThucThiTacVuTheoCODE();
    }
#pragma endregion Lấy lệnh gửi từ app xuống board thành công
  } else {
#ifdef debug
    Serial.println("Lỗi GET lệnh từ app xuống board hoặc không có WIFI");
#endif
  }
#pragma endregion LangNgheLenhAppGuiXuongBoard
}
  