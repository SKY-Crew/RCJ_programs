void stop() {
	// 駆動
	Motor.run(false, 0, 0);
	// LCD表示
	LCD.clear(true);
	LCD.write("THE VOLTAGE", 5, 1);
	LCD.write("IS TOO LOW!!!", 4, 2);
}

void wait(data_t *d) {
	// LCD表示
	LCD.run(d->gyro, d->line, Cam.getCanUse(), bool(d->gyro), isFW, Comc.getCanUse(), d->fellow,
		Line.getQTY(), Line.getVal(), Line.getState(),INA219.getVal(), d->goal,
		d->ball, Ball.getQTY(), Ball.getVal(), Ball.getValInAir(), Ball.getIsInAir(),
		Ball.getValCatch(), d->catchingBall, Ball.getForward(), d->isBallForward, d->distBall,
		frontPSD.getVal(), backPSD.getVal(), d->enemyStands, d->distGoalPSD, d->distGoal);
	// 駆動
	Motor.run(false, 0, 0);
	Kicker.check();

	cLineForward.reset();
}


void correctRot(bool isFW, Angle gyro) {
	const uint16_t THRE_INCREASE_cCR = 60;
	const uint16_t THRE_DECREASE_cCR = 40;
	cCorrectRot.set_COUNT_UP(!bool(cCorrectRot));
	if(bool(cCorrectRot)) {
		// 駆動
		int16_t powerCorrectRot = abs(gyro) >= THRE_INCREASE_cCR
				? signum(gyro) * 160
				: signum(gyro) * 100;
		Motor.run(false, powerCorrectRot, 0);
		// correctRot継続
		cCorrectRot.increase(abs(gyro) >= THRE_DECREASE_cCR);
	}else {
		// correctRot開始
		cCorrectRot.increase(abs(gyro) >= THRE_INCREASE_cCR);
	}
}

void carryBall(bool isFW, int16_t rot, cam_t goal, Angle gyro, bool catchingBall, bool enemyStandsFront) {
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
			// スタック
			Motor.run(false, rot, 0);
		}
		// carry続けるか
		willCarryBall = Ball.compareCatch(0.3);
	}else {
		// carry始めるか
		willCarryBall = catchingBall;
		if(willCarryBall) {
			timeStartCB = millis();
		}
	}
}

void ballInAir(bool isBallInAir, int16_t rot, Angle rotOwn, Dist distGoal, vectorRT_t ball) {
	if(isBallInAir && Cam.getCanUse()) {
		// ボール真上前方・ボールない
		if(distGoal >= FAR) {
			Motor.run(180, rot, 190);
		}else if(distGoal > CLOSE) {
			Motor.run(rotOwn, rot, 160);
		}else if(!rotOwn.isDown(10) && distGoal >= CLOSE) {
			Motor.run(signum(rotOwn) * 90, rot, 110);
		}else {
			Motor.run(distGoal < CLOSE ? 0 : Angle(false), rot, 100);
		}
	}else if(isBallInAir && ball.t.isUp(60)) {
		Motor.run(false, rot, 0);
	}
}

void run(data_t *d, bool isFW, Angle dir, int16_t rot) {
	willCarryBall = false;
	if(isFW) {
		// FW
		bool leavingLine = bool(d->line.dirInside) && abs(dir - d->line.dirInside) <= 90;
		carryBall(isFW, rot, d->goal, d->gyro,
				d->catchingBall || d->isBallForward, d->enemyStands[0]);
		ballInAir(Ball.getIsInAir() || !bool(d->ball.t),
				rot, d->goal.rotOwn, d->distGoal, d->ball);
		if(d->ball.t.isRight(45) || d->ball.t.isLeft(45)) {
			//ボール横
			Motor.run(dir, rot, (leavingLine || d->distGoal == CLOSE) ? 140 : 190);
		}else if(d->ball.t.isDown(45) || d->distBall >= FAR) {
			// ボール(後方|遠く)
			Motor.run(dir, rot, (leavingLine || d->distGoal == CLOSE) ? 130 : 160);
		}else {
			// ボール前方
			Motor.run(dir, rot, leavingLine ? 100 : 130);
		}
		Kicker.run(d->catchFreely && d->goal.isOppWide);
	}else {
		// GK
		d->isBallForward |= d->ball.t.isUp(10) && d->distBall == CLOSE;
		if(d->goal.diffOwn == TOO_LARGE
				|| (d->goal.diffOwn == LARGE && d->distBall > CLOSE)) {
			// 横行きすぎ・横&ボール遠く
			Motor.run(90 * (- d->goal.sideOwn), rot, 150);
		}else if(d->distGoal == TOO_FAR && (d->ball.t.isUp(90) || !bool(d->ball.t))) {
			// ゴール遠すぎ&ボール(前方|ない)
			Motor.run(d->goal.rotOwn ? d->goal.rotOwn : 180, rot, 200);
		}else if(d->distGoal == TOO_FAR && d->distBall <= CLOSE) {
			// ゴール遠すぎ&ボール(後方)近く
			Motor.run(Ball.getDir(d->ball), rot, 200);
		}
		// ボール捕獲
		carryBall(isFW, rot, d->goal, d->gyro, d->catchingBall, false);
		if(d->isBallForward) {
			// ボール前方直線上
			Motor.run(0, rot, 150);
		}else if(d->ball.t.isDown(80) && d->distGoal <= PROPER) {
			// ボール後方&ゴール遠くない
			Motor.run(false, rot, 0);
		}else if(d->goal.diffOwn == SMALL && d->distBall <= CLOSE) {
			// 少し横&ボール近く
			Motor.run(dir * signum(d->ball.t), rot, 90);
		}else if(!bool(d->ball.t)
				|| d->ball.t.isUp(5)
				|| d->goal.sideOwn * signum(d->ball.t) == 1) {
			// ボールない(ゴール遠くない)・ボール前方(遠く)・ボール外側(自分端)
			Motor.run(d->distGoal == CLOSE ? 0 : d->distGoal >= FAR ? 180 : Angle(false),
					rot, 60);
		}else if(d->ball.t.isDown(80) && d->distBall <= CLOSE){
			//ボール後方近く(ゴール遠い)
			Motor.run(Ball.getDir(d->ball), rot, 140);
		}else {
			//ボール(前方|遠い)(ゴール遠い)
			Motor.run(dir * signum(d->ball.t), rot, 140);
		}
		Kicker.run(d->catchFreely && !d->fellow.exists);
	}
	carryingBall = willCarryBall && d->ball.t.isUp(20) && d->distBall == CLOSE;
}