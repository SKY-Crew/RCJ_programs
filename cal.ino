void get(data_t *d) {
	d->gyro = Gyro.get();
	d->goal = Cam.get();
	Cam.send(double(d->gyro));

	d->enemyStands[0] = frontPSD.getBool(false);
	d->enemyStands[1] = backPSD.getBool(false);

	const uint16_t THRE_BACK_PSD[2] = {950, 1300};
	d->distGoalPSD = compare(backPSD.getVal(), THRE_BACK_PSD, 3, CLOSE);
	if(isFW) {
		const uint16_t THRE_DIST_FW[3] = {20, 34, 55};
		d->distGoal = compare(d->goal.distOwn, THRE_DIST_FW, 4, TOO_CLOSE);
	}else {
		const uint16_t THRE_DIST_GK[2] = {11, 20};
		Dist distGK = compare(d->goal.distOwn, THRE_DIST_GK, 3, PROPER);
		d->distGoal = Cam.getCanUse()
				&& (backPSD.getVal() > 4000 || d->goal.diffOwn >= SMALL || distGK == TOO_FAR)
				? distGK : d->distGoalPSD; ////
	}

	d->ball = Ball.get();

	const uint16_t THRE_DB[2] = {330, 260};
	d->distBall = compare(d->ball.r, THRE_DB, 3, CLOSE);
	d->isBallForward = d->distBall == CLOSE && d->ball.t.isUp(15) && Ball.getForward() >= 570;
	d->catchingBall = Ball.getCatch() && d->ball.t.isUp(30) && d->distBall == CLOSE;
	d->catchFreely = d->catchingBall && !d->enemyStands[0]
			&& (isFW || d->distGoal == TOO_FAR || !Cam.getCanUse());

	d->line = Line.get(isFW, d->gyro, Gyro.getDiff(), d->goal.isInCorner != 0);

	d->fellow = Comc.communicate(canRun, isFW, d->catchFreely ? 1000 : d->ball.r);
}


Angle calDir(bool isFW, vectorRT_t ball, Dist distGoal) {
	Angle dir;
	if(isFW) {
		dir = Ball.getDir(ball);
	}else {
		int16_t DIR_GKs[] = {110, 90, 70};
		dir = bool(ball.t)
				? DIR_GKs[min(distGoal , FAR) - CLOSE] * signum(ball.t)
				: Angle(false);
	}
	trace(11) { Serial.println("dir:"+str(dir)); }
	return dir;
}

int16_t calRot(bool isFW, cam_t goal, Angle gyro, bool catchingBall, bool isBallForward) {
	int16_t rot = 0;
	if(isFW) {
		if(Cam.getCanUse() && !bool(gyro)) {
			// camのみ
			rot = Cam.multiRotGoal(goal.rotOpp);
		}else if(bool(gyro)) {
			// 両方 or gyroのみ
			rot = Gyro.multiRot(0);
		}
	}else {
		rot = Gyro.multiRot(0);
	}
	trace(12) { Serial.println("rot:"+str(rot)); }
	return rot;
}

void checkRole(bool canBecomeGK, comc_t fellow, double ball_r) {
	if(Comc.getCanUse()) {
		if(fellow.exists && isFW == fellow.isFW) {
			if(canBecomeGK && isFW && ball_r > fellow.ball_r) {
				//両方FW && ボール遠い
				isFW = false;
			}else if(!isFW && !canRun) {
				// 停止状態
				isFW = true;
			}else if(!isFW && IS_SKY) {
				// 両方GK
				isFW = true;
			}
		}
		if(canRun && !fellow.exists && isFW && canBecomeGK) {
			// fellowいなくなる
			isFW = false;
		}
	}
}

void avoidMulDef(Angle *dir, comc_t fellow, vectorRT_t ball, Dist distGoal) {
	if(fellow.exists || true) {
		if(ball.t.isDown(90)) {
			switch (distGoal) {
			// 少し後ろ
			case CLOSE:
				*dir = ball.t.isDown(10) ? Angle(false)
						: 80 * signum(ball.t);
				break;
			// 後ろ過ぎ
			case TOO_CLOSE:
				*dir = ball.t.isDown(10) ? 0
						: 50 * signum(ball.t);
				break;
			default:
				break;
			}
		}
	}
}

void detectEnemyBack(Angle *dir, vectorRT_t ball, bool enemyStandsBack) {
	if(enemyStandsBack && dir->isDown(70)) {
		*dir = constrainAngle(ball.t, -90, 90);
	}
}

void detectBallOutside(Angle *dir, line_t line, Angle gyro) {
	if(bool(line.dirInside) && bool(*dir)) {
		Angle absoluteDI = line.dirInside - gyro;
		if(absoluteDI.isRight(45)) {
			if(line.canPause && (*dir - absoluteDI).inside(180 - 10, 180 + 20)) {
				// 停止
				*dir = Angle(false);
			}else if((*dir - absoluteDI).inside(90, 180)) {
				// 後退
				*dir = absoluteDI + 90;
			}
		}else if(absoluteDI.isLeft(45)) {
			if(line.canPause && (*dir - absoluteDI).inside(180 - 20, 180 + 10)) {
				// 停止
				*dir = Angle(false);
			}else if((*dir - absoluteDI).inside(-180, -90)) {
				// 後退
				*dir = absoluteDI - 90;
			}
		}
	}
}

void detectLineForward(Angle *dir, vectorRT_t ball, Dist distBall) {
	bool isBallOutside = ball.t.isUp(bool(cLineForward) ? 25 : 35);
	switch(sideLF) {
		case LEFT: isBallOutside |= ball.t.inside(-60, 0); break;
		case RIGHT: isBallOutside |= ball.t.inside(0, 60); break;
		default: break;
	}

	if(bool(cLineForward)) {
		*dir = false;
		cLineForward.increase(distBall >= PROPER && isBallOutside);
	}else if(cLineForward.compare(0)) {
		*dir = 180;
		cLineForward.increase(isBallOutside);
	}else {
		cLineForward.increase(false);
	}
}