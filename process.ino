void process() {
	// get
	data_t d;
	get(&d);
	// Role受動的変更
	checkRole(!bool(cBecomeFW), d.fellow);
	if(canRun) {
		// 走行中
		if(!prvCanRun) {
			// スタート直後
			d.line.isOutside = false;
			LCD.clear(true);
			LCD.write("Running!", 0, 0);
		}
		if(d.line.isOutside) {
			// ライン復帰
			Motor.run(d.line.dirInside, 0, 150);
			// ライン前方向
			cLineForward.reset();
			cLineForward.increase(d.line.isFront);
			sideLF = d.line.isFront ? d.goal.isInCorner : sideLF;
			if(!isFW && Cam.getCanUse() && (d.line.dirInside.isLeft(30) || d.line.dirInside.isRight(30))) {
				d.goal.diffOwn = TOO_LARGE;
				d.goal.sideOwn = Side(- signum(d.line.dirInside));
			}

			cLineBackward.reset();
			cLineBackward.increase(d.line.isBack);
		}else if(d.line.isInAir){
			// 空中
			Motor.run(false, 0, 0);
			cLineForward.reset();
			cLineBackward.reset();
			if(Comc.getCanUse()) {
				isFW = true;
				prvIsFW = isFW;
				cBecomeFW.reset();
			}
		}else {
			if(!isFW && d.fellow.isFW && d.fellow.allowChangeRole) {
				// Role能動的変更
				isFW = d.isBallForward
						|| (d.goal.sideOwn * signum(d.ball.t) == 1 && d.distBall <= PROPER);
			}
			if(!isFW) {
				// gyro考慮
				d.ball.t = bool(d.ball.t) ? d.ball.t - d.gyro : Angle(false);
			}
			// dir計算
			Angle dir = calDir(isFW, d.ball, d.gyro, d.goal.distOwn);
			if(isFW) {
				if(avoidMulDef(&dir, d.fellow, d.ball, d.distGoal, d.goal)) {
					// マルチ対策
				}else if(detectLineBackward(&dir, d.ball, d.gyro)) {
					// ライン後方向:前進->停止
				}
				if(detectBallOutside(&dir, d.ball, d.line, d.gyro)) {
					// ライン上停止
				}else if(detectLineForward(&dir, d.ball, d.distBall, d.gyro)) {
					// ライン前方向:後進->停止
				}else if(detectEnemyBack(&dir, d.ball, d.distBall, d.enemyStands[1])) {
					// 真後ろ敵
				}
			}
			// rot計算
			int16_t rot = calRot(isFW, d.goal, d.gyro, d.catchingBall, d.isBallForward, bool(d.line.dirInside));
			if(!carryingBall) {
			/* if(!carryingBall) {
				// 姿勢その場修正
				correctRot(isFW, d.gyro);
			}*/
			// 駆動
			run(&d, isFW, dir, rot);
		}
	}else {
		// 待機
		wait(&d);
		// Role強制変更
		if(digitalRead(P_CHANGE_ROLE) && !prvChangeRole) {
			isFW = !isFW;
		}
		prvChangeRole = digitalRead(P_CHANGE_ROLE);
	}
}