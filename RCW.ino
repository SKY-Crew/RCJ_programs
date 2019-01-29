#include "Include.h"

const bool IS_SKY = false;

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
	gyroSetup();

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);
}
Angle prvBall;////
Count cBallUp(5, false);
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
		vectorRT_t ball = Ball.get(false);

		cBallUp.increase(bool(prvBall) && diff(ball.t, prvBall) > 10);
		prvBall = ball.t;
		if(bool(cBallUp)) {
			ball.t = false;
		}
		Serial.println(int16_t(cBallUp));

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
		cEnemyStandsFront.increase(frontPSD.get());
		bool enemyStandsFront = bool(cEnemyStandsFront);
		cCatchFreely.increase(catchingBall && !enemyStandsFront);
		bool catchFreely = bool(cCatchFreely) && (isFW || goal.distGK >= 2 || !Cam.getCanUse());
		//Role受動的変更
		checkRole(!bool(cBecomeFW), fellow);
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
				//dir計算
				Angle dir = Ball.getDir(ball.t, isBallClose);
				if(!isFW) {
					//Role能動的変更
					isFW = (catchFreely || line.isInAir) && fellow.exists;
				}
				if(isFW) {
					//FW
					if(fellow.exists) {
						//マルチ対策
						if(ball.t.inside(90, 270)) {
							switch (goal.distFW) {
							//少し後ろ
							case 1: dir = ball.t.inside(170, 190) ? Angle(false)
										: ball.t.inside(90, 180) ? 90 : 270;
									isGoalClose = true;
									break;
							//後ろ過ぎ
							case 0: dir = ball.t.inside(170, 190) ? 0
										: ball.t.inside(90, 180) ? 50 : 310;
									isGoalClose = false;
									break;
							}
						}
					}
					if(bool(line.dirInside)) {
						//ライン上
						////gyro考慮 absolute 角検知易しく
						if(line.dirInside.inside(45, 135)) {
							if(dir.inside(line.dirInside + 170, line.dirInside + 200)
								&& line.canPause) {
								//停止
								dir = Angle(false);
							}else if(dir.inside(line.dirInside + 90, line.dirInside + 180)) {
								//後退
								dir = line.dirInside + 90;
							}
						}else if(line.dirInside.inside(225, 315)) {
							if(dir.inside(line.dirInside + 160, line.dirInside + 190)
								&& line.canPause) {
								//停止
								dir = Angle(false);
							}else if(dir.inside(line.dirInside - 180, line.dirInside - 90)) {
								//後退
								dir = line.dirInside - 90;
							}
						}
					}
					//rot計算
					if(Cam.getCanUse() && getCanUseGyro()) {
						//両方使用可
						rot = (catchingBall || isBallForward) && abs(goal.rotOpp) <= 3
							? Cam.multiRotGoal(goal.rotOpp)
							: multiRotGyro(gyro);
						if(goal.isInCorner) {
							//角検知
							if(signum(goal.rotOpp) > 0) {
								goal.isInCorner = dir.inside(- 120 + gyro, 15 + gyro);
							}else {
								goal.isInCorner = dir.inside(- 15 + gyro, 120 + gyro);
							}
						}
					}else if(Cam.getCanUse()) {
						//camのみ
						rot = Cam.multiRotGoal(goal.rotOpp);
					}else if(getCanUseGyro()) {
						//gyroのみ
						rot = multiRotGyro(gyro);
					}
				}else {
					//GK
					//dir計算
					ball.t = bool(ball.t) ? ball.t - gyro : Angle(false);
					Angle dirGK = isGoalCloseLazer ? 110 : goal.distGK > 0 ? 70 : 90;
					dir = bool(ball.t) ? dirGK * signum(ball.t) : Angle(false);
					//rot計算
					rot = multiRotGyro(gyro);
				}
				if(!carryingBall) {
					//姿勢その場修正
					correctRot(isFW, gyro);
				}
				//駆動
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