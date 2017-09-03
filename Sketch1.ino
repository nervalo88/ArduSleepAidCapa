/*
 Name:		Sketch1.ino
 Created:	08/01/2017 02:09:04
 Author:	Reno
*/
#include <CapacitiveSensor.h>
#define CAPACITIVE_STEPS 250
#define CAPACITIVE_SENSITIVITY 100

CapacitiveSensor   cs = CapacitiveSensor(2, 6);

#define TSYST_PERIOD_MS 50
#define TSYST2_PERIOD_MS 10000	// update breathing time graduately

#define CYCLE_TIME_MIN		8   //Default : 8  // time for one sleep cycle
#define INSP_PERCENT		33  // % of cycle time for inspiration
#define MAX_BREATH_FREQ		11	//breath per minute at startup
#define MIN_BREATH_FREQ		6	//breath per minute

#define LED_PIN	3				// blue led analog output PWM pin !!
#define MAX_PWM 200				// Shall crete a halo on the roof but not burn user eyes
#define MIN_PWM 0				

int etpCycle = 0;
long startMillis;

long cycleTimeMs = 60 * CYCLE_TIME_MIN * 1000L;

long startBreathTimeMs = 60000 / MAX_BREATH_FREQ;
long endBreathTimeMs = 60000 / MIN_BREATH_FREQ;

long breathTimeMsIncrementPerPeriod2= TSYST2_PERIOD_MS * (endBreathTimeMs - startBreathTimeMs) / cycleTimeMs;

long  inspTimeMs, inspLoops, expTimeMs, expLoops;
long currentBreathTimeMs = startBreathTimeMs;
long breathTimeIncLoopCounter = 0;
int lightIncCounter = 0;
int lightDecCounter = 0;

int prevLightIntensity = MIN_PWM;
int currentLightIntensity = prevLightIntensity;

long state;
long lightRatio;

void printVal(long val);

#include <TaskScheduler.h>

void tSyst();
void tSyst2();

Task sysTask(TSYST_PERIOD_MS, TASK_FOREVER, tSyst);
Task sysTask2(TSYST2_PERIOD_MS, TASK_FOREVER, tSyst2);

Scheduler runner;

void tSyst() {

	switch (etpCycle)
	{
	case 0:
		Serial.println("Waiting for start...");
		etpCycle = 10;
		break;

	case 10:
		if (cs.capacitiveSensor(CAPACITIVE_STEPS) > CAPACITIVE_SENSITIVITY) {
			etpCycle = 20;
			startMillis = 0;
		}
		break;
	

	case 20:
		// calculates cycle parameters

		if (currentBreathTimeMs < endBreathTimeMs) {
			// Calculate breathing cycle 
			inspTimeMs = INSP_PERCENT*currentBreathTimeMs / 100;
			inspLoops = inspTimeMs / TSYST_PERIOD_MS;
			expTimeMs = currentBreathTimeMs - inspTimeMs;
			expLoops = expTimeMs / TSYST_PERIOD_MS;
						
			etpCycle = 30;
			currentLightIntensity = MIN_PWM;
		}
		else {
			etpCycle = 100;
		}

		break;
	case 30:
		//light increase during inspTimeMs
	
		state = (100 * lightIncCounter) / inspLoops;
		state *= 10;
		lightRatio = getLightPercentRaise(state);
		currentLightIntensity = MIN_PWM + lightRatio*(MAX_PWM - MIN_PWM) / 1000;
		//printVal(state, lightRatio, currentLightIntensity);

		lightIncCounter++;

		
		if (lightIncCounter > inspLoops) {
			etpCycle = 40;
			currentLightIntensity = MAX_PWM;
			lightIncCounter = 0;
		}
		break;
	case 40:
		//light decrease during expTimeMs
		currentLightIntensity = MAX_PWM - lightDecCounter*(MAX_PWM-MIN_PWM)/expLoops;
		state = (100 * lightDecCounter) / expLoops;
		state *= 10;
		lightRatio = getLightPercentDec(state);
		currentLightIntensity = MIN_PWM + lightRatio*(MAX_PWM - MIN_PWM) / 1000;
		//printVal(state,lightRatio,currentLightIntensity);


		lightDecCounter++;
		if (lightDecCounter > expLoops) {
			etpCycle = 20;
			lightDecCounter = 0;
		}
		break;
	case 100:
		Serial.println("FIN DE CYCLE ... BONNE NUIT !");
		currentBreathTimeMs = startBreathTimeMs;
		currentLightIntensity = 0;
		etpCycle = 0;
		break;
	default:
		break;

	}

	//update output  when usefull

	if (currentLightIntensity != prevLightIntensity) {

		analogWrite(LED_PIN, currentLightIntensity);
		prevLightIntensity = currentLightIntensity;
		printVal(currentLightIntensity);
	}


}

void tSyst2() {
	// update breathing time graduately
		currentBreathTimeMs += breathTimeMsIncrementPerPeriod2;
}

void setup() {
	Serial.begin(115200);
	runner.init();
	runner.addTask(sysTask);
	runner.addTask(sysTask2);
	sysTask.enable();
	sysTask2.enable();
}

// the loop function runs over and over again until power down or reset
void loop() {
	runner.execute();
}

void printVal(long val) {
	Serial.print(millis()- startMillis);
	Serial.print(",");
	Serial.println(val);
}

void printVal(long val1, long val2) {
	Serial.print(millis() - startMillis);
	Serial.print(",");
	Serial.print(val1);
	Serial.print(",");
	Serial.println(val2);
}

void printVal(long val1, long val2, long val3) {
	Serial.print(millis() - startMillis);
	Serial.print(",");
	Serial.print(val1);
	Serial.print(",");
	Serial.print(val2);
	Serial.print(",");
	Serial.println(val3);
}

int lightRaise[10][2] = {
	{ 0, 0 },
	{ 157, 240 },
	{ 263, 421 },
	{ 368, 566 },
	{ 473, 686 },
	{ 578, 801 },
	{ 684, 891 },
	{ 789, 963 },
	{ 894, 1000 },
	{ 1000, 1000 }

};

long getLightPercentRaise(long x) {
	long lightPercent;
	x *= 1;
	for (int i = 0; i <= 10; i++) {
		if (x < lightRaise[i][0]) {
			int xa = lightRaise[i-1][0];
			int xb = lightRaise[i][0];
			int ya = lightRaise[i-1][1];
			int yb = lightRaise[i][1];
			lightPercent =ya+(x-xa)*(yb-ya)/(xb-xa);

			if (lightPercent > 1000) lightPercent = 1000;
			break;
		}
	}
	return lightPercent;
}

int lightDec[11][2] = {
	{ 0, 1000 },
	{ 100, 900 },
	{ 200, 687 },
	{ 300, 431 },
	{ 400, 275 },
	{ 500, 181 },
	{ 600, 118 },
	{ 700, 62 },
	{ 800, 25 },
	{ 900, 6 },
	{ 1000, 0 }
};

long getLightPercentDec(long x) {
	long lightPercent;
	x *= 1;
	for (int i = 0; i < 11; i++) {
		if (x < lightDec[i][0]) {
			int xa = lightDec[i - 1][0];
			int xb = lightDec[i][0];
			int ya = lightDec[i - 1][1];
			int yb = lightDec[i][1];

			//printVal(xa, xb);
			//printVal(ya, yb);

			lightPercent = ya + (x - xa)*(yb - ya) / (xb - xa);
			if (lightPercent > 1000) lightPercent = 1000;
			//printVal( x, lightPercent);
			break;
		}
	}
	if (state == 1000) lightPercent = 0;
	return lightPercent;
}