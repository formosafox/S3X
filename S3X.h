// ************************************************************************
// 修改日期：2017-05-24
// ************************************************************************
#ifndef S3X_H
#define S3X_H
// ************************************************************************
#define RS485_Transmit        HIGH                // RS-485 資料傳送
#define RS485_Receive         LOW                 // RS-485 資料接收
//-------------------------------------------------------
const byte S3X_S31  = 0;                          // 模擬設備的索引：S31
const byte S3X_S32  = 1;                          // 模擬設備的索引：S32
const byte S3X_S33  = 2;                          // 模擬設備的索引：S33
//-------------------------------------------------------
const byte S3X_LogMode = 0;                       // 運作模式：紀錄模式
const byte S3X_SimMode = 1;                       // 運作模式：模擬模式
//-------------------------------------------------------
const word PacketCheck_Start      = 0xFFFF;       // 封包檢查：開始
const word PacketCheck_Finish     = 0xEEEE;       // 封包檢查：完成
const word PacketCheck_Error      = 0xDDDD;       // 封包檢查：資料錯誤
//-------------------------------------------------------
const byte PacketMode_Start       = 1;            // 封包模式：開始
const byte PacketMode_Finish      = 2;            // 封包模式：完成
const byte PacketMode_Error       = 3;            // 封包模式：資料錯誤
const byte PacketMode_Idle        = 0;            // 封包模式：閒置
//-------------------------------------------------------
const byte StatusBit_1_Unknow              = 0;   // 系統狀態位元：1：未知
const byte StatusBit_2_LowWater            = 1;   // 系統狀態位元：2：水位過低
const byte StatusBit_3_ForcedHeatingMode   = 2;   // 系統狀態位元：3：強制加熱模式
const byte StatusBit_4_Heating             = 3;   // 系統狀態位元：4：加熱中
const byte StatusBit_5_Unknow              = 4;   // 系統狀態位元：5：未知
const byte StatusBit_6_CanNotHeating       = 5;   // 系統狀態位元：6：無法加熱
const byte StatusBit_7_TemperatureAbnormal = 6;   // 系統狀態位元：7：溫度異常
const byte StatusBit_8_ControlBoxOffline   = 7;   // 系統狀態位元：8：控制箱未連線
//-------------------------------------------------------
const byte HeadingCommand_None    = 0x2A;         // 加熱命令：無
const byte HeadingCommand_Active  = 0xAA;         // 加熱命令：強制加熱
//-------------------------------------------------------
const unsigned long P31_S3X_TimeOut = 3 * 1000;   // 連線愈時：3 秒內沒資料
//-------------------------------------------------------
const byte S3X_PacketLength=20;                   // S3X 封包長度
// ************************************************************************
class S3X { 
  private :
    //-------------------------------------------------------
    byte _S3X_RunModeIndex = S3X_LogMode;         // 運作模式：紀錄模式、模擬模式
    byte _S3X_Index = S3X_S31;                    // 要模擬的 S3X 設備索引
    byte _Sim_ForcedHeating = HeadingCommand_None;// S3X 模擬：強制加熱
    byte _Sim_SetTemperature = 0x00;              // S3X 模擬：水溫設定  回應 00：不變，30~75：更改
    //-------------------------------------------------------
    unsigned long _DataInputTime;                 // 時間：當有輸入資料
    //-------------------------------------------------------
    boolean _PacketFinish;                        // 封包是否已完成
    boolean _PacketError;                         // 封包是否有錯誤
    //-------------------------------------------------------
    boolean _DataUpdate = false;                  // 封包完成後比對前一筆是否有更新
    unsigned long _DataUpdateTime;                // 時間：當資料有更新
    //-------------------------------------------------------
    unsigned long _PacketTotalCount = 0;          // 封包總數計數
    unsigned long _PacketErrorCount = 0;          // 封包錯誤計數
    //-------------------------------------------------------
    byte _TempData[S3X_PacketLength];             // 接收完資料後經比對資料需更新時【置換】此處資料
    //-------------------------------------------------------
    void PacketDataInput(byte BD);                // 方法：封包資料輸入
    void ReceivedData_UpdateTo_TempData(void);
    void ReceivedData_Clear(void);
    void PollingCommand_ReScanMode(void);
    void ReceivedData_Start(byte BD);
    void ReceivedData_Add(byte BD);
    word S3X_Slave_SimMode(byte BD, word CheckStep);
    word S3X_Slave_LogMode(byte BD, word CheckStep);
    void FunctionCode_ResponseData(byte Data , byte Count);
    //-------------------------------------------------------
  public :
    //-------------------------------------------------------
    S3X(void);
    ~S3X(void);
    //-------------------------------------------------------
    byte RunModeIndex(void) {return(_S3X_RunModeIndex);};
    void RunModeIndex(byte RunModeIndex) {_S3X_RunModeIndex = (RunModeIndex == S3X_LogMode ? S3X_LogMode : S3X_SimMode);};
    void IdentIndex(byte S3X_Index);                                                  // 設定：對應的 S3X 設備：S3X_S31、S3X_S32、S3X_S33
    byte IdentIndex(void) {return(_S3X_Index);};                                      // 取得：對應的 S3X 設備：S3X_S31、S3X_S32、S3X_S33
    void ResetData(void);                                                             // 方法：清除 TempData 資料
    //-------------------------------------------------------
    void begin(Stream* RS485_Port, unsigned long RS485_Speed);                        //  S3X：開始運行
    void Clear_ReceiveBuffer(void);                                                   // 清空已接收的資料
    void Run(void);                                                                   // 放入 loop() 內執行
    //-------------------------------------------------------
    // 方法：模擬強制加熱模式 
    void Sim_ForcedHeating(void) {_Sim_ForcedHeating=(_S3X_RunModeIndex==S3X_LogMode?HeadingCommand_None:HeadingCommand_Active);};         
    void Sim_SetTemperature(byte TMP) {_Sim_SetTemperature=constrain(TMP, 30, 75);};  // 方法：模擬水溫設定 
    //-------------------------------------------------------
                                                                                      // 取得：等待資料輸入是否愈時
    boolean Link_TimeOut(void) {return(millis()-_DataInputTime>=P31_S3X_TimeOut?true:false);}; 
    //-------------------------------------------------------
    boolean PacketFinish(void) {return(_PacketFinish);};                              // 取得：封包是否已完成
    boolean PacketError(void) {return(_PacketError);};                                // 取得：封包是否有錯誤
    //-------------------------------------------------------
    boolean DataUpdate(void) {return(_DataUpdate);};                                  // 取得：封包完成後比對前一筆是否有更新
    //-------------------------------------------------------
    unsigned long PacketTotalCount(void) {return(_PacketTotalCount);};                // 取得：封包總數計數
    unsigned long PacketErrorCount(void) {return(_PacketErrorCount);};                // 取得：封包錯誤計數
    //-------------------------------------------------------
    byte NowTemperature(void) {return(_TempData[12]);};                               // 取得：目前水溫
    byte SetTemperature(void) {return(_TempData[15]);};                               // 取得：水溫設定
    byte SystemStatusByte(void) {return(_TempData[18]);};                             // 取得：系統狀態(Byte)
    byte SystemStatusBit(byte index) {return((_TempData[18]>>index) & 0x01);};        // 取得：系統狀態(Bit) index=0~7
    //-------------------------------------------------------
};
#endif
