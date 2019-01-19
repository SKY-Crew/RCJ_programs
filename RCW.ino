#include "Include.h"

const bool IS_SKY = false;

const uint8_t P_START = 28;
bool canRun;
bool prvCanRun;

const uint8_t P_CHANGE_ROLE = 32;
const uint8_t P_IS_FW = 31;
bool isFW = IS_SKY;
bool prvIsFW;
Count cBecomeFW(100, false);

uint16_t BORDER_IF;
uint16_t BORDER_IC;

Count cCorrectRot(3);
bool correctingRot = false;
uint16_t BORDER_INCREASE_CCR;
uint16_t BORDER_DECREASE_CCR;

Count cEnemyStandsFront(8, false);

bool carryingBall = false;
bool willCarryBall = false;
const bool BORDER_CONTINUE_CARRY = 0.3;
uint32_t timeStartCB;

Count cCatchFreely;

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
	cBecomeFW.increment(!(isFW && (!prvIsFW || (canRun && !prvCanRun))));
	prvIsFW = isFW;
	digitalWrite(P_IS_FW, isFW);
	if(INA219.checkVolt() && !Actuator.getIsKicking()) {
		//電池残量少
		canRun = false;
		stop();
	}else {
		//get
		vectorRT_t ball = Ball.get(false);
		bool isBallClose = ball.r >= BORDER_IC;
		bool isBallForward = Ball.getForward() >= BORDER_IF && isBallClose;
		bool catchingBall = Ball.getCatch() && ball.t.inside(330, 30) && isBallClose;
		Angle gyro = getGyro();
		line_t line = Line.get(isFW, getCanUseGyro(), gyro);
		cam_t goal = Cam.get();
		bool isGoalClose = false;
		int16_t rot = 0;
		comc_t fellow = Comc.communicate(canRun, isFW);
		bool isGoalCloseLazer = backPSD.get();
		cEnemyStandsFront.increment(!frontPSD.get());
		bool enemyStandsFront = bool(cEnemyStandsFront);
		checkRole(!bool(cBecomeFW), fellow);
		cCatchFreely.increment(catchingBall && !enemyStandsFront);
		bool catchFreely = bool(cCatchFreely) && (isFW || goal.distGK >= 2 || !Cam.getCanUse());

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
				Angle dir = Ball.getDir(ball.t, isBallClose);
				if(!isFW) {
					isFW = (catchFreely || line.isInAir) && fellow.exists;
				}
				if(isFW) {
					if(fellow.exists) {
						if(ball.t.inside(90, 270)) {
							switch (goal.distFW) {
							case 1: dir = ball.t.inside(170, 190) ? -1
										: ball.t.inside(90, 180) ? 90 : 270;
									isGoalClose = true;
									break;
							case 0: dir = ball.t.inside(170, 190) ? 0
										: ball.t.inside(90, 180) ? 50 : 310;
									isGoalClose = false;
									break;
							}
						}
					}
					if(bool(line.dirInside)) {
						if(line.dirInside.inside(45, 135)) {
							if(dir.inside(line.dirInside + 170, line.dirInside + 200)
							&& line.canPause) {
								dir = false;
							}else if(dir.inside(line.dirInside + 90, line.dirInside + 180)) {
								dir = line.dirInside + 90;
							}
						}else if(line.dirInside.inside(225, 315)) {
							if(dir.inside(line.dirInside + 160, line.dirInside + 190)
							&& line.canPause) {
								dir = false;
							}else if(dir.inside(line.dirInside - 180, line.dirInside - 90)) {
								dir = line.dirInside - 90;
							}
						}
					}

					if(Cam.getCanUse() && getCanUseGyro()) {
						rot = (catchingBall || isBallForward) && abs(goal.rotOpp) <= 3
							? Cam.multiRotGoal(goal.rotOpp)
							: multiRotGyro(gyro);
						if(goal.isInCorner) {
							if(signum(goal.rotOpp) > 0) {
								goal.isInCorner = dir.inside(- 120 + gyro, 15 + gyro);
							}else {
								goal.isInCorner = dir.inside(- 15 + gyro, 120 + gyro);
							}
						}
					}else if(Cam.getCanUse()) {
						rot = Cam.multiRotGoal(goal.rotOpp);
					}else if(getCanUseGyro()) {
						rot = multiRotGyro(gyro);
					}
				}else {
					ball.t = bool(ball.t) ? (ball.t - gyro) : false;
					uint16_t dirGK = isGoalCloseLazer ? 110 : goal.distGK > 0 ? 70 : 90;
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
				Line.getQTY(), Line.getValue(), Line.getState(),INA219.getValue(), goal,
				ball, Ball.getQTY(), Ball.getValue(),
				Ball.getValueCatch(), catchingBall, Ball.getForward(), isBallForward, isBallClose,
				frontPSD.getValue(), enemyStandsFront, backPSD.getValue(), isGoalCloseLazer);
			//駆動
			Actuator.run(false, 0, 0);
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