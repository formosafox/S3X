// ************************************************************************
// 修改日期：2017-05-22
// ************************************************************************
#ifndef Blink_H
#define Blink_H
// ************************************************************************
const byte Blink_Idle = 0;                // 閃爍：閒置
const byte Blink_ON   = 1;                // 閃爍：亮
const byte Blink_OFF  = 2;                // 閃爍：暗
// ************************************************************************
class Blink { 
  private :
    //-------------------------------------------------------
    byte _Mode = Blink_Idle;
    //-------------------------------------------------------
  public :
    //-------------------------------------------------------
    Blink(void);
    ~Blink(void);
    //-------------------------------------------------------
    void Update(void);
    byte GetLowHigh(void) {return(_Mode==Blink_ON?HIGH:LOW);};
    boolean isON(void) {return(_Mode==Blink_ON?true:false);};
    boolean isActive(void) {return(_Mode==Blink_Idle?false:true);};
    void Active(void) {if (_Mode==Blink_Idle) _Mode=Blink_ON;};
    //-------------------------------------------------------
};
#endif
