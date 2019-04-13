#include "Include.h"
#include "Declare.h"

/*
Debug
1 Motor::moveAngle
2 Ball::val
3 Ball::valInAir
4 Line::val
5 Gyro::crt
6 PSD::val
31 FPS
*/

void setup() {
	delay(1000);
	Serial.begin(9600);

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);
}

void loop() {
	//debug
	trace_cmdloop(0);
	uint64_t timeLoop = micros();
	//駆動重複リセット
	Motor.setHaveRun(false);
	Kicker.setHaveChecked(false);
	//走行可か
	prvCanRun = canRun;
	canRun = digitalRead(P_START);
	//Role強制変更
	cBecomeFW.increase(isFW && (!prvIsFW || (canRun && !prvCanRun)));
	//Role表示LED
	prvIsFW = isFW;
	digitalWrite(P_IS_FW, isFW);
	if(INA219.checkVolt() && !Kicker.getIsKicking()) {
		//電池残量少
		canRun = false;
		stop();
	}else {
		process();
	}
	//fps計算
	delayMicroseconds(max(0l, WAIT + (int64_t)timeLoop - (int64_t)micros()));
	timeLoop = micros() - timeLoop;
	trace(31) { Serial.println("FPS:"+str(1000 * 1000 / timeLoop)); }
}