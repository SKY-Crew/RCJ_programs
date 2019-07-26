#include "Include.h"
#include "Declare.h"

/*
Debug
1 Motor::moveAngle
2 Ball::val

4 Line::val
5 Gyro::crt
6 PSD::val
7 Comc::fellow
8 Cam::rotOpp, rotOwn, distOwn
9 Cam::isOppWide, posOwn, isInCorner
11 dir
12 rot
30 Gyro:debug
31 FPS
*/

void setup() {
	delay(1000);
	Serial.begin(9600);

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);

	Buzzer.set(60, 100, true, 1);
	Buzzer.beep();
}

void loop() {
	// debug
	trace_cmdloop(0);
	uint64_t timeLoop = micros();
	// 駆動重複リセット
	Motor.setHaveRun(false);
	Motor.setRateVolt(INA219.getRate());
	if(isSuperField) { Motor.setRatePower(isFW ? 0.9 : 1.0);
	}else if(techCha == 2) { Motor.setRatePower(1.0);
	}else { Motor.setRatePower(isFW ? 1.0 : 1.0); }
	// }else { Motor.setRatePower(isFW ? 0.6 : 0.6); }
	Kicker.setHaveChecked(false);
	Kicker.setPower();
	Buzzer.reset();
	// 走行可か
	prvCanRun = canRun;
	if(techCha != 3) {
		canRun = digitalRead(P_START);
	}else {
		switch(Comc.getCommand()) {
			case 1: canRun = false; break;
			case 2: canRun = true; break;
			case 3: Motor.run(90, Gyro.calRot(0), 150); break;
			default: break;
		}
	}
	canStartRunning &= canRun;
	// Role変更禁止
	cBecomeFW.increase(isFW && (!prvIsFW || (canRun && !prvCanRun)));
	// Role表示LED
	prvIsFW = isFW;
	digitalWrite(P_IS_FW, isFW);
	if(INA219.checkVolt() && !Kicker.getIsKicking()) {
		// 電池残量少
		canRun = false;
		stop();
	}else {
		process();
	}

	//音
	Buzzer.beep();
	// fps計算
	const int64_t WAIT = 6000;
	delayMicroseconds(max(0l, WAIT + (int64_t)timeLoop - (int64_t)micros()));
	timeLoop = micros() - timeLoop;
	trace(31) { Serial.println("FPS:"+str(1000 * 1000 / timeLoop)); }
}