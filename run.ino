void stop() {
	Actuator.run(-1, 0, 0);
	LCD.clear(true);
	LCD.write("THE VOLTAGE", 5, 1);
	LCD.write("IS TOO LOW!!!", 4, 2);
}

void checkRole(bool canBecomeGK, comc_t fellow) {
	if(Comc.getCanUse()) {
		if(fellow.exists && isFW == fellow.isFW) {
			if(canBecomeGK && isFW) {
				isFW = false;
				countCatchFreely = 0;
			}else if(!isFW && !canRun) {
				isFW = true;
			}else if(!isFW && IS_SKY) {
				isFW = true;
			}
		}
		if(canRun && !fellow.exists && isFW && canBecomeGK) {
			isFW = false;
			countCatchFreely = 0;
		}
	}

	BORDER_IF = isFW ? 610 : fellow.exists ? 670 : 610;
	BORDER_IC = isFW ? 350 : fellow.exists ? 400 : 350;
	BORDER_INCREASE_CCR = isFW ? 60 : 30;
	BORDER_DECREASE_CCR = isFW ? 40 : 5;
	MAX_CCF = isFW ? 3 : 1;
}

void correctRot(bool isFW, int16_t gyro) {
	correctingRot = correctingRot ? countCorrectRot > 0 : countCorrectRot >= MAX_CCR;
	if(correctingRot) {
		int16_t powerCorrectRot = abs(gyro) >= BORDER_INCREASE_CCR
			? signum(gyro) * (isFW ? 120 : 60)
			: signum(gyro) * (isFW ? 20 : 20);
		Actuator.run(-1, powerCorrectRot, 0);
		countCorrectRot = abs(gyro) < BORDER_DECREASE_CCR
			? countCorrectRot - 1
			: MAX_CCR;
	}else {
		countCorrectRot = abs(gyro) >= BORDER_INCREASE_CCR
			? min(MAX_CCR, countCorrectRot + 1)
			: 0;
	}
}

void carryBall(bool isFW, bool onLine, int16_t rot, int16_t gyro, bool catchingBall, bool enemyStandsFront) {
	willCarryBall = carryingBall;
	if(carryingBall) {
		if(Ball.getCountCatch() >= Ball.getMAX_C_CATCH() * 0.3 && millis() - timeStartCB < 1500) {
			if(abs(gyro) >= 30) {
				Actuator.run(simplifyDeg(signum(rot) * 40), 0, onLine ? 150 : isFW ? 230 : 200);
			}else {
				Actuator.run(0, rot * 1.5 + (enemyStandsFront ? signum(rot + 0.5) * 200 : 0), onLine ? 150 : isFW ? 230 : 200);
			}
		}else {
			Actuator.run(-1, 0, 0);
		}
		willCarryBall = Ball.getCountCatch() >= Ball.getMAX_C_CATCH() * 0.3;
	}else {
		willCarryBall = catchingBall;
		if(willCarryBall) {
			timeStartCB = millis();
		}
	}
}

void run(bool isFW, vectorRT_t ball, double dir, int16_t rot, int16_t gyro, cam_t goal,
	bool isGoalCloseLazer, bool isGoalClose, comc_t fellow, bool catchingBall, bool catchFreely,
	bool isBallForward, bool isBallClose, bool enemyStandsFront, line_t line) {
	willCarryBall = false;
	if(isFW) {
		bool onLine = line.dirInside != -1 && insideAngle(dir, line.dirInside + 90, line.dirInside - 90);
		carryBall(isFW, onLine, rot, gyro, catchingBall, enemyStandsFront);
		if(goal.isInCorner) {
			Actuator.run(-1, rot * 1.5, 0);
		}else if(isBallForward) {
			//ボール前方直線上
			Actuator.run(dir, rot * 1.5, onLine ? 120 : 210);
		}else if(insideAngle(dir, 90, 270)) {
			//ボール後方
			Actuator.run(dir, min(rot * 1.5, 60), (onLine || isGoalClose) ? 100 : 180);
		}else {
			//ボール前方
			Actuator.run(dir, min(rot * 1.5, 60), onLine ? 100 : 140);
		}
		Actuator.kick(catchFreely && goal.isWide);
	}else {
		isBallForward |= insideAngle(ball.t, 350, 10) && isBallClose;
		bool isTooFarGoal = goal.distGK >= 2 && !isGoalCloseLazer;
		bool isOnSide = abs(goal.rot) >= 3 || (isTooFarGoal && abs(goal.rot) >= 2);
		if(isOnSide) {
			//横行きすぎ
			Actuator.run(180 - signum(goal.rot) * (isTooFarGoal ? 60 : 100), rot, 120);
		}else if(isTooFarGoal) {
			//ゴールとても遠すぎ・仲間がいてゴール遠すぎ
			Actuator.run(180, rot, 160);
		}
		//ボール捕獲
		carryBall(isFW, false, rot, gyro, catchingBall, false);
		if(isBallForward) {
			//ボール前方
			Actuator.run(0, rot, fellow.exists ? 40 : 100);
		}else if(isBallClose && abs(goal.rot) >= 1 && signum(ball.t - 180) == signum(goal.rot)) {
			Actuator.run(Ball.getDir(ball.t, isBallClose), 0, 40);
		}else if(dir == -1
			||	(signum(ball.t - 180) == signum(goal.rot) && abs(goal.rot) >= 2)
			||	insideAngle(ball.t, 150, 210)
			||	insideAngle(ball.t, 355, 5)) {
			//ボールない・ボール外側・ボール後ろ(150~210)・ボール前方遠く
			Actuator.run(isGoalCloseLazer ? 0 : goal.distGK > 0 ? 180 : -1,
				(!isGoalCloseLazer && goal.distGK <= 0) ? signum(rot) * 60 : 0, 80);
		}else if(insideAngle(ball.t, 340, 20) || signum(goal.rot) == signum(dir - 180)) {
			//ボールある程度前方・少し横
			Actuator.run(dir, rot, 120);
		}else {
			//ボール斜め前方
			Actuator.run(dir, rot, 210);
		}
		Actuator.kick(catchFreely && !fellow.exists);
	}
	carryingBall = willCarryBall && insideAngle(ball.t, 340, 20) && isBallClose;
}