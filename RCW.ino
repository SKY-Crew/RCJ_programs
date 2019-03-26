#include "Include.h"
#include "Declare.h"

void setup() {
	delay(1000);
	Serial.begin(9600);

	pinMode(P_START, INPUT);
	pinMode(P_CHANGE_ROLE, INPUT);
	pinMode(P_IS_FW, OUTPUT);
}

void loop() {
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
		main();
	}
}