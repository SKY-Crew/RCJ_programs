uint16_t maxIR[16];
uint16_t minIR[16];
double avgIR[16];

void setIR() {
	while(digitalRead(P_START)) {
		LCD.run4IR(Ball.getQTY(), maxIR, minIR, avgIR);
		if(Ina219.checkVolt()) {
			//電池残量少
			stop();
		}
		delay(1);
	}
	while(!digitalRead(P_START)) {
		LCD.run4IR(Ball.getQTY(), maxIR, minIR, avgIR);
		if(Ina219.checkVolt()) {
			//電池残量少
			stop();
		}
		delay(1);
	}
	for(int i = 0; i < Ball.getQTY(); i ++) {
		maxIR[i] = 0;
		minIR[i] = 10000;
		avgIR[i] = 0;
	}

	LCD.clear(false);
	LCD.write("Running!", 0, 0);
	Actuator.setHaveRun(false);
	Actuator.run(-1, 80, 0);
	delay(1000);
	for(int count = 0; count < 1000; count ++) {
		if(Ina219.checkVolt()) {
			//電池残量少
			stop();
		}else {
			Actuator.run(-1, 80, 0);
			Actuator.setHaveRun(false);
			Ball.get(false);
			uint16_t *value = Ball.getValue();
			for(int i = 0; i < 16; i ++) {
				maxIR[i] = max(value[i], maxIR[i]);
				if(value[i] > 0) {
					minIR[i] = min(value[i], minIR[i]);
				}
				avgIR[i] += value[i] / 1000.0;
			}
		}
	}

	Actuator.run(-1, 0, 0);
}