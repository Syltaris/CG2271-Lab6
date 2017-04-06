
#include <Arduino.h>
#include "prioq.h"

unsigned int halfDelayInterval_4Hz = 125;
unsigned int halfDelayInterval_2Hz = 250;

int LED0_PIN = 7;
int LED1_PIN = 6;
int ISR0_PIN = 2;
int ISR1_PIN = 3;

unsigned long isr0MillisOld = 0;
unsigned long isr1MillisOld = 0;

TPrioQueue *queue;


// Declares a new type called "funcptr"
typedef void (*funcptr)(void);

// Flashes LED at pin 7: 5 times a 4 Hz
void int0task()
{
	for(int i = 0; i < 5; i++) {
		digitalWrite(LED0_PIN, HIGH);
		delay(halfDelayInterval_4Hz);
		digitalWrite(LED0_PIN, LOW);
		delay(halfDelayInterval_4Hz);
	}
}

// Flashes LED at pin 6: 5 times at 2HZ
void int1task()
{
	for(int i = 0; i < 5; i++) {
		digitalWrite(LED1_PIN, HIGH);
		delay(halfDelayInterval_2Hz);
		digitalWrite(LED1_PIN, LOW);
		delay(halfDelayInterval_2Hz);
	}
}

void int0ISR()
{
	Serial.print("isr0 activated\n");

	unsigned long currMillis = millis();

	if(currMillis - isr0MillisOld > 200) {
		enq(queue , (void *) int0task, 0); //enq isr0 with priority 0

		isr0MillisOld = currMillis;
	}
}

void int1ISR()
{
	Serial.print("isr1 activated\n");

	unsigned long currMillis = millis();

	if(currMillis - isr1MillisOld > 200) {
		enq(queue , (void *) int1task, 1); //enq isr1 with priority 1

		isr1MillisOld = currMillis;
	}
}

void setup()
{
	pinMode(LED0_PIN, OUTPUT);
	pinMode(LED1_PIN, OUTPUT);

	queue = makeQueue(); //initialise empty queue

	attachInterrupt(digitalPinToInterrupt(ISR0_PIN), int0ISR ,RISING);
	attachInterrupt(digitalPinToInterrupt(ISR1_PIN), int1ISR ,RISING);
}

// Dequeues and calls functions if the queue is not empty
void loop()
{
	void* deqVar = deq(queue);

	if(deqVar != NULL) {
		funcptr funcToCall = (funcptr) deqVar;
		funcToCall();
	}
}

