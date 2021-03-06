void stop() {
	// 駆動
	Motor.run(false, 0, 0);
	// LCD表示
	LCD.clear(true);
	LCD.write("THE VOLTAGE", 5, 1);
	LCD.write("IS TOO LOW!!!", 4, 2);
	//音
	Buzzer.set(88, 100, true, 10000);
}

void wait(data_t *d) {
	// Buzzer
	Buzzer.set(30, 200, Gyro.resetting(), 60);
	Buzzer.set(35, 200, prvChangeRole, 60);
	// LCD表示
	LCD.run(d->gyro, d->line, Cam.getCanUse(), bool(d->gyro), isFW, Comc.getCanUse(), d->fellow,
		Line.getQTY(), Line.getVal(), Line.getState(),INA219.getVal(), d->goal,
		d->ball, Ball.getQTY(), Ball.getVal(),
		Ball.getValCatch(), d->catchingBall, Ball.getForward(), d->isBallForward, d->distBall,
		frontPSD.getVal(), d->valBackPSD, d->enemyStands, d->distGoalPSD, d->distGoal);
	// 駆動
	Motor.run(false, 0, 0);
	Kicker.check();

	cLineForward.reset();
	cLineBackward.reset();
}

/*
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
*/
void carryBall(bool isFW, int16_t rot, cam_t goal, Angle gyro, bool catchingBall, bool enemyStandsFront, bool leavingLine, bool isBallForward) {
	willCarryBall = carryingBall;
	if(carryingBall) {
		cLineForward.increase(false);
		powerCB = max(powerCB, isBallForward ? 200 : 230);
		if(millis() - timeStartCB < 1500) {
			if(isFW) {
				if(!bool(goal.rotOpp)) {
					Motor.run(0, Gyro.calRot(0), powerCB);
				}else if(Cam.getCanUse()) {
					Angle dir = catchingBall ? absCon(double(goal.rotOpp - gyro) * 0.7, 70) : 0;
					Motor.run(dir, Cam.calRot(goal.rotOpp), powerCB * conMap(double(abs(dir)), 0, 45, 1, 1.4));
				}else {
					Motor.run(0, rot, powerCB);
				}
			}else {
				Motor.run(0, rot, powerCB);
			}
		}else {
			// スタック
			Motor.run(false, rot, 0);
		}
		// carry続けるか
		willCarryBall = Ball.compareCatch(0) || isBallForward;
	}else {
		// carry始めるか
		willCarryBall = catchingBall || isBallForward;
		if(willCarryBall) {
			timeStartCB = millis();
			powerCB = 0;
		}
	}
}

void ballInAir(bool isBallInAir, int16_t rot, Angle gyro, Angle rotOwn, Angle rotOpp, Dist distGoal, vectorRT_t ball) {
	if(isBallInAir && Cam.getCanUse()) {
		// ボール真上前方・ボールない
		cLineForward.reset();
		cLineBackward.reset();
		if(distGoal >= FAR) {
			if(rotOpp.isUp(10) || !bool(rotOpp)) {
				Motor.run(180, rot, 100);
			}else {
				Motor.run(rotOpp + signum(rotOpp - gyro) * 90, rot, 200);
			}
		}else if(distGoal > CLOSE) {
			Motor.run(rotOwn, rot, 160);
		}else if(!bool(rotOwn) && distGoal >= CLOSE) {
			Motor.run(180, rot, 100);
		}else if(!rotOwn.isDown(10) && distGoal >= CLOSE) {
			Motor.run(signum(rotOwn) * 90, rot, 110);
		}else {
			Motor.run(distGoal < CLOSE ? 0 : Angle(false), rot, 100);
		}
	}else if(isBallInAir && ball.t.isUp(60)) {
		cLineForward.reset();
		cLineBackward.reset();
		Motor.run(false, rot, 0);
	}
}

void run(data_t *d, bool isFW, Angle dir, int16_t rot) {
	willCarryBall = false;
	if(isFW) {
		// FW
		bool leavingLine = bool(d->line.dirInside) && (dir - d->line.dirInside).isDown(45);
		carryBall(isFW, rot, d->goal, d->gyro,
				d->catchingBall, d->enemyStands[0], leavingLine, d->isBallForward && !cLineForward.compare(0));
		ballInAir(!bool(d->ball.t), rot, d->gyro, d->goal.rotOwn, d->goal.rotOpp, d->distGoal, d->ball);
		if(cLineForward.compare(0)) {
			Motor.run(dir, Gyro.calRot(0), 140);
		}
		if(cLineBackward.compare(0)) {
			Motor.run(dir, Gyro.calRot(0), 140);
		}
		if(d->ball.t.isDown(45) || d->distBall >= FAR) {
			// ボール(後方|遠く)
			Motor.run(dir, rot, (leavingLine || d->distGoal == CLOSE) ? 160 : 190);
		}else {
			// ボール(前方|横)
			Motor.run(dir, rot,
					conMap(double(abs(d->ball.t)), 10, 45, 120, 190)
					* (leavingLine ? 0.7 : 1));
		}
		Kicker.run(d->catchFreely);
	}else {
		// GK
		if(d->goal.diffOwn >= LARGE) {
			// 横行きすぎ・横&ボール遠く
			Motor.run(90 * (- d->goal.sideOwn), rot, 150);
		}else if(d->distGoal == TOO_FAR && (d->ball.t.isUp(90) || !bool(d->ball.t))) {
			// ゴール遠すぎ&ボール(前方|ない)
			Motor.run(bool(d->goal.rotOwn) ? d->goal.rotOwn : 180, rot, 200);
		}else if(d->distGoal == TOO_FAR && d->distBall <= CLOSE) {
			// ゴール遠すぎ&ボール(後方)近く
			Motor.run(Ball.getDir(d->ball), rot, 200);
		}else if(!(d->goal.rotOwn - d->gyro).isDown(10) && d->distGoal == PROPER && !bool(d->ball.t)) {
			// ゴール真後ろでない&ボールない
			Motor.run(signum(d->goal.rotOwn - d->gyro) * 90, rot, 130);
		}
		// ボール捕獲
		carryBall(isFW, rot, d->goal, d->gyro, d->catchingBall, false, false, false);
		if(d->isBallForward) {
			// ボール前方直線上
			Motor.run(d->distGoal >= (d->fellow.exists ? FAR : TOO_FAR) ? Angle(false) : 0, rot, 45);
		}else if(d->ball.t.isDown(45) && d->distGoal <= PROPER) {
			// ボール後方&ゴール遠くない
			Motor.run(false, rot, 0);
		/*}else if(d->goal.diffOwn == SMALL && d->distBall <= CLOSE && d->goal.sideOwn * signum(d->ball.t) == 1) {
			// 少し横&ボール近く
			Motor.run(dir, rot, 90);*/
		}else if(!bool(d->ball.t)
				|| d->ball.t.isUp(d->distBall <= PROPER ? 3 : 3)
				|| d->goal.sideOwn * signum(d->ball.t) == 1) {
			// ボールない(ゴール遠くない)・ボール前方(遠く)・ボール外側(自分端)
			Motor.run(d->distGoal == CLOSE ? 0 : d->distGoal >= FAR ? 180 : Angle(false),
					rot, 50);
		}else if(d->ball.t.isDown(45) && d->distBall <= CLOSE){
			//ボール後方近く(ゴール遠い)
			Motor.run(Ball.getDir(d->ball), rot, 140);
		}else if(d->distBall >= FAR) {
			Motor.run(dir, rot,
					conMap(double(abs(d->ball.t)), 3, 15, 100, 140, 30, 140));
		}else {
			Motor.run(dir, rot,
					d->ball.t.isUp(20) ?
							conMap(double(abs(d->ball.t)), 5, 10, 120, 180, 100, 180)
					: d->ball.t.isUp(45) ? 200 : 230);
		}
		Kicker.run(d->catchFreely && !d->fellow.exists);
	}
	carryingBall = willCarryBall && d->ball.t.isUp(20) && d->distBall == CLOSE;
}