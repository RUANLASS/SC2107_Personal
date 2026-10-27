// Tasks.h - Lab5 practice tasks (see Tasks.c for the skeleton notes on each)
#ifndef TASKS_H_
#define TASKS_H_

// Pre-known demo + calibration helpers (run at the desk, USB connected)
void Task_PreKnown(void);     // Lab2/3/4 values live in the watch window
void Task_Calibrate(void);    // streams IR / reflectance / bump / tach over UART
void Test_Square(void);       // 4 x (300 mm + 90 deg right): tune TURN/DIST_CAL_PCT
void Test_Turn360(void);      // one 360 deg turn: tune TURN_CAL_PCT
void Test_Straight1m(void);   // 1000 mm straight: tune DIST_CAL_PCT
void Test_LineFollow(void);   // plain line following: tune LF_KP_NUM / speed

// M-tasks
void Task_M1_LineTurnLeft(void);
void Task_M2_BumpReverseTurn(void);
void Task_M3_IRStopReverseTurn(void);
void Task_M4_SW2LineFollow5s(void);
void Task_M5_BumpBinaryLED(void);
void Task_M6_ReflectanceLED(void);
void Task_M7_TachDistance(void);
void Task_M8_Corridor(void);
void Task_M9_CountBumps10s(void);
void Task_M10_LineThenDrive2s(void);

// H-tasks
void Task_H1_BumpBinDecHex(void);
void Task_H2_LineRecoveryPark(void);
void Task_H3_AvoidAndCount(void);
void Task_H4_Turn180Drive40(void);
void Task_H5_FSM(void);
void Task_H6_BumpSpeedDir(void);
void Task_H7_DistanceSequence(void);
void Task_H8_LineBumpOverride(void);
void Task_H9_Parking(void);
void Task_H10_NumberEntryDrive(void);

void Task_WallFollow(void);
void Task403_RotateScan(void);
//void Task404_StateMach(){}
//void Task405_BumpMode(void);
void Task406_Boundary(void);
//void Task407_Password(){}
//void Task408_LED_UDC(){}
static uint8_t EnterNumber(const char *label);
void Task409_SumDiff(void);
//void Task410_NumDrive(){}

#endif
