#include "AdvMath.h"
#include "Angle.h"
#include "Count.h"

#include "Actuator.h"
uint8_t P_M_DIR[4] = {24, 25, 26, 27};
uint8_t P_M_PWR[4] = {5, 6, 9, 10};
Actuator Actuator(true, 4, P_M_DIR, P_M_PWR, 45, 0.8773, 32.73, 29, 54, 30, 3, 40);
// CAN_MOVE, QTY, P_DIR,  P_PWR, firstRM, SLOPE_POWER, INTERCEPT_POWER,
// 	P_KICKER, P_ONOFF_KICKER, P_RUN_KICKER, MAX_CK, MAX_CKW

#include "Ball.h"
uint8_t P_IR[16] = {36, 35, 53, 52, 51, 50, 49, 48, 47, 40, 41, 42, 43, 44, 45, 46};
#if IS_SKY
	uint16_t MAX_IR[] = {623, 568, 619, 593, 580, 619, 604, 593,
		593, 611, 591, 586, 605, 580, 595, 593};
	uint16_t AVG_IR[] = {381, 332, 362, 344, 337, 360, 331, 333,
		344, 359, 337, 341, 379, 353, 360, 363};
#else
	uint16_t MAX_IR[] = {622, 566, 588, 566, 625, 618, 651, 566,
		562, 618, 577, 566, 607, 622, 622, 618};
	uint16_t AVG_IR[] = {355, 270, 280, 290, 344, 327, 337, 284,
		330, 328, 276, 299, 329, 358, 354, 346};
#endif
double SLOPE_DIR[5][2] = {{1, 1}, {0.2, 2}, {0.111, 0}, {-0.167, 0.222}, {0, 0}};
double INTERCEPT_DIR[5][2] = {{0, 0}, {8, -20}, {13.333, 70}, {55, 50}, {25, 90}};
double POINT_DIR[4][2] = {{10, 20}, {60, 45}, {150, 90}, {180, 180}};

Ball Ball(16, P_IR, MAX_IR, AVG_IR, 2, 200, 0.1, 5, SLOPE_DIR, INTERCEPT_DIR, POINT_DIR, A20, 400, 10);
// QTY, PORT, MAX_IR, AVG_IR,
// MEASURING_COUNT, BORDER_WEAK, MULTI_AVG,
// SIZE_SLOPE_DIR, SLOPE_DIR, INTERCEPT_DIR, POINT_DIR,
// P_CATCH, BORDER_CATCH, MAX_C_CATCH);

#include "Line.h"
uint8_t P_LINE[16] = {A9, A8, A7, A6, A5, A4, A3, A2, A1, A0, A22, A21, A10, A11, A26, A25};
Line Line(true, 16, P_LINE, 5, 100, 400, 12, 0.5);
// CAN_LEAVE_LINE, QTY, PORT, MAX_CIIA, BORDER_BLACK, BORDER_WHITE, BORDER_IS_IN_AIR, MULTI_AVG

#include "Cam.h"
Cam Cam(1, 56, 3, 3, 15, 10);
// X, P_ONOFF, CENTER_OPP_GOAL, CENTER_OWN_GOAL, SLOPE_RG, INTERCEPT_RG


#define GYRO_ONOFF_PIN 55
#define GYRO_RESET_PIN 2
#define CAMERA_I2C_WIRE Wire1
#define SIZE_SLOPE_RG 3
int16_t SLOPE_RG[SIZE_SLOPE_RG] = {0, 10, 30};
int16_t POINT_RG[SIZE_SLOPE_RG - 1] = {5, 25};
#include "Gyro.h"

#include "PSD.h"
PSD frontPSD(1, 0.7, 1000, 6);
PSD backPSD(2, 0.7, 700, 3);
// X, MULTI_AVG, BORDER_IS_CLOSE, MAX_CC

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