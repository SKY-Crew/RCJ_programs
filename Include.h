#ifndef INCLUDE_ORIG
#define INCLUDE_ORIG

#include "AdvMath.h"
#include "Angle.h"
#include "Count.h"

#include "Motor.h"
uint8_t P_M_DIR[4] = {24, 25, 26, 27};
uint8_t P_M_PWR[4] = {5, 6, 9, 10};
Motor Motor(true, 4, P_M_DIR, P_M_PWR, 45, 0.8773, 32.73);
// CAN_MOVE, QTY, P_DIR, P_PWR, firstRM, SLOPE_POWER, INTERCEPT_POWER

#include "Kicker.h"
Kicker Kicker(29, 54, 30, 3, 40);
// 	P_KICKER, P_ONOFF_KICKER, P_RUN_KICKER, MAX_CK, MAX_CKW

#include "Ball.h"
uint8_t P_IR[16] = {36, 35, 53, 52, 51, 50, 49, 48, 47, 40, 41, 42, 43, 44, 45, 46};
uint16_t THRE_DIST[2] = {0, 400};
double SLOPE_DIR[5][2] = {{0.25, 0.667}, {1, 2.5}, {0, 0}, {-0.056, 0.056}, {0, 0}};
double INTERCEPT_DIR[5][2] = {{0, 0}, {-15, -27.5}, {30, 85}, {35, 80}, {25, 90}};
double POINT_DIR[4][2] = {{20, 15}, {45, 45}, {89, 89}, {179, 179}};

Ball Ball(16, P_IR, 2, 200, 0.1, THRE_DIST, 5, SLOPE_DIR, INTERCEPT_DIR, POINT_DIR, A20, 400, 10);
// QTY, PORT,
// MEASURING_COUNT, THRE_WEAK, CHANGE_RATE,
// SIZE_SLOPE_DIR, SLOPE_DIR, INTERCEPT_DIR, POINT_DIR,
// P_CATCH, THRE_CATCH, MAX_C_CATCH);

#include "Line.h"
uint8_t P_LINE[16] = {A9, A8, A7, A6, A5, A4, A3, A2, A1, A0, A22, A21, A10, A11, A26, A25};
Line Line(true, 16, P_LINE, 5, 100, 400, 12, 0.5);
// CAN_LEAVE_LINE, QTY, PORT, MAX_CIIA, THRE_BLACK, THRE_WHITE, THRE_IS_IN_AIR, CHANGE_RATE

#include "Cam.h"
Cam Cam(1, 56, 3, 3, 15, 10);
// X, P_ONOFF, CENTER_OPP_GOAL, CENTER_OWN_GOAL, SLOPE_RG, INTERCEPT_RG


// #define GYRO_ONOFF_PIN 55
// #define GYRO_RESET_PIN 2
// #define CAMERA_I2C_WIRE Wire1
// #define SIZE_SLOPE_RG 3
int16_t SLOPE_RG[3] = {0, 10, 30};
int16_t POINT_RG[2] = {5, 25};
#include "Gyro.h"
Gyro Gyro(1, 0x68, 55, 2, 3, SLOPE_RG, POINT_RG);

#include "PSD.h"
PSD frontPSD(1, 0.7, 1000, 6);
PSD backPSD(2, 0.7, 700, 3);
// X, CHANGE_RATE, THRE_IS_CLOSE, MAX_CC

#include "Comc.h"
Comc Comc(5, 57, 10, 20);
// X, P_ONOFF, MAX_C_SND, MAX_C_NR

#include "INA219.h"
INA219 INA219(2, 10.2, 10.4, 10, 10);
// X, LOW_VOLT, HIGH_VOLT, MAX_CR, MAX_CVL

#include "LCD.h"
LCD LCD(7, 13, 11, 8, 12, 0, 100, Line.getQTY());
// P_REDRAW, P_SCK, P_MOSI, P_CS, P_DI, WAIT, MAX_CP, QTY_LINE

const uint8_t P_START = 28;
const uint8_t P_CHANGE_ROLE = 32;
const uint8_t P_IS_FW = 31;

typedef struct {
	Angle gyro;
	cam_t goal;
	bool isGoalClose;
	bool isGoalClosePSD;

	bool enemyStandsFront;
	comc_t fellow;

	vectorRT_t ball;
	Dist distBall;
	bool isBallForward;
	bool catchingBall;
	bool catchFreely;

	line_t line;
} data_t;

#endif