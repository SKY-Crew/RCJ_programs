void correctRot(bool isFW, Angle gyro) {
	cCorrectRot.set_COUNT_UP(!correctingRot);
	correctingRot = bool(cCorrectRot);
	if(correctingRot) {
		//駆動
		int16_t powerCorrectRot = absAngle(gyro) >= BORDER_INCREASE_CCR
			? signum(gyro) * (isFW ? 120 : 60)
			: signum(gyro) * (isFW ? 20 : 20);
		Actuator.run(false, powerCorrectRot, 0);
		//correctRot継続
		cCorrectRot.increase(absAngle(gyro) >= BORDER_DECREASE_CCR);
	}else {
		//correctRot開始
		cCorrectRot.increase(absAngle(gyro) >= BORDER_INCREASE_CCR);
	}
}

void carryBall(bool isFW, line_t line, int16_t rot, cam_t goal, Angle gyro, bool catchingBall, bool enemyStandsFront) {
	willCarryBall = carryingBall;
	if(carryingBall) {
		if(Ball.compareCatch(BORDER_CONTINUE_CARRY) && millis() - timeStartCB < 1500) {
			if(absAngle(gyro) >= 30) {
				//斜め移動
				Actuator.run(enemyStandsFront ? - signum(gyro) * 40 : 0, rot, isFW ? 230 : 200);
			}else if(bool(line.dirInside)) {
				////ライン上斜め移動
				// Actuator.run(signum(line.dirInside) * 20, rot, isFW ? 230 : 200);
				Actuator.run(- signum(line.dirInside) * 10, signum(line.dirInside) * 100, 250);
			}else {
				//敵よけ回転
				Actuator.run(0,
					enemyStandsFront ? signum(goal.rotOpp) * 100 : 0, isFW ? 230 : 200);
			}
		}else {
			//スタック
			Actuator.run(false, 0, 0);
		}
		//carry続けるか
		willCarryBall = Ball.compareCatch(BORDER_CONTINUE_CARRY);
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
		bool leavingLine = bool(d->line.dirInside) && dir.inside(d->line.dirInside + 90, d->line.dirInside - 90);
		carryBall(isFW, d->line, rot, d->goal, d->gyro, d->catchingBall || d->isBallForward, d->enemyStandsFront);
		if(d->goal.isInCorner) {
			Actuator.run(false, rot * 1.5, 0);
		}else if(d->isBallForward) {
			//ボール前方直線上
			Actuator.run(dir, rot * 1.5, leavingLine ? 120 : 210);
		}else if(dir.inside(90, 270)) {
			//ボール後方
			Actuator.run(dir, min(rot * 1.5, 60), (leavingLine || d->isGoalClose) ? 100 : 180);
		}else {
			//ボール前方
			Actuator.run(dir, min(rot * 1.5, 60), leavingLine ? 100 : 140);
		}
		Actuator.kick(d->catchFreely && d->goal.isWide && (Cam.getCanUse() || signum(d->gyro) >= 30 || !bool(d->line.dirInside)));
	}else {
		//GK
		d->isBallForward |= d->ball.t.inside(350, 10) && d->isBallClose;
		bool isTooFarGoal = d->goal.distGK >= 2 && !d->isGoalClosePSD;
		bool isOnSide = abs(d->goal.rot) >= 3 || (isTooFarGoal && abs(d->goal.rot) >= 2);
		if(isOnSide) {
			//横行きすぎ
			Actuator.run(180 - signum(d->goal.rot) * (isTooFarGoal ? 60 : 100), rot, 120);
		}else if(isTooFarGoal) {
			//ゴールとても遠すぎ・仲間がいてゴール遠すぎ
			Actuator.run(180, rot, 160);
		}
		//ボール捕獲
		carryBall(isFW, d->line, rot, d->goal, d->gyro, d->catchingBall, false);
		if(d->isBallForward) {
			//ボール前方
			Actuator.run(0, rot, d->fellow.exists ? 40 : 100);
		}else if(d->isBallClose && abs(d->goal.rot) >= 1 && signum(d->ball.t - 180) == signum(d->goal.rot)) {
			Actuator.run(Ball.getDir(d->ball.t, d->isBallClose), 0, 40);
		}else if(!bool(dir)
			||	(signum(d->ball.t - 180) == signum(d->goal.rot) && abs(d->goal.rot) >= 2)
			||	d->ball.t.inside(150, 210)
			||	d->ball.t.inside(355, 5)) {
			//ボールない・ボール外側・ボール後ろ(150~210)・ボール前方遠く
			Actuator.run(d->isGoalClosePSD ? 0 : d->goal.distGK > 0 ? 180 : Angle(false),
				(!d->isGoalClosePSD && d->goal.distGK <= 0) ? signum(rot) * 60 : 0, 80);
		}else if(d->ball.t.inside(340, 20) || signum(d->goal.rot) != signum(dir)) {
			//ボールある程度前方・少し横
			Actuator.run(dir, rot, 120);
		}else {
			//ボール斜め前方
			Actuator.run(dir, rot, 210);
		}
		Actuator.kick(d->catchFreely && !d->fellow.exists);
	}
	carryingBall = willCarryBall && d->ball.t.inside(340, 20) && d->isBallClose;
}