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
#define DESIREDSPD_OFFSET 10
#define DISTANCE_OFFSET 100

#define STACK_SIZE 100

typedef void (*funcptr)(void);
funcptr funcToExecute;



volatile uint8_t desiredSpeedLevel = 0;
uint8_t speedLevel = 0;
uint8_t distanceReading = 0;

volatile unsigned long incSpdISROldMillis = 0;
volatile unsigned long decSpdISROldMillis = 0;

SemaphoreHandle_t speedSemaphore = NULL;

QueueHandle_t messageQueue = NULL;
QueueHandle_t functionQueue = NULL;

BaseType_t pxHigherPriorityTaskWoken = pdFALSE;


//blinks the required LED based on speedLevel
void setLEDs() {
	digitalWrite(SPEED1_PIN, speedLevel >= 1 ? HIGH : LOW);
	digitalWrite(SPEED2_PIN, speedLevel >= 2 ? HIGH : LOW);
	digitalWrite(SPEED3_PIN, speedLevel >= 3 ? HIGH : LOW);
}

//blink red led
void blinkRED() {
	TickType_t currTick = 0;

	currTick = xTaskGetTickCount();
	digitalWrite(BRAKING_PIN, HIGH);
	vTaskDelayUntil(&currTick, 1000);
	digitalWrite(BRAKING_PIN, LOW);
}

//drive the piezo and motor
void motorDriver() {
	analogWrite(PIEZO_PIN, 25 * speedLevel);
//	vTaskDelay(10 * (3-speedLevel));
//	analogWrite(PIEZO_PIN, 0);
//	vTaskDelay(10 * (3-speedLevel));
}

//checks if desiredSpeed > distance, if so, ensure curr speed limit  (???? s)
void speedAndDistanceCheck() {
	TickType_t currTick = 0;
	uint8_t oldSpeedLevel = speedLevel;

	//check if safe
	speedLevel = (desiredSpeedLevel < distanceReading) ?
					desiredSpeedLevel : distanceReading;

	funcToExecute = setLEDs;
	//set corresponding yellow LEDs
	xQueueSendToBack(functionQueue, &funcToExecute, portMAX_DELAY);

	//set motor later
	funcToExecute = motorDriver;
	xQueueSendToBack(functionQueue, &funcToExecute, portMAX_DELAY);

	//if auto braking occurred
	if ((desiredSpeedLevel > distanceReading) && (oldSpeedLevel > distanceReading)) {
		funcToExecute = blinkRED;

		xQueueSendToBack(functionQueue, &funcToExecute, portMAX_DELAY);
	}
}


//reads value from potentiometer and maps it every 0.5s
void readDistance(void* p) {
	TickType_t currTicks = 0;

	while (1) {
		distanceReading = analogRead(POTENTIO_PIN) / 256; //to accommodate 4 distance levels

		funcToExecute = speedAndDistanceCheck;

		xQueueSendToBack(functionQueue, &funcToExecute, portMAX_DELAY);

		vTaskDelayUntil(&currTicks, 500);
	}
}



void increaseSpeed() {
	if (desiredSpeedLevel < 3) {
		++desiredSpeedLevel;

		funcToExecute = speedAndDistanceCheck;

		xQueueSendToFrontFromISR(functionQueue, &funcToExecute,
				&pxHigherPriorityTaskWoken);
	}
}

void decreaseSpeed() {
	if (desiredSpeedLevel > 0) {
		--desiredSpeedLevel;

		funcToExecute = speedAndDistanceCheck;

		xQueueSendToFrontFromISR(functionQueue, &funcToExecute,
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

void functionExecutor(void* p) {
	funcptr funcToExecute;

	while(1) {
		if(xQueueReceive(functionQueue, &funcToExecute, portMAX_DELAY) == pdTRUE) {
			funcToExecute();
		}
	}
}

void sendUARTMessage(void* p) {
	TickType_t currTicks = 0;

	uint16_t message;

	while(1) {
		message = speedLevel * SPD_OFFSET +
					desiredSpeedLevel * DESIREDSPD_OFFSET +
					distanceReading * DISTANCE_OFFSET;

		xQueueSendToBack(messageQueue, &message, 0);

		vTaskDelayUntil(&currTicks, 1000);
	}
}


//prints the message every 1s
void readUARTMessage(void* p) {
	TickType_t currTicks = 0;

	uint16_t message;

	while (1) {
		//print out received message
		if ((xQueueReceive(messageQueue, &message, portMAX_DELAY)) == pdTRUE) {
			int varToPrint = message / SPD_OFFSET % 10;
			Serial.print("Speed: ");
			Serial.println(varToPrint);
			varToPrint = message / DESIREDSPD_OFFSET % 10;
			Serial.print("Desired Speed: ");
			Serial.println(varToPrint);
			varToPrint = message / DISTANCE_OFFSET % 10;
			Serial.print("Distance: ");
			Serial.print(varToPrint + 1);
			Serial.println("d");
			Serial.println();
		}

		vTaskDelayUntil(&currTicks, 1000);
	}
}

void setup() {
	pinMode(SPEED1_PIN, OUTPUT);
	pinMode(SPEED2_PIN, OUTPUT);
	pinMode(SPEED3_PIN, OUTPUT);
	pinMode(BRAKING_PIN, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(INT1_PIN), increaseSpeedISR, RISING);
	attachInterrupt(digitalPinToInterrupt(INT0_PIN), decreaseSpeedISR, RISING);

	Serial.begin(115200);
	Serial.println("BENIGN");

	messageQueue = xQueueCreate(3, sizeof(uint16_t));
	functionQueue = xQueueCreate(15, 8);

	speedSemaphore = xSemaphoreCreateBinary();

	xSemaphoreGive(speedSemaphore);
}

void loop() {
	xTaskCreate(readDistance,
			"sample_potentiometer",
			100,
			NULL,
			5,
			NULL);

	xTaskCreate(functionExecutor,
			"function_executor",
			200,
			NULL,
			4,
			NULL);

	xTaskCreate(sendUARTMessage,
			"send_UART",
			200,
			NULL,
			3,
			NULL);

	xTaskCreate(readUARTMessage,
			"receive_UART",
			200,
			NULL,
			2,
			NULL);


	vTaskStartScheduler();
}
