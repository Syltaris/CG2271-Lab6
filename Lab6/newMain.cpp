#include <Arduino.h>
#include <avr/io.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#define INT0_PIN 2
#define INT1_PIN 3
#define POTENTIO_PIN 5
#define PIEZO_PIN 10
#define MOTOR_PIN 11

#define SPEED1_PIN 6
#define SPEED2_PIN 7
#define SPEED3_PIN 8
#define BRAKING_PIN 9

#define SPD_OFFSET 1
#define DESIREDSPD_OFFSET 100
#define DISTANCE_OFFSET 10000

#define STACK_SIZE 100

volatile unsigned int desiredSpeedLevel = 0;
unsigned int speedLevel = 0;

unsigned int distanceReading = 0;

unsigned long incSpdISROldMillis = 0;
unsigned long decSpdISROldMillis = 0;

SemaphoreHandle_t speedSemaphore = NULL;
SemaphoreHandle_t blinkRedSemaphore = NULL;

QueueHandle_t messageQueue = NULL;
QueueHandle_t valueQueue = NULL;

unsigned int message;
char messageToSendPrint[20];

BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

void speedAndDistanceCheck(void*);

void increaseSpeed() {
	if (desiredSpeedLevel < 3) {
		++desiredSpeedLevel;

//		Serial.println("fuuuu");

		xQueueSendToBackFromISR(valueQueue, (void * )desiredSpeedLevel,
				&pxHigherPriorityTaskWoken);
	}
}

void decreaseSpeed() {
	if (desiredSpeedLevel > 0) {
		--desiredSpeedLevel;

//		Serial.println("fa");

		xQueueSendToBackFromISR(valueQueue, (void * )desiredSpeedLevel,
				&pxHigherPriorityTaskWoken);
	}
}

void increaseSpeedISR() {
	unsigned long currMillis = millis();

	if (currMillis > incSpdISROldMillis + 200) {
		increaseSpeed();

		incSpdISROldMillis = currMillis;
	}
}

void decreaseSpeedISR() {
	unsigned long currMillis = millis();

	if (currMillis > decSpdISROldMillis + 200) {
		decreaseSpeed();

		decSpdISROldMillis = currMillis;
	}
}

//blinks the required LED based on speedLevel
void setLEDs() {
	//off the required LEDs
	switch (speedLevel) {
	case 0:
		digitalWrite(SPEED1_PIN, LOW);
	case 1:
		digitalWrite(SPEED2_PIN, LOW);
	case 2:
		digitalWrite(SPEED3_PIN, LOW);
		break;
	}
	//on the required LEDs
	switch (speedLevel) {
	case 3:
		digitalWrite(SPEED3_PIN, HIGH);
	case 2:
		digitalWrite(SPEED2_PIN, HIGH);
	case 1:
		digitalWrite(SPEED1_PIN, HIGH);
		break;
	}
}

//turn on red for 1s
void blinkRED(void*p) {
	while (1) {
		if(xSemaphoreTake(blinkRedSemaphore, portMAX_DELAY) == pdTRUE) {
			TickType_t currTick = xTaskGetTickCount();
			digitalWrite(BRAKING_PIN, HIGH);
			vTaskDelayUntil(&currTick, 1000);
			digitalWrite(BRAKING_PIN, LOW);
		}
	}
}

//drive the piezo and motor
void motorDriver(void* p) {
	while(1) {
		analogWrite(PIEZO_PIN, 25 * speedLevel);
		vTaskDelay(10 * (3-speedLevel));
		analogWrite(PIEZO_PIN, 0);
		vTaskDelay(10 * (3-speedLevel));

		analogWrite(MOTOR_PIN, 85 * speedLevel);
	}
}

//checks if desiredSpeed > distance, if so, ensure curr speed limit  (???? s)
void speedAndDistanceCheck(void* p) {
	while (1) {
		int dummyInt;

		//wait for signal to check the values
		if (xQueueReceive(valueQueue, &dummyInt, portMAX_DELAY) == pdTRUE) {
			//for marks
			if (xSemaphoreTake(speedSemaphore, portMAX_DELAY) == pdTRUE) {
				int oldSpeedLevel = speedLevel;

				//check if safe
				speedLevel = (desiredSpeedLevel < distanceReading) ? desiredSpeedLevel : distanceReading;

//				if (potFlag && (oldSpeed > speedLevel)){
//					blinkRed();
//				}

				//if auto braking occurred
				if((desiredSpeedLevel > distanceReading) && (oldSpeedLevel > distanceReading)) {
//					Serial.println("it ded");
					xSemaphoreGive(blinkRedSemaphore);
				}

				setLEDs();
				xSemaphoreGive(speedSemaphore);
			}
		}
	}
}

//reads value from potentiometer and maps it every 0.5s
void readDistance(void* p) {
	while (1) {
		TickType_t currTicks = xTaskGetTickCount();

		distanceReading = analogRead(POTENTIO_PIN) / 256; //to accommodate 4 distance levels

//		char stringToPrint[25];
//		sprintf(stringToPrint, "%u", distanceReading);
//		Serial.println(stringToPrint);

		xQueueSendToBack(valueQueue, (void*) distanceReading, portMAX_DELAY);

		vTaskDelayUntil(&currTicks, 500);
	}
}



//transmit message through UART every 1s
void sendUARTMessage(void* p) {
	while (1) {
		TickType_t currTicks = xTaskGetTickCount();

		if (xSemaphoreTake(speedSemaphore, 0xFFFF) == pdTRUE) {
			message = speedLevel * SPD_OFFSET +
					desiredSpeedLevel * DESIREDSPD_OFFSET +
					distanceReading * DISTANCE_OFFSET;
		}

		//SEND MESSAGE TO QUEUE
		xQueueSendToBack(messageQueue, &message, portMAX_DELAY);

		vTaskDelayUntil(&currTicks, 1000);
	}
}

//prints the message every 1s
void readUARTMessage(void* p) {
	while (1) {
		TickType_t currTicks = xTaskGetTickCount();

		if ((xQueueReceive(messageQueue, &message, portMAX_DELAY)) == pdTRUE) {
			sprintf(messageToSendPrint,"SPD: %u\nDES: %u\nDIS: %u\n",
					message/SPD_OFFSET, message/DESIREDSPD_OFFSET, message/ DISTANCE_OFFSET);
		}

		vTaskDelayUntil(&currTicks, 1000);
	}
}

//void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {
//	Serial.println(*pcTaskName);
//}

void setup() {
	pinMode(SPEED1_PIN, OUTPUT);
	pinMode(SPEED2_PIN, OUTPUT);
	pinMode(SPEED3_PIN, OUTPUT);
	pinMode(BRAKING_PIN, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(INT1_PIN), increaseSpeedISR, RISING);
	attachInterrupt(digitalPinToInterrupt(INT0_PIN), decreaseSpeedISR, RISING);

	messageQueue = xQueueCreate(3, sizeof(int));
	valueQueue = xQueueCreate(3, sizeof(int));

	speedSemaphore = xSemaphoreCreateBinary();
	blinkRedSemaphore = xSemaphoreCreateBinary();

	xSemaphoreGive(speedSemaphore);

	Serial.begin(115200);
	Serial.println("BENING");
}

void loop() {
	xTaskCreate(readDistance,
			"sample_potentiometer",
			STACK_SIZE,
			NULL,
			6,
			NULL);

	xTaskCreate(speedAndDistanceCheck,
			"speed_checking and auto_braking",
			STACK_SIZE,
			NULL,
			5,
			NULL);

	xTaskCreate(motorDriver,
			"motor_controller",
			100,
			NULL,
			4,
			NULL);

	xTaskCreate(blinkRED,
			"LED_Blinker",
			100,
			NULL,
			3,
			NULL);

	xTaskCreate(sendUARTMessage,
			"message_transmit",
			STACK_SIZE,
			NULL,
			2,
			NULL);

	xTaskCreate(readUARTMessage,
			"message_receive",
			STACK_SIZE,
			NULL,
			1,
			NULL);

	vTaskStartScheduler();
}
