#ifndef _MAIN_H_
#define _MAIN_H_


#define FOSC        11059200UL
#define BRT         (65536 - FOSC / 115200 / 4)

#define LIGHT_OFF P36=1
#define LIGHT_ON P36=0
#define LED_OFF P12=0
#define LED_ON P12=1
#define clearMsg(msg, size) memset(msg, 0, size)

#define PWM_MAX 837.0  // 0 km/h
#define PWM_MIN 123.0  //120 km/h


typedef enum {
    CMD_INVALID,
    CMD_NULL,
    CMD_GET,
    CMD_SET,
    CMD_INC,
    CMD_DEC,
    CMD_LIGHTON,
    CMD_LIGHTOFF,
    CMD_LEDON,
    CMD_LEDOFF,
    CMD_SET_PWM,
    CMD_SET_CMOD
} CommandTypeDef;

void Uart2ReceiveStr();
void Uart2Init();
void Uart2Send(char dat);
void Uart2SendStr(char* p) reentrant;
void PCAInit();
void TIM0_Init();
void WritePwmValue(int val);
CommandTypeDef runUARTCommand();
void Delay1000ms();
void getOutputMsg(char* msg);
int atox(char* str);


#endif