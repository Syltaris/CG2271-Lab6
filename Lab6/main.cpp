//#include <Arduino.h>
//#include <avr/io.h>
//#include <FreeRTOS.h>
//#include <task.h>
//#include <semphr.h>
//#include <queue.h>
//
//#define INT0_PIN 2
//#define INT1_PIN 3
//#define POTENTIO_PIN 5
//#define PIEZO_PIN 10
//#define MOTOR_PIN 11
//
//#define SPEED1_PIN 6
//#define SPEED2_PIN 7
//#define SPEED3_PIN 8
//#define BRAKING_PIN 9
//
//#define SPD_OFFSET 1
//#define DESIREDSPD_OFFSET 10
//#define DISTANCE_OFFSET 100
//
//#define STACK_SIZE 100
//
//
//
//volatile uint8_t desiredSpeedLevel = 0;
//uint8_t speedLevel = 0;
//uint8_t distanceReading = 0;
//
//volatile unsigned long incSpdISROldMillis = 0;
//volatile unsigned long decSpdISROldMillis = 0;
//
//SemaphoreHandle_t speedSemaphore = NULL;
//SemaphoreHandle_t blinkRedSemaphore = NULL;
//
//QueueHandle_t messageQueue = NULL;
//QueueHandle_t valueQueue = NULL;
//
//BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
//
//
//
//void increaseSpeed() {
//	if (desiredSpeedLevel < 3) {
//		++desiredSpeedLevel;
//
//		xQueueSendToBackFromISR(valueQueue, (void *)&desiredSpeedLevel,
//				&pxHigherPriorityTaskWoken);
//
//		if(pxHigherPriorityTaskWoken == pdTRUE) {
//			taskYIELD();
//		}
//	}
//}
//
//void decreaseSpeed() {
//	if (desiredSpeedLevel > 0) {
//		--desiredSpeedLevel;
//
//		xQueueSendToBackFromISR(valueQueue, (void * )&desiredSpeedLevel,
//				&pxHigherPriorityTaskWoken);
//
//		if(pxHigherPriorityTaskWoken == pdTRUE) {
//			taskYIELD();
//		}
//	}
//}
//
//void increaseSpeedISR() {
//	unsigned long currMillis = millis();
//
//	if (currMillis > incSpdISROldMillis + 500) {
//		increaseSpeed();
//
//		incSpdISROldMillis = currMillis;
//	}
//}
//
//void decreaseSpeedISR() {
//	unsigned long currMillis = millis();
//
//	if (currMillis > decSpdISROldMillis + 500) {
//		decreaseSpeed();
//
//		decSpdISROldMillis = currMillis;
//	}
//}
//
////reads value from potentiometer and maps it every 0.5s
//void readDistance(void* p) {
//	TickType_t currTicks = 0;
//
//	while (1) {
//		distanceReading = analogRead(POTENTIO_PIN) / 256; //to accommodate 4 distance levels
//		xQueueSendToBack(valueQueue, (void*) &distanceReading, portMAX_DELAY);
//
//		vTaskDelayUntil(&currTicks, 500);
//	}
//}
//
////blinks the required LED based on speedLevel
//void setLEDs() {
//	digitalWrite(SPEED1_PIN, speedLevel >= 1 ? HIGH : LOW);
//	digitalWrite(SPEED2_PIN, speedLevel >= 2 ? HIGH : LOW);
//	digitalWrite(SPEED3_PIN, speedLevel >= 3 ? HIGH : LOW);
//}
//
////checks if desiredSpeed > distance, if so, ensure curr speed limit  (???? s)
//void speedAndDistanceCheck(void* p) {
//	TickType_t currTick = 0;
//
//	while (1) {
//		uint8_t dummyInt;
//
//		//wait for signal to check the values
//		if (xQueueReceive(valueQueue, &dummyInt, portMAX_DELAY) == pdTRUE) {
//			//for marks
//			if (xSemaphoreTake(speedSemaphore, portMAX_DELAY) == pdTRUE) {
//				dummyInt = speedLevel;
//
//				//check if safe
//				speedLevel = (desiredSpeedLevel < distanceReading) ? desiredSpeedLevel : distanceReading;
//
//				//set corresponding yellow LEDs
//				setLEDs();
//
//				//if auto braking occurred
//				if((desiredSpeedLevel > distanceReading) && (dummyInt > distanceReading)) {
//					xSemaphoreGive(blinkRedSemaphore);
//				}
//
////				//send the message
////				uint16_t* message;
////				*message = (speedLevel * SPD_OFFSET +
////							desiredSpeedLevel * DESIREDSPD_OFFSET +
////							distanceReading * DISTANCE_OFFSET);
////				xQueueSendToBack(messageQueue, message, 0);
//
//				xSemaphoreGive(speedSemaphore);
//			}
//		}
//	}
//}
//
////blink red led
//void blinkRED(void* p) {
//	TickType_t currTick = 0;
//
//	while(1) {
//		if(xSemaphoreTake(blinkRedSemaphore, portMAX_DELAY) == pdTRUE) {
//			currTick = xTaskGetTickCount();
//			digitalWrite(BRAKING_PIN, HIGH);
//			vTaskDelayUntil(&currTick, 1000);
//			digitalWrite(BRAKING_PIN, LOW);
//		}
//	}
//}
//
////drive the piezo and motor
//void motorDriver(void* p) {
//	while(1) {
//		if(xSemaphoreTake(speedSemaphore, portMAX_DELAY) ==  pdTRUE) {
//			analogWrite(PIEZO_PIN, 25 * speedLevel);
//			vTaskDelay(10 * (3-speedLevel));
//			analogWrite(PIEZO_PIN, 0);
//			vTaskDelay(10 * (3-speedLevel));
////
////			//controls the motor speed
////			analogWrite(MOTOR_PIN, 85 * speedLevel);
//			xSemaphoreGive(speedSemaphore);
//		}
//	}
//}
//
////void sendUARTMessage(void* p) {
////	TickType_t currTicks = 0;
////
////	while(1) {
////		*message = speedLevel * SPD_OFFSET +
////					desiredSpeedLevel * DESIREDSPD_OFFSET +
////					distanceReading * DISTANCE_OFFSET;
////		xQueueSendToBack(messageQueue, message, 0);
////
////		vTaskDelayUntil(&currTicks, 1000);
////	}
////}
//
//
//////prints the message every 1s
////void readUARTMessage(void* p) {
////	TickType_t currTicks = 0;
////
////	uint16_t* message;
////
////	while (1) {
////		//print out received message
////		if ((xQueueReceive(messageQueue, message, portMAX_DELAY)) == pdTRUE) {
//////			Serial.print("SPD: " + *message/SPD_OFFSET);
//////			Serial.print("DES: "+ *message/DESIREDSPD_OFFSET);
//////			Serial.println("DIS: "+ *message/DISTANCE_OFFSET == 0 ? "d"
//////									: *message/DISTANCE_OFFSET == 1 ? "2d"
//////									: *message/DISTANCE_OFFSET == 2 ? "3d"
//////									: *message/DISTANCE_OFFSET == 3 ? "4d" : "ERR");
////		}
////		Serial.print("SPD: ");
////
////
////		vTaskDelayUntil(&currTicks, 1000);
////	}
////}
//
//void setup() {
//	pinMode(SPEED1_PIN, OUTPUT);
//	pinMode(SPEED2_PIN, OUTPUT);
//	pinMode(SPEED3_PIN, OUTPUT);
//	pinMode(BRAKING_PIN, OUTPUT);
//
//	attachInterrupt(digitalPinToInterrupt(INT1_PIN), increaseSpeedISR, RISING);
//	attachInterrupt(digitalPinToInterrupt(INT0_PIN), decreaseSpeedISR, RISING);
//
//	Serial.begin(115200);
//	Serial.println("BENIGN");
//
//	messageQueue = xQueueCreate(3, sizeof(uint16_t));
//	valueQueue = xQueueCreate(1, sizeof(uint8_t));
//
//	speedSemaphore = xSemaphoreCreateBinary();
//	blinkRedSemaphore = xSemaphoreCreateBinary();
//
//	xSemaphoreGive(speedSemaphore);
//}
//
//void loop() {
//	xTaskCreate(readDistance,
//			"sample_potentiometer",
//			150,
//			NULL,
//			5,
//			NULL);
//
//	xTaskCreate(speedAndDistanceCheck,
//			"speed_checking and auto_braking",
//			150,
//			NULL,
//			4,
//			NULL);
//
//	xTaskCreate(blinkRED,
//			"red_led_blinker",
//			100,
//			NULL,
//			3,
//			NULL);
//
//	xTaskCreate(motorDriver,
//			"motor_controller",
//			100,
//			NULL,
//			3,
//			NULL);
//
////	xTaskCreate(sendUARTMessage,
////			"message_receive",
////			150,
////			NULL,
////			2,
////			NULL);
////
////	xTaskCreate(readUARTMessage,
////			"message_receive",
////			150,
////			NULL,
////			1,
////			NULL);
//
//	vTaskStartScheduler();
//}
