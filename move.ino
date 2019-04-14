void stop() {
	//駆動
	Motor.run(false, 0, 0);
	//LCD表示
	LCD.clear(true);
	LCD.write("THE VOLTAGE", 5, 1);
	LCD.write("IS TOO LOW!!!", 4, 2);
}

void wait(data_t *d) {
	//LCD表示
	LCD.run(d->gyro, d->line, Cam.getCanUse(), bool(d->gyro), isFW, Comc.getCanUse(), d->fellow,
		Line.getQTY(), Line.getVal(), Line.getState(),INA219.getVal(), d->goal,
		d->ball, Ball.getQTY(), Ball.getVal(), Ball.getValInAir(), Ball.getIsInAir(),
		Ball.getValCatch(), d->catchingBall, Ball.getForward(), d->isBallForward, d->distBall,
		frontPSD.getVal(), backPSD.getVal(), d->enemyStands, d->distGoalPSD, d->distGoal);
	//駆動
	Motor.run(false, 0, 0);
	Kicker.check();
}


void correctRot(bool isFW, Angle gyro) {
	const uint16_t THRE_INCREASE_cCR = 60;
	const uint16_t THRE_DECREASE_cCR = 40;
	cCorrectRot.set_COUNT_UP(!bool(cCorrectRot));
	if(bool(cCorrectRot)) {
		//駆動
		int16_t powerCorrectRot = abs(gyro) >= THRE_INCREASE_cCR
				? signum(gyro) * 160
				: signum(gyro) * 100;
		Motor.run(false, powerCorrectRot, 0);
		//correctRot継続
		cCorrectRot.increase(abs(gyro) >= THRE_DECREASE_cCR);
	}else {
		//correctRot開始
		cCorrectRot.increase(abs(gyro) >= THRE_INCREASE_cCR);
	}
}

void carryBall(bool isFW, line_t line, int16_t rot, cam_t goal, Angle gyro, bool catchingBall, bool enemyStandsFront) {
	willCarryBall = carryingBall;
	if(carryingBall) {
		cLineForward.increase(false);
		if(millis() - timeStartCB < 1500) {
			if(isFW) {
				if(enemyStandsFront) {
					Motor.run(bool(gyro) ? gyro * (-0.5) : 0, Gyro.multiRot(goal.rotOpp), 180); ////
				}
			}else {
				Motor.run(0, rot, 170);
			}
		}else {
			//スタック
			Motor.run(false, rot, 0);
		}
		//carry続けるか
		willCarryBall = Ball.compareCatch(0.3);
	}else {
		//carry始めるか
		willCarryBall = catchingBall;
		if(willCarryBall) {
			timeStartCB = millis();
		}
	}
}


void run(data_t *d, bool isFW, Angle dir, int16_t rot) {
	willCarryBall = false;
	if(isFW) {
		//FW
		bool leavingLine = bool(d->line.dirInside) && abs(dir - d->line.dirInside) <= 90;
		carryBall(isFW, d->line, rot, d->goal, d->gyro, d->catchingBall || d->isBallForward, d->enemyStands[0]);
		if(Ball.getIsInAir() && d->ball.t.isUp(60)) {
			//ボール真上前方
			Motor.run(false, rot, 0);
		}else if(d->isBallForward) {
			//ボール前方直線上
			cLineForward.increase(false);
			Motor.run(dir, rot, leavingLine ? 120 : 160);
		}else if(d->ball.t.isDown(70) || d->distBall >= FAR) {
			//ボール後方or遠く
			Motor.run(dir, rot, (leavingLine || d->distGoal == CLOSE) ? 100 : 180);
		}else {
			//ボール前方
			Motor.run(dir, rot, leavingLine ? 90 : 105);
		}
		Kicker.kick(d->catchFreely && d->goal.isWide);
	}else {
		//GK
		d->isBallForward |= d->ball.t.isUp(10) && d->distBall == CLOSE;
		if(d->goal.diffOwn == LARGE) {
			//横行きすぎ
			Motor.run(180 - d->goal.rotOwn * (d->distGoal == TOO_FAR ? 60 : 100), rot, 120);
		}else if(d->distGoal == TOO_FAR) {
			//ゴールとても遠すぎ
			Motor.run(180, rot, 160);
		}
		//ボール捕獲
		carryBall(isFW, d->line, rot, d->goal, d->gyro, d->catchingBall, false);
		if(d->isBallForward) {
			//ボール前方
			Motor.run(0, rot, d->fellow.exists ? 40 : 100);
		}else if(d->distBall == CLOSE && abs(d->goal.rot) >= 1 && signum(d->ball.t - 180) == signum(d->goal.rot)) {
			Motor.run(Ball.getDir(d->ball), 0, 40);
		}else if(!bool(d->ball.t)
				|| (d->goal.rotOwn != signum(d->ball.t) && d->goal.diffOwn >= LARGE)
				|| d->ball.t.isDown(30)
				|| d->ball.t.isUp(5)) {
			//ボールない・ボール外側・ボール後ろ・ボール前方遠く
			Motor.run(d->distGoal == CLOSE ? 0 : d->distGoal >= FAR ? 180 : Angle(false),
				d->distGoal == PROPER ? signum(rot) * 60 : 0, 80);
		}else if(d->ball.t.isUp(20) || d->goal.rotOwn != signum(dir)) {
			//ボールある程度前方・少し横
			Motor.run(dir, rot, 120);
		}else {
			//ボール斜め前方
			Motor.run(dir, rot, 210);
		}
		Kicker.kick(d->catchFreely && !d->fellow.exists);
	}
	carryingBall = willCarryBall && d->ball.t.isUp(20) && d->distBall == CLOSE;
}