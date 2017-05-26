// ************************************************************************
// 修改日期：2017-05-26
// ************************************************************************
// For Arduino 1.0 and earlier
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
// ************************************************************************
#include "SysMode.h"
// ************************************************************************
// SysMode：建構
SysMode::SysMode(unsigned long IdleTime) {
  TimeOut = IdleTime;
}
// ************************************************************************
// SysMode：解構
SysMode::~SysMode(void){/*nothing to destruct*/}
// ************************************************************************
// ************************************************************************
byte SysMode::Check_WakeUp_Sleep(void) {
  switch (_Mode) {
    case SysMode_Idle     :
        if ((millis()-LiveTime) < TimeOut) _Mode = SysMode_doWakeUp;
        break;
    case SysMode_doWakeUp :
        break;
    case SysMode_Normal   :
        if ((millis()-LiveTime) >= TimeOut) _Mode = SysMode_doSleep;
        break;
    case SysMode_doSleep  :
        break;
  }
  return(_Mode);
}
// ************************************************************************
void SysMode::Live(void) {
  switch (_Mode) {
    case SysMode_Idle     :
        LiveTime = millis();
        break;
    case SysMode_doWakeUp :
        break;
    case SysMode_Normal   :
        LiveTime = millis();
        break;
    case SysMode_doSleep  :
        break;
  }
}
// ************************************************************************
void SysMode::To_Normal_Idle(void) {
  switch (_Mode) {
    case SysMode_Idle     :
        break;
    case SysMode_doWakeUp :
        _Mode = SysMode_Normal;
        LiveTime = millis();
        break;
    case SysMode_Normal   :
        break;
    case SysMode_doSleep  :
        _Mode = SysMode_Idle;
        break;
  }
}
// ************************************************************************
