#include "Include.h"

const bool IS_SKY = true;

bool canRun;
bool prvCanRun;

bool prvChangeRole = false;
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

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);
}

void loop() {
	//駆動重複リセット
	Actuator.setHaveRun(false);
	//走行可か
	prvCanRun = canRun;
	canRun = digitalRead(P_START);
	//Role強制変更
	cBecomeFW.increase(isFW && (!prvIsFW || (canRun && !prvCanRun)));
	prvIsFW = isFW;
	digitalWrite(P_IS_FW, isFW);

	if(INA219.checkVolt() && !Actuator.getIsKicking()) {
		//電池残量少
		canRun = false;
		stop();
	}else {
		//get
		data_t d;
		d.gyro = Gyro.get();
		d.goal = Cam.get();
		d.isGoalClose = false;
		d.isGoalCloseLazer = backPSD.get();
		
		cEnemyStandsFront.increase(frontPSD.get());
		d.enemyStandsFront = bool(cEnemyStandsFront);
		d.fellow = Comc.communicate(canRun, isFW);

		d.ball = Ball.get(false);
		d.isBallClose = d.ball.r >= BORDER_IC;
		d.isBallForward = Ball.getForward() >= BORDER_IF && d.isBallClose;
		d.catchingBall = Ball.getCatch() && d.ball.t.inside(330, 30) && d.isBallClose;
		cCatchFreely.increase(d.catchingBall && !d.enemyStandsFront);
		d.catchFreely = bool(cCatchFreely) && (isFW || d.goal.distGK >= 2 || !Cam.getCanUse());

		d.line = Line.get(isFW, Gyro.getCanUse(), d.gyro);

		int16_t rot = 0;
		//Role受動的変更
		checkRole(!bool(cBecomeFW), d.fellow);
		if(canRun) {
			//走行中
			if(!prvCanRun) {
				//スタート直後
				d.line.isOutside = false;
				LCD.clear(true);
				LCD.write("Running!", 0, 0);
			}
			if(d.line.isOutside) {
				//ライン復帰
				Actuator.run(d.line.dirInside, rot, 150);
			}else{
				//dir計算
				Angle dir = Ball.getDir(d.ball.t, d.isBallClose);
				if(!isFW) {
					//Role能動的変更
					isFW = (d.catchFreely || d.line.isInAir) && d.fellow.exists;
				}
				if(isFW) {
					//FW
					if(d.fellow.exists) {
						//マルチ対策
						if(d.ball.t.inside(90, 270)) {
							switch (d.goal.distFW) {
							//少し後ろ
							case 1: dir = d.ball.t.inside(170, 190) ? Angle(false)
										: d.ball.t.inside(90, 180) ? 90 : 270;
									d.isGoalClose = true;
									break;
							//後ろ過ぎ
							case 0: dir = d.ball.t.inside(170, 190) ? 0
										: d.ball.t.inside(90, 180) ? 50 : 310;
									d.isGoalClose = false;
									break;
							}
						}
					}
					if(bool(d.line.dirInside)) {
						//ライン上
						////gyro考慮 absolute 角検知易しく
						if(d.line.dirInside.inside(45, 135)) {
							if(dir.inside(d.line.dirInside + 170, d.line.dirInside + 200)
								&& d.line.canPause) {
								//停止
								dir = Angle(false);
							}else if(dir.inside(d.line.dirInside + 90, d.line.dirInside + 180)) {
								//後退
								dir = d.line.dirInside + 90;
							}
						}else if(d.line.dirInside.inside(225, 315)) {
							if(dir.inside(d.line.dirInside + 160, d.line.dirInside + 190)
								&& d.line.canPause) {
								//停止
								dir = Angle(false);
							}else if(dir.inside(d.line.dirInside - 180, d.line.dirInside - 90)) {
								//後退
								dir = d.line.dirInside - 90;
							}
						}
					}
					//rot計算
					if(Cam.getCanUse() && Gyro.getCanUse()) {
						//両方使用可
						rot = (d.catchingBall || d.isBallForward) && abs(d.goal.rotOpp) <= 3
							? Cam.multiRotGoal(d.goal.rotOpp)
							: Gyro.multiRot(d.gyro);
						if(d.goal.isInCorner) {
							//角検知
							if(signum(d.goal.rotOpp) > 0) {
								d.goal.isInCorner = dir.inside(- 120 + d.gyro, 15 + d.gyro);
							}else {
								d.goal.isInCorner = dir.inside(- 15 + d.gyro, 120 + d.gyro);
							}
						}
					}else if(Cam.getCanUse()) {
						//camのみ
						rot = Cam.multiRotGoal(d.goal.rotOpp);
					}else if(Gyro.getCanUse()) {
						//gyroのみ
						rot = Gyro.multiRot(d.gyro);
					}
				}else {
					//GK
					//dir計算
					d.ball.t = bool(d.ball.t) ? d.ball.t - d.gyro : Angle(false);
					Angle dirGK = d.isGoalCloseLazer ? 110 : d.goal.distGK > 0 ? 70 : 90;
					dir = bool(d.ball.t) ? dirGK * signum(d.ball.t) : Angle(false);
					//rot計算
					rot = Gyro.multiRot(d.gyro);
				}
				if(!carryingBall) {
					//姿勢その場修正
					correctRot(isFW, d.gyro);
				}
				//駆動
				run(d, isFW, dir, rot);
			}
		}else {
			//待機
			//LCD表示
			LCD.run(d.gyro, d.line, Actuator.getCanUseKicker(), Cam.getCanUse(), Gyro.getCanUse(), isFW, Comc.getCanUse(), d.fellow,
				Line.getQTY(), Line.getValue(), Line.getState(),INA219.getValue(), d.goal,
				d.ball, Ball.getQTY(), Ball.getValue(),
				Ball.getValueCatch(), d.catchingBall, Ball.getForward(), d.isBallForward, d.isBallClose,
				frontPSD.getValue(), d.enemyStandsFront, backPSD.getValue(), d.isGoalCloseLazer);
			//駆動
			Actuator.run(false, 0, 0);
			Actuator.checkKick();
			//Role強制変更
			if(digitalRead(P_CHANGE_ROLE) && !prvChangeRole) {
				isFW = !isFW;
			}
			prvChangeRole = digitalRead(P_CHANGE_ROLE);
		}
	}
}



void loop2() {
	//IR調整
	setIR();
}