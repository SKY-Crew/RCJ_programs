void get(data_t *d) {
	d->gyro = Gyro.get();
	d->goal = Cam.get();
	Cam.send(double(d->gyro));

	d->enemyStands[0] = frontPSD.getBool(false);
	d->enemyStands[1] = backPSD.getBool(false);
	d->fellow = Comc.communicate(canRun, isFW, d->ball.r);

	const uint16_t THRE_BACK_PSD[2] = {900, 1200};
	d->distGoalPSD = compare(backPSD.getVal(), THRE_BACK_PSD, 3, CLOSE);
	d->distGoal = d->goal.diffOwn >= LARGE || d->goal.distGK == TOO_FAR
			? d->goal.distGK : d->distGoalPSD;

	d->ball = Ball.get();

	const uint16_t THRE_DB[2] = {370, 200};
	d->distBall = compare(d->ball.r, THRE_DB, 3, CLOSE);
	d->isBallForward = d->distBall == CLOSE && d->ball.t.isUp(15) && Ball.getForward() >= 610;
	d->catchingBall = Ball.getCatch() && d->ball.t.isUp(30) && d->distBall == CLOSE;
	d->catchFreely = d->catchingBall && !d->enemyStands[0]
			&& (isFW || d->distGoal == TOO_FAR || !Cam.getCanUse());

	d->line = Line.get(isFW, d->gyro, Gyro.getDiff(), d->goal.isInCorner);
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
	return dir;
}

int16_t calRot(bool isFW, cam_t goal, Angle gyro, bool catchingBall, bool isBallForward) {
	int16_t rot = 0;
	if(isFW) {
		if(Cam.getCanUse() && !bool(gyro)) {
			//camのみ
			rot = Cam.multiRotGoal(goal.rotOpp);
		}else if(bool(gyro)) {
			//両方 or gyroのみ
			rot = Gyro.multiRot(0);
		}
	}else {
		rot = Gyro.multiRot(0);
	}
	return rot;
}


void checkRole(bool canBecomeGK, comc_t fellow) {
	if(Comc.getCanUse()) {
		if(fellow.exists && isFW == fellow.isFW) {
			if(canBecomeGK && isFW) {
				//fellowがGK->FW
				isFW = false;
			}else if(!isFW && !canRun) {
				//停止状態
				isFW = true;
			}else if(!isFW && IS_SKY) {
				//両方GK
				isFW = true;
			}
		}
		if(canRun && !fellow.exists && isFW && canBecomeGK) {
			//fellowいなくなる
			isFW = false;
		}
	}
}

Dist avoidMulDef(Angle *dir, comc_t fellow, vectorRT_t ball, cam_t goal) {
	Dist distGoal = PROPER;
	if(fellow.exists) {
		distGoal = goal.distFW;
		if(ball.t.isDown(90)) {
			switch (goal.distFW) {
			//少し後ろ
			case CLOSE:
				*dir = ball.t.isDown(10) ? Angle(false)
						: 180 + 90 * (ball.t.isLeft(90) ? 1 : -1);
				break;
			//後ろ過ぎ
			case TOO_CLOSE:
				*dir = ball.t.isDown(10) ? 0
						: 180 + 130 * (ball.t.isLeft(90) ? 1 : -1);
				break;
			default:
				break;
			}
		}
	}
	return distGoal;
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
				//停止
				*dir = Angle(false);
			}else if((*dir - absoluteDI).inside(90, 180)) {
				//後退
				*dir = absoluteDI + 90;
			}
		}else if(absoluteDI.isLeft(45)) {
			if(line.canPause && (*dir - absoluteDI).inside(180 - 20, 180 + 10)) {
				//停止
				*dir = Angle(false);
			}else if((*dir - absoluteDI).inside(-180, -90)) {
				//後退
				*dir = absoluteDI - 90;
			}
		}
	}
}

void detectLineForward(Angle *dir, vectorRT_t ball, Dist distBall) {
	if(bool(cLineForward)) {
		*dir = false;
		cLineForward.increase(ball.t.isUp(25) && distBall >= PROPER);
	}else if(cLineForward.compare(0)) {
		*dir = Angle(180);
		cLineForward.increase(ball.t.isUp(35));
	}else {
		cLineForward.increase(false);
	}
}