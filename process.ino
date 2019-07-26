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
			if(techCha == 6) {
				for(int i = 0; i < 7; i ++) { delay(1000); }
			}
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
						|| (d.goal.sideOwn * signum(d.ball.t) == 1 && d.distBall <= PROPER)
						|| bool(d.line.dirInside);
				Buzzer.set(75, 200, isFW, 50);
			}
			if(!isFW) {
				// gyro考慮
				d.ball.t = bool(d.ball.t) ? d.ball.t - d.gyro : Angle(false);
			}
			// rot計算
			Angle dir;
			int16_t rot;
			Angle targetDir = calRot(&rot, isFW, d.goal, d.distGoal, d.gyro, d.ball,
					d.distBall, d.catchingBall, d.isBallForward, bool(d.line.dirInside));
			calDir(&dir, isFW, d.ball, d.gyro, targetDir, d.valBackPSD);
			// dir計算
			if(isFW && (techCha == 0 || techCha == 2 || techCha == 3 || techCha == 5)) {
				if(avoidMulDef(&dir, d.fellow, d.ball, d.distGoal)) {
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
			if(techCha == 1) {
				if(detectEnemyBack(&dir, d.ball, d.distBall, d.enemyStands[1])) {
					// 真後ろ敵
				}
			}
			if(!carryingBall) {
				// 姿勢その場修正
				correctRot(isFW, d.gyro);
			}
			// 駆動
			switch(techCha) {
				case 1:
					cCarryBall4TC6.increase(d.goal.isOppWide);
					if(d.catchFreely && cCarryBall4TC6.compare(0)) {
						Motor.run(0, rot, 140);
					}
					Motor.run(dir, rot, conMap(double(abs(d.ball.t)), 10, 45, 40, 100));
					Kicker.run(d.catchFreely&& bool(cCarryBall4TC6));
					break;
				case 6:
					cCarryBall4TC6.increase(d.goal.isOppWide);
					if(d.catchFreely && cCarryBall4TC6.compare(0)) {
						Motor.run(0, rot, 140);
					}
					if(d.distBall > CLOSE) {
						Motor.run(dir, rot, conMap(double(abs(d.ball.t)), 10, 45, 70, 120));
					}else {
						Motor.run(dir, rot, conMap(double(abs(d.ball.t)), 10, 45, 30, 70));
					}
					Kicker.run(d.catchFreely && bool(cCarryBall4TC6));
					break;
				case 4:
					if(isFW) {
						if(canStartRunning) {
							run(&d, isFW, dir, rot);
						}
						Cam.snd(canStartRunning ? 255 : 128);
						canStartRunning |= Comc.rcv4TC4() == 255;
					}else {
						if(d.distBall >= FAR) {
							Motor.run(false, 0, 0);
						}else {
							Motor.run(dir, rot, conMap(double(abs(d.ball.t)), 10, 45, 40, 100));
						}
						Cam.snd(0);
						Kicker.run(d.catchFreely && d.goal.rotOpp.isUp(5));
						if(Kicker.getIsKicking()) { Comc.snd4TC4(255); }
					}
					break;
				default:
					run(&d, isFW, dir, rot);
					break;
			}
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