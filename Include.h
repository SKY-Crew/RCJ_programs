#ifndef INCLUDE_ORIG
#define INCLUDE_ORIG

#include "AdvMath.h"
#include "Angle.h"
#include "Count.h"
#include "Dist.h"
#include "Debug.h"

const bool IS_SKY = true;
bool isFW = IS_SKY;

const bool CAN_MOVE = true;

double DIR[2][5] = {{0, 15, 40, 110, 180}, {0, 10, 25, 60, 120}};
double PLUS_DIR[2][5] = {{0, 0, 40, 55, 30}, {0, 0, 70, 90, 75}};

#include "Motor.h"
uint8_t P_M_DIR[4] = {24, 25, 26, 27};
uint8_t P_M_PWR[4] = {5, 6, 9, 10};
double MULTI_POWER_SKY[4] = {1.1, 1.2, 1.1, 1.19};
double MULTI_POWER_CREW[4] = {1.07, 1.05, 1.1, 1.15};
Motor Motor(CAN_MOVE, 4, P_M_DIR, P_M_PWR, 45, 0.8773, 32.73, IS_SKY ? MULTI_POWER_SKY : MULTI_POWER_CREW);
// CAN_MOVE, QTY, P_DIR, P_PWR, firstRM, SLOPE_POWER, INTERCEPT_POWER, MULTI_POWER

#include "Kicker.h"
Kicker Kicker(29, 7, 5, 40, 100);
// P_KICKER, P_RUN_KICKER, MAX_CK, MAX_CKW, MAX_CS

#include "Ball.h"
uint8_t P_IR[16] = {36, 35, 53, 52, 51, 50, 49, 48, 47, 40, 41, 42, 43, 44, 45, 46};
double THRE_DIST_BALL[2] = {300, 180};
double* p_DIR[2] = {DIR[0], DIR[1]};
double* p_PLUS_DIR[2] = {PLUS_DIR[0], PLUS_DIR[1]};
Ball Ball(16, P_IR, 2, 200, 1.0, 0.1, IS_SKY ? 0 : 0, 2, THRE_DIST_BALL, 5, p_DIR, p_PLUS_DIR, A20, 200, 10);
// QTY, PORT,
// MEASURING_COUNT, THRE_WEAK, CHANGE_RATE, CHANGE_RATE_T, PLUS_T
// SIZE_THRE_DIST, THRE_DIST, SIZE_DIR, DIR, PLUS_DIR,
// P_CATCH, THRE_CATCH, MAX_C_CATCH,

#include "Line.h"
uint8_t P_LINE[16] = {A9, A8, A7, A6, A5, A4, A3, A2, A1, A0, A22, A21, A10, A11, A26, A25};
Line Line(true, 16, P_LINE, 5, 120, 300, 12, 0.9);
// CAN_LEAVE_LINE, QTY, PORT, MAX_CIIA, THRE_BLACK, THRE_WHITE, THRE_IS_IN_AIR, CHANGE_RATE

#include "Cam.h"
double Kd = 0.4;
double POINT_CAM[3] = {0, 7, 40};
double ROT_CAM[3] = {0, 10, 100};
Cam Cam(1, 55, 3, POINT_CAM, ROT_CAM, 0.4);
// P_SERIAL, P_ONOFF, SIZE_POINT, POINT, ROT, Kd

double POINT_GYRO[3] = {0, 9, 40};
double ROT_GYRO[3] = {0, 17.2, 100};
#include "Gyro.h"
Gyro Gyro(2, 0x68, 56, 2, 3, POINT_GYRO, ROT_GYRO, 0.3, 30, 30, 60);
// P_WIRE, PORT, ONOFF_PIN, RESET_PIN, SIZE_POINT, POINT, ROT, Kd,
// BROKEN_THRE, STOP_FRAMES, STAY_THRE

#include "PSD.h"
PSD frontPSD(31, 0.3, 450, 6);
PSD backPSD[2] = {PSD(37, 0.9, 400, 6), PSD(38, 0.9, 400, 6)};
// P_WIRE, CHANGE_RATE, THRE_IS_CLOSE, MAX_CC

#include "Comc.h"
Comc Comc(5, 57, 3, 50);
// P_SERIAL, P_ONOFF, MAX_C_SND, MAX_C_NR

#include "INA219.h"
INA219 INA219(2, 10.4, 10.6, 12.4, 10, 10);
// P_WIRE, LOW_VOLT, HIGH_VOLT, MAX_VOLT, MAX_CR, MAX_CVL

#include "LCD.h"
LCD LCD(7, 13, 11, 8, 12, 0, 30, -90, Line.getQTY());
// P_REDRAW, P_SCK, P_MOSI, P_CS, P_DI, WAIT, MAX_CP, PLUS_T, QTY_LINE

#include "Buzzer.h"
Buzzer Buzzer(30);

const uint8_t P_START = 28;
const uint8_t P_CHANGE_ROLE = 32;
const uint8_t P_IS_FW = 54;

typedef struct {
	Angle gyro;
	cam_t goal;

	bool enemyStands[2]; // Front, Back
	comc_t fellow;

	uint16_t valBackPSD;
	Dist distGoalPSD;
	Dist distGoal;

	vectorRT_t ball;
	Dist distBall;
	bool isBallForward;
	bool catchingBall;
	bool catchFreely;

	line_t line;
} data_t;

#endif