#include "Arduino.h"

class XPort
{
public:
    int step = 0;
    int index;
    int pinNumber;
    long T1;
    long T2;
    float fps;

    long RemainMs;
    bool isOn;
    long count;
    long countMax;

    bool ExternTrigger;
    bool Power;

    XPort(){

    };

    XPort(int index, int pinNumber)
    {
        this->index = index;
        this->pinNumber = pinNumber;
        this->step = 0;
        pinMode(pinNumber, OUTPUT);
    };

    XPort(float FPS, float DutyRatio, long count)
    {
        int timePeriodMs = 1000 / FPS;
        this->T1 = round(timePeriodMs * (1 - DutyRatio));
        this->T2 = timePeriodMs - T1;
        this->RemainMs = T1;
        this->isOn = false;
        this->count = count;
        this->countMax = count;
        this->fps = FPS;
    };

    void goStep()
    {
        if (count == 0)
        {
            return;
        }
        switch (step)
        {
        case 0:
            step = 1;
            isOn = true;
            RemainMs = T2;
            digitalWrite(pinNumber, HIGH);
            break;
        case 1:
            step = 0;
            isOn = false;
            RemainMs = T1;
            digitalWrite(pinNumber, LOW);
            if (--count == 0)
            {
                RemainMs = 0;
            }
            break;
        }
    };
};

#define MAX_XPORT 4

class XBus
{
public:
    XPort XPorts[MAX_XPORT];
    long minRemainTime;

    XBus(uint8 pinNumber[])
    {
        for (int i = 0; i < MAX_XPORT; i++)
        {
            XPorts[i] = XPort(i, pinNumber[i]);
        }
    }

    void addXPort(float FPS, float DutyRatio, long count = -1)
    {
        //  XPorts.push_back(XPort(FPS, DutyRatio, count));
    }

    void addXPort_T2(float FPS, long T2Time, long count = -1)
    {
        long timePeriodMs = round(1000.0 / FPS);
        long T1 = timePeriodMs - T2Time;
        float DutyRatio = 1 - (float)T1 / timePeriodMs;
        //  XPorts.push_back(XPort(FPS, DutyRatio, count));
    }

    void updateXPort(int index, float FPS, float DutyRatio, long count = -1)
    {
        XPorts[index] = XPort(FPS, DutyRatio, count);
    }

    void updateXPort_T2(int index, float FPS, long T2Time, long count = -1, bool externTrigger = false, bool power = false)
    {
        long timePeriodMs = round(1000.0 / FPS);
        long T1 = timePeriodMs - T2Time;
        float DutyRatio = 1 - (float)T1 / timePeriodMs;
        XPorts[index] = XPort(FPS, DutyRatio, count);
        XPorts[index].ExternTrigger = externTrigger;
        XPorts[index].Power = power;
    }

    void updateMinRemainTime()
    {
        long temp = __LONG_MAX__;
        for (const XPort &XPort : XPorts)
        {
            if (XPort.RemainMs < temp)
            {
                temp = XPort.RemainMs;
            }
        }
        minRemainTime = temp;
    }

    void check()
    {
        for (int i = 0; i < MAX_XPORT; i++)
        {
            XPort port = XPorts[i];
            if (port.count == 0)
            { // 카운트가 0이면 무시
                continue;
            }
            if (port.RemainMs - minRemainTime <= 0)
            {
                if (port.isOn)
                {
                    port.isOn = false;
                    port.RemainMs = port.T1;
                    //    digitalWrite(pins[i], LOW);
                }
                else
                {
                    port.isOn = true;
                    port.RemainMs = port.T2;
                    //  digitalWrite(pins[i], HIGH);

                    if (port.count > 0)
                    { //-1이면 무한반복, 0이상이면 카운트 감소
                        port.count--;
                        /* 0번 채널의 끝나는 시간을 millis로 출력
                        if(i==0 && port.count==0){
                          Serial.println("");
                          Serial.print(F("Time: ")); Serial.print(millis());

                        }
                        */
                    }
                }
            }
            else
            {
                port.RemainMs -= minRemainTime;
            }
        }
    }
};
