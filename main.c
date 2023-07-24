//Code by CacPixel
//using STC8G1K08(TSSOP20)

#include "stc8g.h"
#include "intrins.h"
#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

bit busy;
char xdata rxbuffer;
char xdata uartStartMsg[] = "/8051MeterUARTStart;\r\n";
char xdata receiveStr[64];
char xdata repeatMsg[32] = { 0 };
char xdata msg[32] = { 0 };
char* xdata receiveStrPtr = receiveStr;
bit shouldProcess = 0;
CommandTypeDef cmdtype = CMD_NULL;
float xdata speed = 0;
char xdata XCCAP2H;
int xdata pwmVal;
int xdata CMODVal;


void PCA_Routine(void) interrupt 7
{
    CF = 0;
}

void TM0_Isr() interrupt 1
{
    if (shouldProcess) {

        cmdtype = runUARTCommand();
        if (cmdtype == CMD_INVALID) {
            //command invalid
            clearMsg(msg, sizeof(msg));
            strcat(msg, "/cmd_invalid;\r\n");
            Uart2SendStr(msg);
        }
        shouldProcess = 0;
    }
}

void Uart2ReceiveStr() {
    if (receiveStrPtr == receiveStr + sizeof(receiveStr) - 1) {
        *receiveStrPtr++ = 0; //防止越界
    } else {
        *receiveStrPtr++ = rxbuffer;
    }
    return;
}

void Uart2Isr() interrupt 8
{
    //接收
    if (S2CON & 0x02) {
        S2CON &= ~0x02;
        busy = 0;
    }

    //发送
    if (S2CON & 0x01) {
        S2CON &= ~0x01;
        rxbuffer = S2BUF;
        if (receiveStr == receiveStrPtr && rxbuffer != '/') {
            return;
        }

        if (shouldProcess)
            return;

        if (rxbuffer == '/')
            cmdtype = CMD_NULL;

        Uart2ReceiveStr();
        if (receiveStrPtr == receiveStr + sizeof(receiveStr) || rxbuffer == ';') {

            if (receiveStrPtr != receiveStr + sizeof(receiveStr))
                *receiveStrPtr = 0;
            else {
                *(receiveStrPtr - 2) = ';';  //防止越界
            }
            receiveStrPtr = receiveStr;
            shouldProcess = 1;
        }
    }
}

void Uart2Init() {
    S2CON = 0x10;
    T2L = BRT;
    T2H = BRT >> 8;
    AUXR = 0x14;
    busy = 0;
}

void Uart2Send(char dat) {
    while (busy);
    busy = 1;
    S2BUF = dat;
}

void Uart2SendStr(char* p) reentrant {
    while (*p) {
        Uart2Send(*p++);
    }
}

void PCAInit() {
    CCON = 0x00;
    CMOD = 0x0e;
    CL = 0x00;
    CH = 0x00;

    P_SW1 |= 1 << 5;        //切换PCA2引脚为P3.7
    CCAPM2 = 0x42;          //10位PWM模式
    PCA_PWM2 = 0xc0;
    CCAP2L = 0xc8;
    CCAP2H = 0xc8;
    CR = 1;

}

void TIM0_Init() {
    AUXR &= 0x7F;
    TMOD &= 0xF0;
    TL0 = 0x66;
    TH0 = 0xFC;
    TF0 = 0;
    TR0 = 1;
    ET0 = 1;
}

void WritePwmValue(int val) {
    XCCAP2H = ((val & 0x0300) >> 8);
    PCA_PWM2 = ((XCCAP2H << 4) | PCA_PWM2) & ((XCCAP2H << 4) | 0xcf);       //先写高2位 XCCAP0H[1:0] ，保持PCA_PWM2其他位不变
    CCAP2H = (val & 0x00ff);                                                //再写低8位
}

void SetMeterWithSpeed() {
    pwmVal = ((PWM_MAX - PWM_MIN) / 120.0 * (120.0 - speed)) + PWM_MIN;
    WritePwmValue(pwmVal);
}

CommandTypeDef runUARTCommand() {
    char* xdata speedPos;
    char* xdata p;
    int xdata proc_str[sizeof(receiveStr)];

    /* 此部分从 stm32 中断回调函数移植而来 */
    if (strstr((char*)receiveStr, "/get") != NULL) {
        cmdtype = CMD_GET;
        getOutputMsg(msg);
        Uart2SendStr(msg);
    } else if (strstr((char*)receiveStr, "/set:") != NULL) {
        cmdtype = CMD_SET;
        speedPos = strstr((char*)receiveStr, ":") + 1;
        strcpy((char*)proc_str, speedPos);
        speed = (float)atof((char*)proc_str);
        SetMeterWithSpeed();
    } else if (strstr((char*)receiveStr, "/inc:") != NULL) {
        cmdtype = CMD_INC;
        speedPos = strstr((char*)receiveStr, ":") + 1;
        strcpy((char*)proc_str, speedPos);
        speed += (float)atof((char*)proc_str);
        SetMeterWithSpeed();
    } else if (strstr((char*)receiveStr, "/dec:") != NULL) {
        cmdtype = CMD_DEC;
        speedPos = strstr((char*)receiveStr, ":") + 1;
        strcpy((char*)proc_str, speedPos);
        speed -= (float)atof((char*)proc_str);
        SetMeterWithSpeed();
    } else if (strstr((char*)receiveStr, "/light_on;") != NULL) {
        cmdtype = CMD_LIGHTON;
        LIGHT_ON;
    } else if (strstr((char*)receiveStr, "/light_off;") != NULL) {
        cmdtype = CMD_LIGHTOFF;
        LIGHT_OFF;
    } else if (strstr((char*)receiveStr, "/led_on;") != NULL) {
        cmdtype = CMD_LEDON;
        LED_ON;
    } else if (strstr((char*)receiveStr, "/led_off;") != NULL) {
        cmdtype = CMD_LEDOFF;
        LED_OFF;
    } else if (strstr((char*)receiveStr, "/PWM:") != NULL) {
        cmdtype = CMD_SET_PWM;
        p = strstr((char*)receiveStr, ":") + 1;
        strcpy((char*)proc_str, p);
        pwmVal = atoi((char*)proc_str);
        WritePwmValue(pwmVal);
    } else if (strstr((char*)receiveStr, "/CMOD:") != NULL) {
        cmdtype = CMD_SET_CMOD;
        p = strstr((char*)receiveStr, ":") + 1;
        strcpy((char*)proc_str, p);
        CMOD = (char)atox((char*)proc_str);
    } else {
        cmdtype = CMD_INVALID;
    }
    return cmdtype;
}


void Delay1000ms()		//@11.0592MHz
{
    unsigned char i, j, k;

    i = 57;
    j = 27;
    k = 112;
    do {
        do {
            while (--k);
        } while (--j);
    } while (--i);
}


void getOutputMsg(char* msg) {
    CMODVal = CMOD;
    clearMsg(msg, sizeof(msg));
    sprintf(msg, "/get:speed=%.2f,pwmVal=%d,CMOD=%#X;\r\n", \
        speed, pwmVal, CMODVal);
}

/* char*转16进制 只支持“0xff”类似的写法，大小写均可 */
int atox(char* str) {
    int hex = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        if (str[3] >= 'A' && str[3] <= 'Z') {
            hex += str[3] - '0' - 8 + 1;
        } else if (str[3] >= 'a' && str[3] <= 'z') {
            hex += str[3] - '0' - 40 + 1;
        } else {
            hex += str[3] - '0';
        }

        if (str[2] >= 'A' && str[2] <= 'Z') {
            hex += (str[2] - '0' - 8 + 1) * 16;
        } else if (str[2] >= 'a' && str[2] <= 'z') {
            hex += (str[2] - '0' - 40 + 1) * 16;
        } else {
            hex += (str[2] - '0') * 16;
        }
    }
    return hex;
}




void main() {
    int i = 0;
    P0M0 = 0x00;
    P0M1 = 0x00;
    P1M0 = 0x1 << 2;
    P1M1 = 0x00;
    P2M0 = 0x00;
    P2M1 = 0x00;
    P3M0 = 0x00;
    P3M1 = 0x00;
    P4M0 = 0x00;
    P4M1 = 0x00;
    P5M0 = 0x00;
    P5M1 = 0x00;
    PCAInit();
    Uart2Init();
    TIM0_Init();

    IE2 = 0x01;
    IP = 0x80;
    IPH = 0x80;
    IP2 = 0x00;
    IP2H = 0x01;
    EA = 1;
    SetMeterWithSpeed();

    Uart2SendStr(uartStartMsg);

    while (1) {
        Delay1000ms();
        CMODVal = CMOD;
        sprintf(repeatMsg, "/status:speed=%.2f,pwmVal=%d,CMOD=%#X;\r\n", \
            speed, pwmVal, CMODVal);
        Uart2SendStr(repeatMsg);
    }
}

