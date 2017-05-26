// ************************************************************************
// 修改日期：2017-05-26
// ************************************************************************
#ifndef SysMode_H
#define SysMode_H
// ************************************************************************
const byte SysMode_Idle        = 0;             // 系統：閒置模式
const byte SysMode_doWakeUp    = 1;             // 系統：執行喚醒
const byte SysMode_Normal      = 2;             // 系統：一般模式
const byte SysMode_doSleep     = 3;             // 系統：執行睡眠
// ************************************************************************
class SysMode { 
  private :
    //-------------------------------------------------------
    unsigned long LiveTime;
    unsigned long TimeOut;
    byte _Mode = SysMode_Normal;
    //-------------------------------------------------------
  public :
    //-------------------------------------------------------
    SysMode(unsigned long IdleTime);
    ~SysMode(void);
    //-------------------------------------------------------
    byte Mode(void) {return(_Mode);};		// 系統模式
    byte Check_WakeUp_Sleep(void);		// 檢查 doWakeUp、doSleep 模式
    void Living(void);				// 活動中
    void To_Normal_Idle(void);			// 由 doWakeUp->Normal 或 doSleep-> Idle
    //-------------------------------------------------------
};
#endif
