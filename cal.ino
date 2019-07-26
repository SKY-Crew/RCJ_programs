void get(data_t *d) {
	d->gyro = Gyro.get();

	d->line = Line.get(d->gyro, bool(d->gyro) ? Gyro.getDiff() : 0);

	d->goal = Cam.get(d->line.isInAir);
	Cam.send(double(d->gyro), isFW);

	d->enemyStands[0] = frontPSD.getBool(false);
	d->enemyStands[1] = backPSD[0].getBool(false) & backPSD[1].getBool(false);
	Buzzer.set(40, 50, d->enemyStands[0], 30);
	Buzzer.set(88, 50, d->enemyStands[1], 150);

	const uint16_t THRE_BACK_PSD[3] = {450, 410, 340};
	d->valBackPSD =
			d->goal.sideOwn == LEFT ? backPSD[0].getVal()
			: d->goal.sideOwn == RIGHT ? backPSD[1].getVal()
			: max(backPSD[0].getVal(), backPSD[1].getVal());
	d->distGoalPSD = compare(d->valBackPSD, THRE_BACK_PSD, 4, CLOSE);
	if(isFW) {
		const uint16_t THRE_DIST_FW[3] = {20, 40, 57};
		d->distGoal = compare(d->goal.distOwn, THRE_DIST_FW, 4, TOO_CLOSE);
	}else {
		const double THRE_DIST_GK[2] = {-1, 20};
		Dist distGK = compare(d->goal.distOwn, THRE_DIST_GK, 3, PROPER);
		d->distGoal = Cam.getCanUse() && distGK == TOO_FAR ? TOO_FAR
				: d->goal.diffOwn >= LARGE ? PROPER : d->distGoalPSD;
	}

	d->line = Line.modify(isFW, d->gyro, d->goal.isInCorner != CENTER, isFW && d->distGoal <= CLOSE);

	d->ball = Ball.get();
	if(bool(d->line.dirInside) && bool(d->ball.t) && (d->ball.t - d->line.dirInside).isDown(70)) {
		d->ball.r = mean(THRE_DIST_BALL, 2);
	}

	d->distBall = compare(d->ball.r, THRE_DIST_BALL, 3, CLOSE);
	d->isBallForward = d->distBall == CLOSE && d->ball.t.isUp(15) && Ball.getForward() >= (isFW ? 450 : 550);
	d->catchingBall = Ball.getCatch() && d->ball.t.isUp(30) && d->distBall == CLOSE;
	d->catchFreely = Ball.getMayKick() && d->catchingBall && !d->enemyStands[0]
			&& (isFW || d->distGoal >= FAR || !Cam.getCanUse());
	Buzzer.set(50, 50, d->catchFreely, 20);

	d->line.isOutside |= !bool(d->ball.t) && bool(d->line.dirInside);
	Buzzer.set(80, 50, d->line.isOutside, 40);

	d->fellow = Comc.rcv(isFW);
	bool allowChangeRole = calAllowChangeRole(isFW, d->ball, d->distGoal, d->goal.distOwn, d->fellow,
			d->fellow.exists && !d->line.isInAir && !d->fellow.isInAir,
			d->line.isInAir && d->fellow.isFW);
	Comc.snd(canRun, isFW, d->ball.r, d->goal.distOwn, allowChangeRole, d->line.isInAir);
}


Angle calRot(int16_t *rot, bool isFW, cam_t goal, Dist distGoal, Angle gyro, vectorRT_t ball,
		Dist distBall, bool catchingBall, bool isBallForward, bool isOnLine) {
	Angle targetDir = 0;
	if(isFW) {
		if(Cam.getCanUse() && bool(goal.rotOpp) &&
				(!bool(gyro) || (distGoal >= FAR && (ball.t - gyro).isUp(90)))) {
			// camのみ
			*rot = Cam.calRot(goal.rotOpp);
			targetDir = goal.rotOpp;
		}else if(bool(gyro)) {
			// 両方 or gyroのみ
			*rot = Gyro.calRot(0);
			targetDir = gyro;
		}
	}else {
		*rot = Gyro.calRot(0);
		targetDir = gyro;
	}
	if(isOnLine) { targetDir = gyro; }
	trace(12) { Serial.println("rot:"+str(*rot)); }
	return targetDir;
}

void calDir(Angle *dir, bool isFW, vectorRT_t ball, Angle gyro, Angle targetDir, double distGoal) {
	if(isFW) {
		ball.t = bool(ball.t) ? ball.t - targetDir : Angle(false);
		*dir = Ball.getDir(ball);
		if(bool(*dir)) { *dir += targetDir; }
	}else {
		*dir = targetDir + signum(ball.t) * conMap(distGoal, 470, 170, 80, 180, 80, 180);
		*dir -= signum(*dir) * conMap(double(abs(gyro)), 0, 30, 0, 40);
	}
	trace(11) { Serial.println("dir:"+str(*dir)); }
}

bool calAllowChangeRole(bool isFW, vectorRT_t ball, Dist distGoal, double distOwn, comc_t fellow, bool onGround, bool multi) {
	bool allowChangeRole = onGround;
	if(isFW && !fellow.isFW) {
		allowChangeRole &= distGoal <= CLOSE && ball.t.isUp(120) && fellow.ball_r - ball.r > 20 && !bool(cBecomeFW);
	}else if(isFW && fellow.isFW && !bool(cBecomeFW)) {
		allowChangeRole &= distOwn - fellow.distOwn > 15
				|| (abs(distOwn - fellow.distOwn) <= 15 && ball.r > fellow.ball_r);
	}
	allowChangeRole |= multi;
	return allowChangeRole;
}

void checkRole(bool canBecomeGK, comc_t fellow) {
	if(Comc.getCanUse()) {
		if(fellow.exists && isFW == fellow.isFW) {
			if(isFW && fellow.allowChangeRole && canBecomeGK) {
				Buzzer.set(75, 200, isFW, 50);
				isFW = false;
			}else if(!isFW && !canRun) {
				// 停止状態
				Buzzer.set(75, 200, isFW, 50);
				isFW = true;
			}else if(!isFW && IS_SKY) {
				// 両方GK
				Buzzer.set(75, 200, isFW, 50);
				isFW = true;
			}
		}
		if(canRun && !fellow.exists && isFW && canBecomeGK) {
			// fellowいなくなる
			Buzzer.set(75, 200, isFW, 50);
			isFW = false;
		}
	}
}

bool avoidMulDef(Angle *dir, comc_t fellow, vectorRT_t ball, Dist distGoal) {
	if(fellow.exists && Comc.getCanUse()) {
		if(ball.t.isDown(100)) {
			switch (distGoal) {
			// 少し後ろ
			case CLOSE:
				*dir = ball.t.isDown(15) ? Angle(false)
						: 90 * signum(ball.t);
				return true;
				break;
			// 後ろ過ぎ
			case TOO_CLOSE:
				*dir = ball.t.isDown(15) || !bool(ball.t) ? 0
						: 70 * signum(ball.t);
				return true;
				break;
			default:
				break;
			}
		}
	}
	return false;
}

bool detectEnemyBack(Angle *dir, vectorRT_t ball, Dist distBall, bool enemyStandsBack) {
	if(enemyStandsBack && dir->isDown(70)) {
		*dir = conAngle(distBall <= PROPER ? *dir : ball.t, -90, 90);
		return true;
	}
	return false;
}

bool detectBallOutside(Angle *dir, vectorRT_t ball, line_t line, Angle gyro) {
	if(bool(line.dirInside) && bool(*dir)) {
		Angle absoluteDI = line.dirInside - gyro;
		Angle diff = *dir - line.dirInside;
		if(line.isBack) {
			if((ball.t - line.dirInside).isDown(45)) {
				*dir = Angle(false);
				return true;
			}
		}
		if(absoluteDI.isRight(45)) {
			if(line.canPause && diff.inside(180 - 45, 180 + 45)) {
				// 停止
				*dir = Angle(false);
				return true;
			}else if(diff.inside(90, 180)) {
				// 後退
				*dir = absoluteDI + 90;
				return true;
			}
		}else if(absoluteDI.isLeft(45)) {
			if(line.canPause && diff.inside(180 - 45, 180 + 45)) {
				// 停止
				*dir = Angle(false);
				return true;
			}else if(diff.inside(-180, -90)) {
				// 後退
				*dir = absoluteDI - 90;
				return true;
			}
		}
	}
	return false;
}

bool detectLineForward(Angle *dir, vectorRT_t ball, Dist distBall, Angle gyro) {
	Angle absoluteBall_t = ball.t - gyro;
	bool isBallOutside = absoluteBall_t.isUp(bool(cLineForward) ? 25 : 35);
	switch(sideLF) {
		case LEFT: isBallOutside |= absoluteBall_t.inside(-60, 0); break;
		case RIGHT: isBallOutside |= absoluteBall_t.inside(0, 60); break;
		default: break;
	}

	if(bool(cLineForward)) {
		*dir = false;
		cLineForward.increase(distBall >= PROPER && isBallOutside);
		return true;
	}else if(cLineForward.compare(0)) {
		*dir = 180 + gyro;
		cLineForward.increase(isBallOutside);
		return true;
	}else {
		cLineForward.increase(false);
	}
	return false;
}

bool detectLineBackward(Angle *dir, vectorRT_t ball, Angle gyro) {
	Angle absoluteBall_t = ball.t - gyro;
	bool isBallOutside = absoluteBall_t.isDown(135);
	if(bool(cLineBackward)) {
		*dir = absoluteBall_t.isDown(30) ? Angle(false) : conAngle(ball.t, -90, 90);
		cLineBackward.increase(isBallOutside);
		return true;
	}else if(cLineBackward.compare(0)) {
		*dir = 0 + gyro;
		cLineBackward.increase(isBallOutside);
		return true;
	}else {
		cLineBackward.increase(false);
	}
	return false;
}