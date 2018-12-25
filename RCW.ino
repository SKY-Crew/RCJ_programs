const bool IS_SKY = false;

const uint8_t P_START = 28;
bool canRun;
bool prvCanRun;

const uint8_t P_CHANGE_ROLE = 32;
const uint8_t P_IS_FW = 31;
bool isFW = IS_SKY;
bool prvIsFW;
uint8_t countBecomeFW = 0;
const uint16_t MAX_CBF = 100;

uint16_t BORDER_IF;
uint16_t BORDER_IC;

bool correctingRot = false;
uint16_t countCorrectRot = 0;
uint16_t BORDER_INCREASE_CCR;
uint16_t BORDER_DECREASE_CCR;
const uint16_t MAX_CCR = 3;

uint16_t countESF = 0;
const uint16_t MAX_CESF = 8;

bool carryingBall = false;
bool willCarryBall = false;
uint32_t timeStartCB;

uint16_t countCatchFreely = 0;
uint16_t MAX_CCF;

#include "includes.h"

void setup() {
	delay(1000);
	Serial.begin(9600);
	gyroSetup();

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);
}

void loop() {
	Actuator.setHaveRun(false);
	prvCanRun = canRun;
	canRun = digitalRead(P_START);

	if((isFW && isFW != prvIsFW) || (canRun && !prvCanRun && isFW)) {
		countBecomeFW = MAX_CBF;
	}
	countBecomeFW = max(countBecomeFW - 1, 0);
	prvIsFW = isFW;
	digitalWrite(P_IS_FW, isFW);
	if(Ina219.checkVolt() && !Actuator.getIsKicking()) {
		//電池残量少
		canRun = false;
		stop();
	}else {
		//get
		vectorRT_t ball = Ball.get(true);
		bool isBallClose = ball.r >= BORDER_IC;
		bool isBallForward = Ball.getForward() >= BORDER_IF && isBallClose;
		bool catchingBall = Ball.getCatching() && insideAngle(ball.t, 330, 30) && isBallClose;
		int16_t gyro = simplifyDeg(getGyro() + 180) -180;
		line_t line = Line.get(isFW, getCanUseGyro(), gyro);
		cam_t goal = Cam.get();
		bool isGoalClose = false;
		int16_t rot = 0;
		comc_t fellow = Comc.communicate(canRun, isFW);
		bool isGoalCloseLazer = backPSD.get();
		countESF = frontPSD.get() ? MAX_CESF : max(countESF - 1, 0);
		bool enemyStandsFront = countESF > 0;
		checkRole(countBecomeFW <= 0, fellow);
		countCatchFreely = (catchingBall && !enemyStandsFront) ? min(countCatchFreely + 1, MAX_CCF) : 0;
		bool catchFreely = countCatchFreely >= MAX_CCF && (isFW || goal.distGK >= 2 || !Cam.getCanUse());

		if(canRun) {
			//走行中
			if(!prvCanRun) {
				//スタート直後
				line.isOutside = false;
				LCD.clear(true);
				LCD.write("Running!", 0, 0);
			}
			if(line.isOutside) {
				//ライン復帰
				Actuator.run(line.dirInside, rot, 150);
			}else{
				double dir = Ball.getDir(ball.t, isBallClose);
				if(!isFW) {
					isFW = (catchFreely || line.isInAir) && fellow.exists;
				}
				if(isFW) {
					if(fellow.exists) {
						if(insideAngle(ball.t, 90, 270)) {
							switch (goal.distFW) {
							case 1: dir = insideAngle(ball.t, 170, 190) ? -1
										: insideAngle(ball.t, 90, 180) ? 90 : 270;
									isGoalClose = true;
									break;
							case 0: dir = insideAngle(ball.t, 170, 190) ? 0
										: insideAngle(ball.t, 90, 180) ? 50 : 310;
									isGoalClose = false;
									break;
							}
						}
					}
					if(line.dirInside != -1) {
						if(insideAngle(line.dirInside, 45, 135)) {
							if(insideAngle(dir, line.dirInside + 170, line.dirInside + 200)
							&& line.canPause) {
								dir = -1;
							}else if(insideAngle(dir, line.dirInside + 90, line.dirInside + 180)) {
								dir = simplifyDeg(line.dirInside + 90);
							}
						}else if(insideAngle(line.dirInside, 225, 315)) {
							if(insideAngle(dir, line.dirInside + 160, line.dirInside + 190)
							&& line.canPause) {
								dir = -1;
							}else if(insideAngle(dir, line.dirInside - 180, line.dirInside - 90)) {
								dir = simplifyDeg(line.dirInside - 90);
							}
						}
					}

					if(Cam.getCanUse() && getCanUseGyro()) {
						rot = (catchingBall || isBallForward) && abs(goal.rotOpp) <= 3
							? Cam.multiRotGoal(goal.rotOpp)
							: multiRotGyro(gyro);
						if(goal.isInCorner) {
							if(signum(goal.rotOpp) > 0) {
								goal.isInCorner = insideAngle(dir, - 120 + gyro, 15 + gyro);
							}else {
								goal.isInCorner = insideAngle(dir, - 15 + gyro, 120 + gyro);
							}
						}
					}else if(Cam.getCanUse()) {
						rot = Cam.multiRotGoal(goal.rotOpp);
					}else if(getCanUseGyro()) {
						rot = multiRotGyro(gyro);
					}
				}else {
					ball.t = ball.t < 0 ? -1 : simplifyDeg(ball.t - gyro);
					int dirGK = isGoalCloseLazer ? 110 : goal.distGK > 0 ? 70 : 90;
					dir = ball.t < 0 ? -1 : signum(ball.t - 180) * dirGK + 180;

					rot = multiRotGyro(gyro);
				}
				if(!carryingBall) {
					correctRot(isFW, gyro);
				}
				run(isFW, ball, dir, rot, gyro, goal, isGoalCloseLazer, isGoalClose, fellow,
					catchingBall, catchFreely, isBallForward, isBallClose, enemyStandsFront, line);
			}
		}else {
			//待機
			//LCD表示
			LCD.run(gyro, line, Actuator.getCanUseKicker(), Cam.getCanUse(), getCanUseGyro(), isFW, Comc.getCanUse(), fellow,
				Line.getQTY(), Line.getValue(), Line.getState(),Ina219.getValue(), goal,
				ball, Ball.getQTY(), Ball.getValue(),
				Ball.getValueCatch(), catchingBall, Ball.getForward(), isBallForward, isBallClose,
				frontPSD.getValue(), enemyStandsFront, backPSD.getValue(), isGoalCloseLazer);
			//駆動
			Actuator.run(-1, 0, 0);
			Actuator.checkKick();
			//FW or GK
			if(digitalRead(P_CHANGE_ROLE)) {
				isFW = !isFW;
				while(digitalRead(P_CHANGE_ROLE)) {  }
			}
		}
	}
}



void loop2() {
	setIR();
}