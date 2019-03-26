void get(data_t *d) {
	d->gyro = Gyro.get();
	d->goal = Cam.get();
	const uint16_t THRE_BACK_PSD[2] = {900, 1200};
	d->distGoalPSD = compare(backPSD.getValue(), THRE_BACK_PSD, 3, true, CLOSE);
	d->distGoal = abs(d->goal.rot) >= 2 || d->goal.distGK == TOO_FAR ? d->goal.distGK : d->distGoalPSD;

	d->enemyStandsFront = frontPSD.getBool();
	d->fellow = Comc.communicate(canRun, isFW);

	d->ball = Ball.get();

	const uint16_t THRE_DB[2] = {370, 200};
	d->distBall = compare(d->ball.r, THRE_DB, 3, false, CLOSE);
	d->isBallForward = d->distBall == CLOSE
		&& Ball.getForward() >= (isFW ? 610 : d->fellow.exists ? 670 : 610);
	d->catchingBall = Ball.getCatch() && d->ball.t.isUp(30) && d->distBall == CLOSE;
	
	cCatchFreely.set_MAX(isFW ? 3 : 1);
	cCatchFreely.increase(d->catchingBall && !d->enemyStandsFront);
	d->catchFreely = bool(cCatchFreely) && (isFW || d->distGoal == TOO_FAR || !Cam.getCanUse());

	d->line = Line.get(isFW, d->gyro, Gyro.getDiff());
}


Angle calDir(bool isFW, vectorRT_t ball, Angle gyro, cam_t goal, bool distGoal, Dist distBall) {
	Angle dir;
	if(isFW) {
		dir = Ball.getDir(ball);
	}else {
		int16_t DIR_GKs[] = {110, 70, 90};
		Angle dirGK = DIR_GKs[min(distGoal, 2)];
		dir = bool(ball.t) ? dirGK * signum(ball.t) : Angle(false);
	}
	return dir;
}

int16_t calRot(bool isFW, cam_t goal, Angle gyro, bool catchingBall, bool isBallForward) {
	//rot計算
	int16_t rot = 0;
	if(isFW) {
		if(Cam.getCanUse() && bool(gyro)) {
			//両方使用可
			rot = (catchingBall || isBallForward) && abs(goal.rotOpp) <= 3
				? Cam.multiRotGoal(goal.rotOpp)
				: Gyro.multiRot(0);
		}else if(Cam.getCanUse()) {
			//camのみ
			rot = Cam.multiRotGoal(goal.rotOpp);
		}else if(bool(gyro)) {
			//gyroのみ
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
				//fellowがGK→FW
				isFW = false;
				cCatchFreely.reset();
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
			cCatchFreely.reset();
		}
	}
}

bool avoidMulDef(Angle *dir, comc_t fellow, vectorRT_t ball, cam_t goal) {
	bool isGoalClose = false;
	if(fellow.exists) {
		if(ball.t.isDown(90)) {
			switch (goal.distFW) {
			//少し後ろ
			case CLOSE:
				*dir = ball.t.isDown(10) ? Angle(false)
					: ball.t.isRight(90) ? 90 : 270;
				isGoalClose = true;
				break;
			//後ろ過ぎ
			case TOO_CLOSE:
				*dir = ball.t.isDown(10) ? 0
					: ball.t.isRight(90) ? 50 : 310;
				isGoalClose = false;
				break;
			default:
				break;
			}
		}
	}
	return isGoalClose;
}

void detectBallOutside(Angle *dir, line_t line, Angle gyro) {
	if(bool(line.dirInside)) {
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
			}else if((*dir - absoluteDI).inside(180, 270)) {
				//後退
				*dir = absoluteDI - 90;
			}
		}
	}
}