void get(data_t *d) {
	d->gyro = Gyro.get();
	d->goal = Cam.get(prvIsInAir);
	Cam.send(double(d->gyro));

	d->enemyStands[0] = frontPSD.getBool(false);
	d->enemyStands[1] = backPSD.getBool(false);

	const uint16_t THRE_BACK_PSD[2] = {1200, 1500};
	d->distGoalPSD = compare(backPSD.getVal(), THRE_BACK_PSD, 3, CLOSE);
	if(isFW) {
		const uint16_t THRE_DIST_FW[3] = {22, 37, 60};
		d->distGoal = compare(d->goal.distOwn, THRE_DIST_FW, 4, TOO_CLOSE);
	}else {
		const double THRE_DIST_GK[3] = {1.5, 4, 8};
		Dist distGK = compare(d->goal.distOwn, THRE_DIST_GK, 4, CLOSE);
		d->distGoal = Cam.getCanUse()
				&& (backPSD.getVal() > 4000 || d->goal.diffOwn >= SMALL || distGK == TOO_FAR)
				? distGK : d->distGoalPSD;
	}

	d->ball = Ball.get();

	d->distBall = compare(d->ball.r, THRE_DIST_BALL, 3, CLOSE);
	d->isBallForward = d->distBall == CLOSE && d->ball.t.isUp(15) && Ball.getForward() >= 450;
	d->catchingBall = Ball.getCatch() && d->ball.t.isUp(30) && d->distBall == CLOSE;
	d->catchFreely = d->catchingBall && !d->enemyStands[0]
			&& (isFW || d->distGoal >= FAR || !Cam.getCanUse());

	d->line = Line.get(isFW, d->gyro, Gyro.getDiff(), d->goal.isInCorner != 0);
	prvIsInAir = d->line.isInAir;
	d->line.isOutside |= !bool(d->ball.t) && bool(d->line.dirInside);

	d->fellow = Comc.rcv(isFW);
	Comc.snd(canRun, isFW, d->ball.r, d->goal.distOwn, d->distGoal <= CLOSE, d->line.isInAir);

}


Angle calDir(bool isFW, vectorRT_t ball, double distGoal) {
	Angle dir;
	if(isFW) {
		dir = Ball.getDir(ball);
	}else {
		dir = constrain(map(distGoal, 5, 60, 90, 180), 85, 180);
	}
	trace(11) { Serial.println("dir:"+str(dir)); }
	return dir;
}

int16_t calRot(bool isFW, cam_t goal, Angle gyro, bool catchingBall, bool isBallForward, bool isOnLine) {
	int16_t rot = 0;
	if(isFW) {
		if(Cam.getCanUse() && !bool(gyro)) {
			// camのみ
			rot = Cam.multiRotGoal(goal.rotOpp);
		}else if(bool(gyro)) {
			// 両方 or gyroのみ
			rot = Gyro.multiRot(0, isOnLine);
		}
	}else {
		rot = Gyro.multiRot(0, isOnLine);
	}
	trace(12) { Serial.println("rot:"+str(rot)); }
	return rot;
}

void checkRole(bool canBecomeGK, bool onGround, comc_t fellow, double ball_r, double distOwn) {
	if(Comc.getCanUse()) {
		if(fellow.exists && isFW == fellow.isFW) {
			if(canBecomeGK && onGround && isFW) {
				// 両方FW
				if(fellow.distOwn - distOwn > 5) {
					//ゴール近い
					isFW = false;
				}else if(abs(distOwn - fellow.distOwn) <= 5 && fellow.ball_r > ball_r) {
					//ゴール等距離&ボール遠い
					isFW = false;
				}
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

bool avoidMulDef(Angle *dir, comc_t fellow, vectorRT_t ball, Dist distGoal, cam_t goal) {
	if(digitalRead(57)) { // fellow.exists && Comc.getCanUse()) {
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
			if(goal.diffOwn >= TOO_LARGE && bool(goal.rotOwn)) {
				*dir = false;
				return true;
			}
		}
	}
	return false;
}

bool detectEnemyBack(Angle *dir, vectorRT_t ball, bool enemyStandsBack) {
	if(enemyStandsBack && dir->isDown(70)) {
		*dir = constrainAngle(ball.t, -90, 90);
		return true;
	}
	return false;
}

bool detectBallOutside(Angle *dir, line_t line, Angle gyro) {
	if(bool(line.dirInside) && bool(*dir)) {
		Angle absoluteDI = line.dirInside - gyro;
		if(absoluteDI.isRight(45)) {
			if(line.canPause && (*dir - absoluteDI).inside(180 - 30, 180 + 30)) {
				// 停止
				*dir = Angle(false);
				return true;
			}else if((*dir - absoluteDI).inside(90, 180)) {
				// 後退
				*dir = absoluteDI + 90;
				return true;
			}
		}else if(absoluteDI.isLeft(45)) {
			if(line.canPause && (*dir - absoluteDI).inside(180 - 30, 180 + 30)) {
				// 停止
				*dir = Angle(false);
				return true;
			}else if((*dir - absoluteDI).inside(-180, -90)) {
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