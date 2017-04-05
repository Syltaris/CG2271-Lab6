//#include <Arduino.h>
//#include <avr/io.h>
//#include <FreeRTOS.h>
//#include <task.h>
//#include <queue.h>
//#include <semphr.h>
//
//#define STACK_SIZE 200
//
//#define INT0_PIN 2
//#define INT1_PIN 3
//#define POTENTIO_PIN 5
//
//#define SPEED1_PIN 6
//#define SPEED2_PIN 7
//#define SPEED3_PIN 8
//#define BRAKING_PIN 9
//
//#define halfDelayInterval_2Hz 250
//
//volatile unsigned int desiredSpeedLevel = 0;
//unsigned int speedLevel = 0;
//
//unsigned int distanceReading = 0;
//
//unsigned long incSpdISROldMillis = 0;
//unsigned long decSpdISROldMillis = 0;
//
//SemaphoreHandle_t speedSemaphore = NULL;
//
//QueueHandle_t messageQueue = NULL;
//QueueHandle_t valueQueue = NULL;
//
//BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
//
//void speedAndDistanceCheck(void*);
//
//void increaseSpeed() {
//	if (desiredSpeedLevel < 3) {
//		++desiredSpeedLevel;
//
//		Serial.println("fuuuu");
//
//		xQueueSendFromISR(valueQueue, (void *)desiredSpeedLevel, &pxHigherPriorityTaskWoken);
//	}
//}
//
//void decreaseSpeed() {
//	if (desiredSpeedLevel > 0) {
//		--desiredSpeedLevel;
//
//		xQueueSendFromISR(valueQueue, (void *)desiredSpeedLevel, &pxHigherPriorityTaskWoken);
//	}
//}
//
//void increaseSpeedISR() {
//	unsigned long currMillis = millis();
//	Serial.println("fuuuu");
//
//	if (currMillis < incSpdISROldMillis + 200) {
//		increaseSpeed();
//	}
//}
//
//void decreaseSpeedISR() {
//	unsigned long currMillis = millis();
//
//	if (currMillis < decSpdISROldMillis + 200) {
//		decreaseSpeed();
//	}
//}
//
////blinks the required LED based on speedLevel
//void setLEDs() {
//	//off the required LEDs
//	switch (speedLevel) {
//	case 0:
//		digitalWrite(SPEED3_PIN, LOW);
//	case 1:
//		digitalWrite(SPEED2_PIN, LOW);
//	case 2:
//		digitalWrite(SPEED1_PIN, LOW);
//		break;
//	}
//	//on the required LEDs
//	switch (speedLevel) {
//	case 3:
//		digitalWrite(SPEED3_PIN, HIGH);
//	case 2:
//		digitalWrite(SPEED2_PIN, HIGH);
//	case 1:
//		digitalWrite(SPEED1_PIN, HIGH);
//		break;
//	}
//}
//
////turn on red for 1s
//void blinkRED(void*p) {
//	while (1) {
//
//		digitalWrite(BRAKING_PIN, HIGH);
//		vTaskDelay(1000);
//		digitalWrite(BRAKING_PIN, LOW);
//
//	}
//}
//
////reads value from potentiometer and maps it every 0.5s
//void readDistance(void* p) {
//	while (1) {
//		unsigned int* distanceReading = (unsigned int*) p;
//		TickType_t currTicks = xTaskGetTickCount();
//
//		*distanceReading = analogRead(POTENTIO_PIN) / 255; //to accommodate 4 distance levels
//
//		vTaskDelayUntil(&currTicks, 500);
//	}
//}
//
////checks if desiredSpeed > distance, if so, ensure curr speed limit  (???? s)
//void speedAndDistanceCheck(void* p) {
//	while (1) {
//		unsigned int* desiredSpeedLevel = (unsigned int*) p;
//
//		int dummyInt;
//		int oldSpeed = speedLevel;
//
//		//wait for signal to check the values
//		if (xQueueReceive(valueQueue, &dummyInt, portMAX_DELAY) == pdTRUE) {
//			//for marks
//			if (xSemaphoreTake(speedSemaphore, portMAX_DELAY) == pdTRUE) {
//				//check if safe
//				if (*desiredSpeedLevel > distanceReading
//						&& speedLevel != distanceReading) {
//					speedLevel = distanceReading; //match speed to distance level
//
//					//flash red led
//
//				} else if (speedLevel < *desiredSpeedLevel) {
//					speedLevel = *desiredSpeedLevel;
//				} else {
//					speedLevel = *desiredSpeedLevel;
//				}
//
////				if (potFlag && oldSpeed > speedLevel){
////					blinkRed();
////				}
//
//				xSemaphoreGive(speedSemaphore);
//			}
//
//			setLEDs();
//		}
//	}
//}
//
////outputs speaker sound AND drives DC motor
//void motorDriver(void* p) {
//	while (1) {
//
//	}
//}
//
////transmit message through UART every 1s
//void pushMessage(void* p) {
//	while (1) {
//		TickType_t currTicks = xTaskGetTickCount();
//
//		char messageToSend[50];
//
//		if (xSemaphoreTake(speedSemaphore, 0xFFFF) == pdTRUE) {
//			strcpy(messageToSend, "Speed Level: ");
//			strcat(messageToSend, speedLevel + "\n");
//			strcat(messageToSend, "Distance: ");
//			strcat(messageToSend, distanceReading + "\n");
//		} else {
//			strcpy(messageToSend, "ERR_SEMAPHORE_NOT_FREE\n");
//		}
//
//		//SEND MESSAGE TO QUEUE
//		if (xQueueSendToBack(messageQueue, messageToSend,
//				portMAX_DELAY) == pdFALSE) {
//			printf("QUEUE FULL AND CANNOT SEND MESSAGE\n");
//		}
//
//		vTaskDelayUntil(&currTicks, 1000);
//	}
//}
//
////prints the message every 1s
//void readMessage(void* p) {
//	while (1) {
//		TickType_t currTicks = xTaskGetTickCount();
//
//		char messageToPrint[50];
//
//		if ((xQueueReceive(messageQueue, messageToPrint, portMAX_DELAY))
//				== pdFALSE) {
//			printf("QUEUE EMPTY AND CANNOT RECEIVE MESSAGE\n");
//		}
//
//		vTaskDelayUntil(&currTicks, 1000);
//	}
//}
//
//void setup() {
//	pinMode(SPEED1_PIN, OUTPUT);
//	pinMode(SPEED2_PIN, OUTPUT);
//	pinMode(SPEED3_PIN, OUTPUT);
//	pinMode(BRAKING_PIN, OUTPUT);
//
//	attachInterrupt(digitalPinToInterrupt(INT0_PIN), increaseSpeedISR, RISING);
//	attachInterrupt(digitalPinToInterrupt(INT1_PIN), decreaseSpeedISR, RISING);
//
//	messageQueue = xQueueCreate(5, sizeof(char[30]));
//	valueQueue = xQueueCreate(5, sizeof(int));
//
//	Serial.begin(115200);
//	Serial.println("BENING");
//}
//
//void loop() {
//	xTaskCreate(speedAndDistanceCheck,
//			"speed_checking and auto_braking",
//			STACK_SIZE,
//			(void* ) &speedLevel,
//			4,
//			NULL);
//
////	xTaskCreate(readDistance,
////			"sample_potentiometer",
////			STACK_SIZE,
////			(void*) &distanceReading,
////			3,
////			NULL);
//
////	xTaskCreate(motorDriver,
////			"motor_controller",
////			STACK_SIZE,
////			NULL,
////			3,
////			NULL);
//
////	xTaskCreate(blinkRED,
////			"LED_Blinker",
////			STACK_SIZE,
////			(void*) speedLevel,
////			2,
////			NULL);
//
//	xTaskCreate(pushMessage,
//			"message_transmit",
//			STACK_SIZE,
//			(void *) speedLevel,
//			1,
//			NULL);
//
//	xTaskCreate(readMessage,
//			"message_receive",
//			STACK_SIZE,
//			NULL,
//			1,
//			NULL);
//
//	vTaskStartScheduler();
//}
