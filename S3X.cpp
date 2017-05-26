// ************************************************************************
// 修改日期：2017-05-24
// ************************************************************************
// For Arduino 1.0 and earlier
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
// ************************************************************************
#include "S3X.h"
// ************************************************************************
//-------------------------------------------------------
byte PacketModeStatus = PacketMode_Idle;      // 封包模式狀態
word PacketCheckStep;                         // 目前封包檢查步驟
byte PollingCommand[3] = {0x88, 0x77, 0x66};  // S3X 設備輪詢命令
byte SlaveID[3] = {0x31, 0x32, 0x33};         // 設備ID
//-------------------------------------------------------
byte ReceivedData[S3X_PacketLength];          // ComPort 接收的資料
byte ReceivedData_Count;                      // ComPort 接收的資料個數
//-------------------------------------------------------
Stream* RS485;                                // RS485 通訊埠
boolean S3X_Begin = false;                    // 避免忘記呼叫 S3X::begin 後續呼叫 S3X::Run 而造成一直重開機
//-------------------------------------------------------
unsigned long T_1_5;                          // inter character time out
unsigned long T_3_0;                          // frame delay
unsigned long T_3_5;                          // frame delay
unsigned long T_S3X;                          // 整個 S3X Packet 完成時間
// ************************************************************************
// S3X：建構
S3X::S3X() {
  _DataInputTime = millis();
  _DataUpdateTime = millis();
  memset(_TempData,0,S3X_PacketLength);
}
// ************************************************************************
// S3X：解構
S3X::~S3X(void){/*nothing to destruct*/}
// ************************************************************************
// S3X：設定
void S3X::IdentIndex(byte S3X_Index) {
  if (S3X_Index == S3X_S31 || S3X_Index == S3X_S32 || S3X_Index == S3X_S33) 
        _S3X_Index=S3X_Index;
  else  _S3X_Index=S3X_S31;
  PacketModeStatus = PacketMode_Idle;
}
// ************************************************************************
// S3X：清除 TempData 資料
void S3X::ResetData(void) {
  PollingCommand_ReScanMode();
  memset(_TempData,0,S3X_PacketLength);
}
// ************************************************************************
//  S3X：開始運行
void S3X::begin(Stream* RS485_Port, unsigned long RS485_Speed) {
  //-------------------------------------------------------
  // T1.5 與 T3.5 用法參考自 Modbus RTU libraries for Arduino 
  // https://sites.google.com/site/jpmzometa/arduino-mbrt/arduino-modbus-slave
  // https://code.google.com/archive/p/simple-modbus/
  // http://forum.arduino.cc/index.php?topic=176142.0
  // https://drive.google.com/drive/folders/0B0B286tJkafVSENVcU1RQVBfSzg
  //-------------------------------------------------------
  // Modbus states that a baud rate higher than 19200 must use a fixed 750 us 
  // for inter character time out and 1.75 ms for a frame delay for baud rates
  // below 19200 the timing is more critical and has to be calculated.
  // E.g. 9600 baud in a 10 bit packet is 960 characters per second
  // In milliseconds this will be 960characters per 1000ms. So for 1 character
  // 1000ms/960characters is 1.04167ms per character and finally modbus states
  // an inter-character must be 1.5T or 1.5 times longer than a character. Thus
  // 1.5T = 1.04167ms * 1.5 = 1.5625ms. A frame delay is 3.5T.
  //-------------------------------------------------------
  if (RS485_Speed > 19200) {
    T_1_5 = 750; 
    T_3_0 = T_1_5 * 2;
    T_3_5 = 1750; 
  } else {
    T_1_5 = 15000000/RS485_Speed; // 1T * 1.5 = T1.5
    T_3_0 = T_1_5 * 2;
    T_3_5 = 35000000/RS485_Speed; // 1T * 3.5 = T3.5
  }
  //-------------------------------------------------------
  T_S3X = (T_3_0 /1000) * S3X_PacketLength; // 轉為毫秒(ms)*封包長度
  //-------------------------------------------------------
  RS485 = RS485_Port;
  //-------------------------------------------------------
  Clear_ReceiveBuffer();  // 清空已接收的資料
  //-------------------------------------------------------
  S3X_Begin = true;                                   // 避免忘記呼叫 S3X::begin 後續呼叫 S3X::Run 而造成一直重開機
  //-------------------------------------------------------
}
// ************************************************************************
// 清空已接收的資料
void S3X::Clear_ReceiveBuffer(void) {
  if (RS485->available() == 0) return;    // 先用 available() 判斷，免得有時會稍稍卡一下
  while (RS485->read() != -1);            // 清空已接收的資料
  _DataInputTime = millis();              // 更新時間：當有輸入資料
}
// ************************************************************************
// S3X：執行接收資料
void S3X::Run(void) {
  boolean WaitFinish=false;   // 等待完成模式
  unsigned long StartTime;
  byte RB;
  //-------------------------------------------------------
  _PacketFinish = false;
  _PacketError = false;
  //-------------------------------------------------------
  if (S3X_Begin != true) return;                  // 避免忘記呼叫 S3X::begin 後續呼叫 S3X::Run 而造成一直重開機
  //-------------------------------------------------------
  if (RS485->available() == 0) return;
  //-------------------------------------------------------
  do {
    while (RS485->available()>0) {
      //-------------------------------------------------------
      _DataInputTime = millis();                          // 更新時間：當有輸入資料
      //-------------------------------------------------------
      RB=RS485->read();
      delayMicroseconds(T_1_5);  // inter character time out
      PacketDataInput(RB);
      if (RB==0x00 && ReceivedData_Count>=5) { // 功能【00】查詢子機代碼 & 已傳回代碼：2+1+2=5
        StartTime = millis();                             // 紀錄時間點
        WaitFinish=true;                                  // 啟動等待完成模式
      }
      if (_PacketFinish==true) WaitFinish=false;
    }
    if (WaitFinish==true) {                               // 等待完成模式
      if ((millis()-StartTime)>=T_S3X) WaitFinish=false;  // 超過時間未完成時，關閉等待完成模式
    }
  } while (WaitFinish);
}
// ************************************************************************
// 封包資料輸入
void S3X::PacketDataInput(byte BD) {
  //-------------------------------------------------------   
  if (PacketModeStatus == PacketMode_Idle) {
      if (BD == PollingCommand[_S3X_Index]) {                   // 檢查是否為模擬設備對應的輪詢指令
          ReceivedData_Start(BD);                               // 開始接收資料至 ReceivedData
      }
  } else {
      if (_S3X_RunModeIndex == S3X_LogMode)
            PacketCheckStep = S3X_Slave_LogMode(BD, PacketCheckStep);
      else  PacketCheckStep = S3X_Slave_SimMode(BD, PacketCheckStep);
      //-------------------------------------------------------
      if (PacketCheckStep == PacketCheck_Error) {               // 當封包資料分析結果是錯誤時
          if (ReceivedData_Count > 3) {                        // 檢查是否完整輪詢指令後才錯誤
              _PacketTotalCount++;
              _PacketErrorCount++;
              _PacketError = true;
          }
          //-------------------------------------------------------
          if (BD == PollingCommand[_S3X_Index])                 // 檢查是否為模擬設備對應的輪詢指令
                ReceivedData_Start(BD);                         // 開始接收資料至 ReceivedData
          else  PollingCommand_ReScanMode();                    // 重新掃描輪詢命令模式
        //-------------------------------------------------------
      } else {
          if (PacketCheckStep == PacketCheck_Finish) {          // 封包接收完畢
              _PacketFinish = true;
              ReceivedData_UpdateTo_TempData();                 // 更新資料至 TempData
              PollingCommand_ReScanMode();                      // 重新掃描輪詢命令模式
          }
      }
  }
  //-------------------------------------------------------
}
// ************************************************************************
// Received 資料 Update 至 Temp 資料
void S3X::ReceivedData_UpdateTo_TempData(void) {
  _DataUpdate = false;
  _PacketTotalCount++;
  if (memcmp(_TempData, ReceivedData, ReceivedData_Count) != 0) {  // 比對 TempData 與 ReceivedData 是否相同：相同時=0
    memcpy(_TempData, ReceivedData, ReceivedData_Count);           // 當資料不同時更新至 Temp 資料
    _DataUpdateTime = millis();
    _DataUpdate = true;
  }
}
// ************************************************************************
// 重新掃描輪詢命令模式
void S3X::PollingCommand_ReScanMode(void) {
  ReceivedData_Clear();
  PacketModeStatus = PacketMode_Idle;
}
// ************************************************************************
// ReceivedData 開始接收資料
void S3X::ReceivedData_Start(byte BD) {
  ReceivedData_Clear();
  ReceivedData_Add(BD);
  PacketModeStatus = PacketMode_Start;
  PacketCheckStep = PacketCheck_Start;
}
// ************************************************************************
// 清空 ReceivedData 資料並將計數器歸零
void S3X::ReceivedData_Clear(void) {
  memset(ReceivedData,0,S3X_PacketLength);
  ReceivedData_Count=0;
}
// ************************************************************************
// 接收資料加入 ReceivedData
void S3X::ReceivedData_Add(byte BD) {
  if (ReceivedData_Count>=S3X_PacketLength) return;
  ReceivedData[ReceivedData_Count]=BD;
  ReceivedData_Count++;
}
// ************************************************************************
// S3X 設備：模擬模式
word S3X::S3X_Slave_SimMode(byte BD, word CheckStep ) {
  word NextStep = PacketCheck_Error;
  //-------------------------------------------------------
  switch (CheckStep) {
    case PacketCheck_Start :
      if (BD == PollingCommand[_S3X_Index]) { // 檢查 PollingCommand 內容是否相符
        ReceivedData_Add(BD);
        NextStep = 0x0000;
      }
      break;
    case 0x0000 :
      if (BD == 0x00) {                       // 功能【00】：查詢子機代碼
        ReceivedData_Add(BD);                 // 儲存【00】
                                              // 回應子機代碼
        FunctionCode_ResponseData(SlaveID[_S3X_Index], 2); 
        NextStep = 0x1100;
      }
      break;
    case 0x1100 :
      if (BD == 0x11) {                       // 功能【11】：查詢子機強制加熱按鈕狀態
        ReceivedData_Add(BD);                 // 儲存【11】
                                              // 回應加熱按鈕狀態
        FunctionCode_ResponseData(_Sim_ForcedHeating, 2); 
        if (_Sim_ForcedHeating == HeadingCommand_Active) {  // 檢查是否強制加熱動作
          _Sim_ForcedHeating = HeadingCommand_None;         // 將值重設一般免得一直在送出強制加熱動作
        }
        NextStep = 0x1500;
      }
      break;
    case 0x1500 :
      if (BD == 0x15) {                       // 功能【15】：查詢子機更改水溫
        ReceivedData_Add(BD);                 // 儲存【15】
                                              // 回應 00：不變 30~75：更改
        FunctionCode_ResponseData(_Sim_SetTemperature, 2); 
        if (_Sim_SetTemperature != 0x00) {    // 檢查是否更改設定溫度
          _Sim_SetTemperature = 0x00;         // 將值歸零免得一直在送出要更改主機的設定水溫
        }
        NextStep = 0x2200;
      }
      break;
    case 0x2200 :
      if (BD == 0x22) {                       // 功能【22】：主機(P31)現在水溫值
        ReceivedData_Add(BD);                 // 儲存【22】
        NextStep = 0x2201;
      }
      break;
    case 0x2201 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)現在水溫值【1】
      FunctionCode_ResponseData(BD, 1);       // 回應收到的現在水溫值給主機(P31)
      NextStep = 0x2500;
      break;
    case 0x2500 :
      if (BD == 0x25) {                       // 功能【25】：主機(P31)設定水溫值
        ReceivedData_Add(BD);                 // 儲存【25】
        NextStep = 0x2501;
      }
      break;
    case 0x2501 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)設定水溫值【1】
      FunctionCode_ResponseData(BD, 1);       // 回應收到的設定水溫值給主機(P31)
      NextStep = 0x3300;
      break;
    case 0x3300 :
      if (BD == 0x33) {                       // 功能【33】：主機(P31)系統狀態值
        ReceivedData_Add(BD);                 // 儲存【33】
        NextStep = 0x3301;
      }
      break;
    case 0x3301 :
      ReceivedData_Add(BD);
      FunctionCode_ResponseData(BD, 1);       // 回應收到的系統狀態值給主機(P31)
      NextStep = PacketCheck_Finish;          // 接收完畢
      break;
    default :
      break;
  }
  return (NextStep);
}
// ************************************************************************
// S3X 設備：紀錄模式
word S3X::S3X_Slave_LogMode(byte BD, word CheckStep ) {
  word NextStep = PacketCheck_Error;
  //-------------------------------------------------------
  switch (CheckStep) {
    case PacketCheck_Start :
      if (BD == PollingCommand[_S3X_Index]) { // 檢查 PollingCommand 內容是否相符
        ReceivedData_Add(BD);
        NextStep = 0x0000;
      }
      break;
    case 0x0000 :
      if (BD == 0x00) {                       // 功能【00】：查詢子機代碼
        ReceivedData_Add(BD);                 // 儲存【00】
        NextStep = 0x0001;
      }
      break;
    case 0x0001 :
      if (BD == SlaveID[_S3X_Index]) {        // ID 第1碼有符合
        ReceivedData_Add(BD);                 // 儲存子機代碼【1】
        NextStep = 0x0002;
      }
      break;
    case 0x0002 :
      if (BD == SlaveID[_S3X_Index]) {        // ID 第2碼有符合
        ReceivedData_Add(BD);                 // 儲存子機代碼【2】
        NextStep = 0x1100;
      }
      break;
    case 0x1100 :
      if (BD == 0x11) {                       // 功能【11】：查詢子機強制加熱按鈕狀態
        ReceivedData_Add(BD);                 // 儲存【11】
        NextStep = 0x1101;
      }
      break;
    case 0x1101 :
      if (BD == HeadingCommand_None || BD == HeadingCommand_Active) {  // 符合指令
        ReceivedData_Add(BD);                 // 儲存子機強制加熱按鈕狀態【1】
        NextStep = 0x1102;
      }
      break;
    case 0x1102 :
      if (BD == HeadingCommand_None || BD == HeadingCommand_Active) {  // 符合指令
        ReceivedData_Add(BD);                 // 儲存子機強制加熱按鈕狀態【2】
        NextStep = 0x1500;
      }
      break;
    case 0x1500 :
      if (BD == 0x15) {                       // 功能【15】：查詢子機更改水溫
        ReceivedData_Add(BD);                 // 儲存【15】
        NextStep = 0x1501;
      }
      break;
    case 0x1501 :
      ReceivedData_Add(BD);                   // 儲存子機更改水溫值【1】
      NextStep = 0x1502;
      break;
    case 0x1502 :
      ReceivedData_Add(BD);                   // 儲存子機更改水溫值【2】
      NextStep = 0x2200;
      break;
    case 0x2200 :
      if (BD == 0x22) {                       // 功能【22】：主機(P31)現在水溫值
        ReceivedData_Add(BD);                 // 儲存【22】
        NextStep = 0x2201;
      }
      break;
    case 0x2201 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)現在水溫值【1】
      NextStep = 0x2202;
      break;
    case 0x2202 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)現在水溫值【2】
      NextStep = 0x2500;
      break;
    case 0x2500 :
      if (BD == 0x25) {                       // 功能【25】：主機(P31)設定水溫值
        ReceivedData_Add(BD);                 // 儲存【25】
        NextStep = 0x2501;
      }
      break;
    case 0x2501 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)設定水溫值【1】
      NextStep = 0x2502;
      break;
    case 0x2502 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)設定水溫值【2】
      NextStep = 0x3300;
      break;
    case 0x3300 :
      if (BD == 0x33) {                       // 功能【33】：主機(P31)系統狀態值
        ReceivedData_Add(BD);                 // 儲存【33】
        NextStep = 0x3301;
      }
      break;
    case 0x3301 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)系統狀態值【1】
      NextStep = 0x3302;
      break;
    case 0x3302 :
      ReceivedData_Add(BD);                   // 儲存主機(P31)系統狀態值【2】
      NextStep = PacketCheck_Finish;          // 接收完畢
      break;
    default :
      break;
  }
  return (NextStep);
}
// ************************************************************************
// 功能碼資料回應
void S3X::FunctionCode_ResponseData(byte Data , byte Count) {
    for (byte CX = 0; CX < Count; CX++) {
      //-------------------------------------------------------
      ReceivedData_Add(Data);    // 存入回應資料
      //-------------------------------------------------------
      //delayMicroseconds(T_1_5);  // inter character time out
      RS485->write(Data); // 功能代碼回應資料
      RS485->flush();     // 等待資料傳送完畢：https://www.baldengineer.com/when-do-you-use-the-arduinos-to-use-serial-flush.html
      delayMicroseconds(T_3_0);  // allow a frame delay to indicate end of transmission
      //-------------------------------------------------------
    }
}
// ************************************************************************ 
