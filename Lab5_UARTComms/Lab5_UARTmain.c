#include "msp.h"
#include <stdint.h>
#include <string.h>
#include "..\inc\UART0.h"
#include "..\inc\EUSCIA0.h"
#include "..\inc\FIFO0.h"
#include "..\inc\Clock.h"
//#include "..\inc\SysTick.h"
#include "..\inc\SysTickInts.h"
#include "..\inc\CortexM.h"
#include "..\inc\TimerA1.h"
//#include "..\inc\Bump.h"
#include "..\inc\BumpInt.h"
#include "..\inc\LaunchPad.h"
#include "..\inc\Motor.h"
#include "../inc/IRDistance.h"
#include "../inc/ADC14.h"
#include "../inc/LPF.h"
#include "..\inc\Reflectance.h"
#include "../inc/TA3InputCapture.h"
#include "../inc/Tachometer.h"

#define P2_4 (*((volatile uint8_t *)(0x42098070)))
#define P2_3 (*((volatile uint8_t *)(0x4209806C)))
#define P2_2 (*((volatile uint8_t *)(0x42098068)))
#define P2_1 (*((volatile uint8_t *)(0x42098064)))
#define P2_0 (*((volatile uint8_t *)(0x42098060)))


// At 115200, the bandwidth = 11,520 characters/sec
// 86.8 us/character
// normally one would expect it to take 31*86.8us = 2.6ms to output 31 characters
// Random number generator
// from Numerical Recipes
// by Press et al.
// number from 0 to 31
uint32_t Random(void){
static uint32_t M=1;
  M = 1664525*M+1013904223;
  return(M>>27);
}
char WriteData,ReadData;
uint32_t NumSuccess,NumErrors;
void TestFifo(void){char data;
  while(TxFifo0_Get(&data)==FIFOSUCCESS){
    if(ReadData==data){
      ReadData = (ReadData+1)&0x7F; // 0 to 127 in sequence
      NumSuccess++;
    }else{
      ReadData = data; // restart
      NumErrors++;
    }
  }
}
uint32_t Size;
int Program5_1(void){
//int main(void){
    // test of TxFifo0, NumErrors should be zero
  uint32_t i;
  Clock_Init48MHz();
  WriteData = ReadData = 0;
  NumSuccess = NumErrors = 0;
  TxFifo0_Init();
  TimerA1_Init(&TestFifo,43);  // 83us, = 12kHz
  EnableInterrupts();
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      TxFifo0_Put(WriteData);
      WriteData = (WriteData+1)&0x7F; // 0 to 127 in sequence
    }
    Clock_Delay1ms(10);
  }
}

char String[64];
uint32_t MaxTime,First,Elapsed;
int Program5_2(void){
//int main(void){
    // measurement of busy-wait version of OutString
  uint32_t i;
  DisableInterrupts();
  Clock_Init48MHz();
  UART0_Init();
  WriteData = 'a';
  SysTick_Init(0x1000000,2); //OHL - using systick INT api
  MaxTime = 0;
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      String[i] = WriteData;
      WriteData++;
      if(WriteData == 'z') WriteData = 'a';
    }
    String[i] = 0; // null termination
    First = SysTick->VAL;
    UART0_OutString(String);
    Elapsed = ((First - SysTick->VAL)&0xFFFFFF)/48; // usec

    if(Elapsed > MaxTime){
        MaxTime = Elapsed;
    }
    UART0_OutChar(CR);UART0_OutChar(LF);
    Clock_Delay1ms(100);
  }
}

int Program5_3(void){
//int main(void){
    // measurement of interrupt-driven version of OutString
  uint32_t i;
  DisableInterrupts();
  Clock_Init48MHz();
  EUSCIA0_Init();
  WriteData = 'a';
  SysTick_Init(0x1000000,2); //OHL - using systick INT api
  MaxTime = 0;
  EnableInterrupts();
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      String[i] = WriteData;
      WriteData++;
      if(WriteData == 'z') WriteData = 'a';
    }
    String[i] = 0; // null termination
    First = SysTick->VAL;
    EUSCIA0_OutString(String);
    Elapsed = ((First - SysTick->VAL)&0xFFFFFF)/48; // usec
    if(Elapsed > MaxTime){
        MaxTime = Elapsed;
    }
    EUSCIA0_OutChar(CR);EUSCIA0_OutChar(LF);
    Clock_Delay1ms(100);
  }
}
int Program5_4(void){
//int main(void){
    // demonstrates features of the EUSCIA0 driver
  char ch;
  char string[20];
  uint32_t n;
  DisableInterrupts();
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  EUSCIA0_Init();     // initialize UART
  EnableInterrupts();
  EUSCIA0_OutString("\nLab 5 Test program for EUSCIA0 driver\n\rEUSCIA0_OutChar examples\n");
  for(ch='A'; ch<='Z'; ch=ch+1){// print the uppercase alphabet
     EUSCIA0_OutChar(ch);
  }
  EUSCIA0_OutChar(LF);
  for(ch='a'; ch<='z'; ch=ch+1){// print the lowercase alphabet
    EUSCIA0_OutChar(ch);
  }
  while(1){
    EUSCIA0_OutString("\n\rInString: ");
    EUSCIA0_InString(string,19); // user enters a string
    EUSCIA0_OutString(" OutString="); EUSCIA0_OutString(string); EUSCIA0_OutChar(LF);

    EUSCIA0_OutString("InUDec: ");   n=EUSCIA0_InUDec();
    EUSCIA0_OutString(" OutUDec=");  EUSCIA0_OutUDec(n); EUSCIA0_OutChar(LF);
    EUSCIA0_OutString(" OutUFix1="); EUSCIA0_OutUFix1(n); EUSCIA0_OutChar(LF);
    EUSCIA0_OutString(" OutUFix2="); EUSCIA0_OutUFix2(n); EUSCIA0_OutChar(LF);

    EUSCIA0_OutString("InUHex: ");   n=EUSCIA0_InUHex();
    EUSCIA0_OutString(" OutUHex=");  EUSCIA0_OutUHex(n); EUSCIA0_OutChar(LF);
  }
}


//IR Sensor
//from Lab4_ADCmain
volatile uint32_t nr,nc,nl;
volatile uint32_t ADCflag = 0;
void SensorRead_ISR(void){  
    uint32_t raw17,raw12,raw16;
    P1OUT ^= 0x01;         
    P1OUT ^= 0x01;         
    ADC_In17_12_16(&raw17,&raw12,&raw16); 
    nr = LPF_Calc(raw17);  
    nc = LPF_Calc2(raw12);
    nl = LPF_Calc3(raw16); 
    ADCflag = 1;     
    P1OUT ^= 0x01;         
}


void IRSensor_Init(){
    uint32_t raw17,raw12,raw16;
    uint32_t s;
    ADCflag = 0;
    s = 256;
    ADC0_InitSWTriggerCh17_12_16(); 
    ADC_In17_12_16(&raw17,&raw12,&raw16);
    LPF_Init(raw17,s);    
    LPF_Init2(raw12,s);   
    LPF_Init3(raw16,s);    

}


//bump switch

volatile uint8_t bumpData, bumpFlag; 
void HandleBump(uint8_t bumpSensor){
   bumpData = bumpSensor;
   bumpFlag = 1;
   P4->IFG &= ~0xED;                  
}



// tachometer
// from Lab4_Tachmain
uint16_t Period0;              
uint16_t First0;             
int Done0;                     
void PeriodMeasure0(uint16_t time){
  P2_0 = P2_0^0x01;          
  Period0 = (time - First0)&0xFFFF;
  First0 = time;                  
  Done0 = 1;
}
uint16_t Period2;             
uint16_t First2;               
int Done2;      
void PeriodMeasure2(uint16_t time){
  P2_2 = P2_2^0x01;           
  Period2 = (time - First2)&0xFFFF; 
  First2 = time;                   
  Done2 = 1;
}

void TimedPause(uint32_t time){
  Clock_Delay1ms(time);         
  Motor_Stop();
  while(LaunchPad_Input()==0);  
  while(LaunchPad_Input());    
}

#define PERIOD 1000  

void toggle_GPIO(void){
    P2_4 ^= 0x01;     
}

void Tachometer_Init(){
    P2->SEL0 &= ~0x11;
    P2->SEL1 &= ~0x11; 
    P2->DIR |= 0x11;    
    First0 = First2 = 0;
    Done0 = Done2 = 0;  
}

//---------------------------------------------------------------

void RSLK_Reset(void){
    DisableInterrupts();
    Clock_Init48MHz();  
    Motor_Init();
    Motor_Stop();
    LaunchPad_Init();
    BumpInt_Init(&HandleBump);
    Reflectance_Init();
    IRSensor_Init();
    Tachometer_Init();
    EUSCIA0_Init();    
    EnableInterrupts();
}



// RSLK Self-Test
int main(void) {
    uint32_t cmd=0xDEAD, menu=0;
    uint32_t choice = 0xDEAD, repeat = 1;
    uint8_t reflectanceData;
    uint32_t delay = 3000; //For Motor Test, set as 3 seconds for each movement
    uint32_t pwm = 3000;
    uint32_t i = 0, n, count = 0;
    volatile uint8_t bumpTemp;

    DisableInterrupts();
    Clock_Init48MHz();  // makes SMCLK=12 MHz
    //SysTick_Init(48000,2);  // set up SysTick for 1000 Hz interrupts
    Motor_Init();
    Motor_Stop();
    LaunchPad_Init();
    //Bump_Init();
    //Bumper_Init();
    BumpInt_Init(&HandleBump);
    Reflectance_Init();
    IRSensor_Init();
    Tachometer_Init();
    EUSCIA0_Init();     // initialize UART
    EnableInterrupts();

  while(1){                     // Loop forever
      // write this as part of Lab 5
      EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("RSLK Testing"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[0] RSLK Reset"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[1] Motor"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[2] IR Sensor"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[3] Bumper"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[4] Reflectance Sensor"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[5] Tachometer"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[6] Obstacle Avoidance by IR Sensor"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[7] Obstacle Avoidance by Bump Switch"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      EUSCIA0_OutString("[8] Go straight black line by reflectence sensor"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);

      EUSCIA0_OutString("Enter your command: ");
      EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
      cmd=EUSCIA0_InUDec();
      

      switch(cmd){
          case 0: //RSLK Reset
              RSLK_Reset();
              EUSCIA0_OutString("Reset Completed!!"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              menu =1;
              cmd=0xDEAD;
              break;

          case 1: //Motor Test
              repeat = 1;
              delay = 3000;
              pwm = 3000;
              choice = 0xDEAD;
              while(repeat){
                  EUSCIA0_OutString("What motion would you like to test?"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[0] Moving Forward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[1] Moving Backward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[2] Moving Right Wheel Forward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[3] Moving Right Wheel Backward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[4] Moving Left Wheel Forward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[5] Moving Left Wheel Backward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[6] Moving Rotate Left"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("[7] Moving Rotate Right"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("Press any other number to exit!"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  EUSCIA0_OutString("Enter your choice: ");
                  choice=EUSCIA0_InUDec();
                  EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  switch(choice){
                  case 0 : //Move Forward
                      Motor_Forward(pwm, pwm);
                      Clock_Delay1ms(delay);
                      Motor_Stop();
                      break;
                  case 1: //Move Backward
                      Motor_Backward(pwm,pwm);
                      Clock_Delay1ms(delay);
                      Motor_Stop();
                      break;
                  case 2: //Right Wheel Forward
                      Motor_Left(pwm, 0);
                      Clock_Delay1ms(delay);
                      Motor_Stop();
                      break;
                  case 3: //Right Wheel Backwards
                      Motor_Right(pwm,0);
                      Clock_Delay1ms(delay); 
                      Motor_Stop();
                      break;
                  case 4: //Left Wheel Forward
                      Motor_Right(0, pwm);
                      Clock_Delay1ms(delay); 
                      Motor_Stop();
                      break;
                  case 5: //Left Wheel Backwards
                      Motor_Left(0, pwm);
                      Clock_Delay1ms(delay); 
                      Motor_Stop();
                      break;
                  case 6://Turn Left
                      Motor_Left(pwm, pwm);
                      Clock_Delay1ms(delay); 
                      Motor_Stop();
                      break;
                  case 7:
                      Motor_Right(pwm, pwm);
                      Clock_Delay1ms(delay); 
                      Motor_Stop();
                  break;
                  default:
                      repeat = 0;

                  }
                  EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              }
              menu =1;
              cmd=0xDEAD;
              break;

          case 2://IR Sensor
              EUSCIA0_OutString("IR Sensor Testing, display 30 readings"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              TimerA1_Init(&SensorRead_ISR,250);
              i = 0;
              while(i < 30){
                  for(n=0; n<2000; n++){
                      while(ADCflag == 0){};
                      ADCflag = 0; 
                  }
                  UART0_OutString("Left sensor: ");UART0_OutUDec5(LeftConvert(nl));UART0_OutString(" cm,");
                  UART0_OutString("Center sensor: ");UART0_OutUDec5(CenterConvert(nc));UART0_OutString(" cm,");
                  UART0_OutString("Right sensor: ");UART0_OutUDec5(RightConvert(nr));UART0_OutString(" cm");
                  i++;
              }
              menu =1;
              cmd=0xDEAD;
              break;

          case 3://Bumper
              bumpTemp = bumpData = 0x3F;
              bumpFlag = 0;
              //choice = 0xDEAD;
              count = 0;
              i = 0;

              EUSCIA0_OutString("Bumper Testing"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("Pls press the bump you want to test: "); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              while(1){
                  WaitForInterrupt();
                  if(bumpFlag == 1) {
                      bumpTemp = bumpData;
                      break; 
                  }
              }
              for (i = 0; i<6; i++){
                  if(bumpTemp % 2 == 0){
                      EUSCIA0_OutString("Bump Number ");
                      EUSCIA0_OutUDec(i);
                      EUSCIA0_OutString(" was pressed");EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  }
                  bumpTemp = bumpTemp>>1;
              }

              bumpData = 0x3F;
              bumpFlag = 0;
              menu=1;
              cmd=0xDEAD;
              break;

          case 4: //Reflectance sensor
              count = 0;
              reflectanceData = 0;
              EUSCIA0_OutString("Reflectance Sensor Testing"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("Show Results: (10 sec)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              while(count < 10){
                  reflectanceData = Reflectance_Read(1000);
                  for(i = 0; i < 8; i++){
                      EUSCIA0_OutUDec(reflectanceData%2);
                      reflectanceData = reflectanceData >> 1;
                  }
                  EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  Clock_Delay1ms(1000);
                  count ++;
                }
              menu =1;
              cmd=0xDEAD;
              break;

          case 5: //Tachometer
              bumpFlag = 0; //Exit program using bum switch as interrupt
              EUSCIA0_OutString("Tachometer testing:"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("Display period in 1 sec for 30 times"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("Press side button to start "); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("Press bump to exit"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              TimerA1_Init(&toggle_GPIO,10);    // 50Khz sampling
              TimerA3Capture_Init(&PeriodMeasure0,&PeriodMeasure2);
              TimedPause(500);
              Motor_Forward(3000,3000); // 50%
              EnableInterrupts();
              while(1){
                  WaitForInterrupt();
                  UART0_OutString("Left motor(from front) Period0 = ");UART0_OutUDec5(Period0);UART0_OutString(" right motor(from front) Period2 = ");UART0_OutUDec5(Period2);UART0_OutString(" \r\n");
                  if(bumpFlag == 1) break;
                  Clock_Delay1ms(1000);
              }
              Motor_Stop();
              menu =1;
              cmd=0xDEAD;
              break;

          case 6:
              // move automatically
              // Any obstacle less than 10cm from IR sensor -- action
              bumpFlag = 0;
              pwm = 1500;
              EUSCIA0_OutString("Evading using IRSensors"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("You can stop movement by pressing buttons"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              TimerA1_Init(&SensorRead_ISR,250);
              EnableInterrupts();
              while(1){
                  EUSCIA0_OutString("Moving forward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  Motor_Forward(pwm, pwm);
                  if(LeftConvert(nl) < 10){ //Something near left of robot
                      EUSCIA0_OutString("turn right"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                      Motor_Stop();
                      Motor_Right(pwm, pwm);
                      Clock_Delay1ms(500); //Turn for 0.5 seconds
                      Motor_Stop();
                      Motor_Forward(pwm,pwm);
                  }
                  else if(CenterConvert(nc) < 10){
                      EUSCIA0_OutString("Moving back and right"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                      Motor_Stop();
                      Motor_Backward(pwm, pwm);
                      Clock_Delay1ms(500); //Reverse for 0.5 seconds
                      Motor_Stop();
                      Motor_Right(pwm, pwm);
                      Clock_Delay1ms(500); //Turn for 0.5 seconds
                      Motor_Stop();

                      Motor_Forward(pwm,pwm);
                  }
                  else if(RightConvert(nr) < 10){
                      EUSCIA0_OutString("turn left"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                      Motor_Stop();
                      Motor_Left(pwm, pwm);
                      Clock_Delay1ms(500); //Turn for 0.5 seconds
                      Motor_Stop();
                      Motor_Forward(pwm,pwm);
                  }
                  UART0_OutUDec5(LeftConvert(nl));UART0_OutString(" cm,");
                  UART0_OutUDec5(CenterConvert(nc));UART0_OutString(" cm,");
                  UART0_OutUDec5(RightConvert(nr));UART0_OutString(" cm");
                  if(bumpFlag == 1) break;
              }
              Motor_Stop();
              menu =1;
              cmd=0xDEAD;
              break;

          case 7:
              // move straight automatically
              // whenever a bumper pressed -- action
              menu =1;
              cmd=0xDEAD;
              bumpFlag = 0;
              bumpTemp = bumpData = 0x3F;
              pwm = 1500;
              EUSCIA0_OutString("Obstacle Avoidance using Bump Switches"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EUSCIA0_OutString("Stop movement using reset button"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
              EnableInterrupts();
              while(1){
                  Motor_Forward(pwm,pwm);
                  if(bumpFlag == 1){
                      bumpTemp = bumpData;
                      bumpTemp = bumpTemp>>3;
                      if((bumpTemp) != 0x7){ // Right 3 
                          Motor_Stop();
                          Motor_Backward(pwm,pwm);
                          Clock_Delay1ms(500);
                          Motor_Stop();
                          Motor_Right(pwm,pwm);
                          Clock_Delay1ms(500);
                          Motor_Stop();
                          Motor_Forward(pwm,pwm);
                      }
                      else{ // Left 3 
                          Motor_Stop();
                          Motor_Backward(pwm,pwm);
                          Clock_Delay1ms(500);
                          Motor_Stop();
                          Motor_Left(pwm,pwm);
                          Clock_Delay1ms(500);
                          Motor_Stop();
                          Motor_Forward(pwm,pwm);
                      }
                      bumpFlag = 0;
                  }
                  if(LaunchPad_Input() != 0) break;
              }
              Motor_Stop();

              break;
          case 8:
              // move straight in black line
              count = 0;
            reflectanceData = 0;
            uint8_t temp = 0;
            EUSCIA0_OutString("Reflectance Sensor Moving Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
            EUSCIA0_OutString("Displaying 10 results with 1 sec interval"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
            bumpFlag = 0;
            pwm = 1000;
            EnableInterrupts();
              while (count < 10){
                  reflectanceData = Reflectance_Read(1000);
                  temp = reflectanceData;
                for(i = 0; i < 8; i++){
                    EUSCIA0_OutUDec(temp%2);
                    temp = temp >> 1;
                }
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                  if (reflectanceData >= 48){
                    Motor_Stop();
                    Motor_Right(pwm, pwm);
                    Clock_Delay1ms(500); //Turn for 0.5 seconds
                    Motor_Stop();
                    Motor_Forward(pwm,pwm);
                  }
                  else if (reflectanceData <= 16){
                      Motor_Stop();
                    Motor_Left(pwm, pwm);
                    Clock_Delay1ms(500); //Turn for 0.5 seconds
                    Motor_Stop();
                    Motor_Forward(pwm,pwm);
                  }
                  else if (reflectanceData == 0){
                      Motor_Stop();
                  }
                  else{
                      Motor_Stop();
                      Motor_Forward(pwm,pwm);
                      Clock_Delay1ms(500);
                  }

                  count++;
                  if(bumpFlag == 1) break;
              }
              Motor_Stop();

              menu =1;
              cmd=0xDEAD;
              break;



          default:
              menu=1;
              break;
      }

      if(!menu)Clock_Delay1ms(3000);
      else{
          menu=0;
      }

  }
}
