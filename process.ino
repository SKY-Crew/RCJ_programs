void process() {
	// get
	data_t d;
	get(&d);
	checkRole(!bool(cBecomeFW), d.fellow, d.ball.r);
	// Role受動的変更
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
		}else if(d.line.isInAir){
			// 空中
			Motor.run(false, 0, 0);
			isFW = true;
			cBecomeFW.reset();
		}else {
			if(!isFW) {
				isFW = d.catchFreely && d.fellow.exists;
				// Role能動的変更
			}
			if(!isFW) {
				// gyro考慮
				d.ball.t = bool(d.ball.t) ? d.ball.t - d.gyro : Angle(false);
			}
			// dir計算
			Angle dir = calDir(isFW, d.ball, d.goal.distOwn);
			if(isFW) {
				// マルチ対策
				avoidMulDef(&dir, d.fellow, d.ball, d.distGoal);
				// 真後ろ敵
				detectEnemyBack(&dir, d.ball, d.enemyStands[1]);
				// ライン上停止
				detectBallOutside(&dir, d.line, d.gyro);
				// ライン前方向:後進->停止
				detectLineForward(&dir, d.ball, d.distBall);
			}
			// rot計算
			int16_t rot = calRot(isFW, d.goal, d.gyro, d.catchingBall, d.isBallForward);
			if(!carryingBall) {
				// 姿勢その場修正
				correctRot(isFW, d.gyro);
			}
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