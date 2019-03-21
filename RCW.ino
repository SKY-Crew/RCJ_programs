#include "Include.h"
#include "Declare.h"

void setup() {
	delay(1000);
	Serial.begin(9600);

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);
}

void loop() {
	//駆動重複リセット
	Motor.setHaveRun(false);
	Kicker.setHaveCheckKick(false);
	//走行可か
	prvCanRun = canRun;
	canRun = digitalRead(P_START);
	//Role強制変更
	cBecomeFW.increase(isFW && (!prvIsFW || (canRun && !prvCanRun)));
	prvIsFW = isFW;
	digitalWrite(P_IS_FW, isFW);
	if(INA219.checkVolt() && !Kicker.getIsKicking()) {
		//電池残量少
		canRun = false;
		stop();
	}else {
		//get
		data_t d;
		get(&d);
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
				Motor.run(d.line.dirInside, 0, 150);
			}else{
				if(!isFW) {
					//Role能動的変更
					isFW = (d.catchFreely || d.line.isInAir) && d.fellow.exists;
				}
				if(!isFW) {
					//gyro考慮
					d.ball.t = bool(d.ball.t) ? d.ball.t - d.gyro : Angle(false);
				}
				//dir計算
				Angle dir = calDir(isFW, d.ball, d.gyro, d.goal, d.isGoalClosePSD, d.distBall);
				if(isFW) {
					//マルチ対策
					d.isGoalClose = avoidMulDef(&dir, d.fellow, d.ball, d.goal);
					//ライン角
					detectCornerLine(&dir, d.line, d.gyro);
				}
				//rot計算
				int16_t rot = calRot(isFW, d.goal, d.gyro, d.catchingBall, d.isBallForward);
				if(!carryingBall) {
					//姿勢その場修正
					correctRot(isFW, d.gyro);
				}
				//駆動
				run(&d, isFW, dir, rot);
			}
		}else {
			//待機
			wait(&d);
			//Role強制変更
			if(digitalRead(P_CHANGE_ROLE) && !prvChangeRole) {
				isFW = !isFW;
			}
			prvChangeRole = digitalRead(P_CHANGE_ROLE);
		}
	}
}