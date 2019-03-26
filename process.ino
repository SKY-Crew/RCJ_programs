void process() {
	//get
	data_t d;
	get(&d);
	//Role受動的変更
	checkRole(!bool(cBecomeFW), d.fellow);
	if(canRun) {
		//走行中
		if(!prvCanRun) {
			//スタート直後
			d.line.isOutside = false;
			LCD.clear(true);
			LCD.write("Running!", 0, 0);
		}
		if(d.line.isOutside) {
			//ライン復帰
			Motor.run(d.line.dirInside, 0, 150);
			if(Line.getIsLineFront()) {
				cLineForward.increase(true);
			}
		}else{
			if(!isFW) {
				//Role能動的変更
				isFW = (d.catchFreely || d.line.isInAir) && d.fellow.exists;
			}
			if(!isFW) {
				//gyro考慮
				d.ball.t = bool(d.ball.t) ? d.ball.t - d.gyro : Angle(false);
			}
			//dir計算
			Angle dir = calDir(isFW, d.ball, d.gyro, d.goal, d.distGoal, d.distBall);
			if(isFW) {
				//マルチ対策
				d.distGoal = avoidMulDef(&dir, d.fellow, d.ball, d.goal) ? CLOSE : PROPER;
				//ライン上停止
				detectBallOutside(&dir, d.line, d.gyro);
				//ライン前後進->停止
				detctLineForward(&dir, d.ball);
			}
			//rot計算
			int16_t rot = calRot(isFW, d.goal, d.gyro, d.catchingBall, d.isBallForward);
			if(!carryingBall) {
				//姿勢その場修正
				correctRot(isFW, d.gyro);
			}
			//駆動
			run(&d, isFW, dir, rot);
		}
	}else {
		//待機
		wait(&d);
		//Role強制変更
		if(digitalRead(P_CHANGE_ROLE) && !prvChangeRole) {
			isFW = !isFW;
		}
		prvChangeRole = digitalRead(P_CHANGE_ROLE);
	}
}