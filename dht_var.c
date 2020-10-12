/*
 *  dht11.c:
 *  Simple test program to test the wiringPi functions
 *
 */

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXTIMINGS 85
int SENSOR_PIN = 4;

/*
* digitalRead() and friends from wiringpi are defined as returning a value < 256.
* However, they are returned as int() types.
* This is a safety function.
*/
static uint8_t readPin(const int sensorPin) {
  int value = digitalRead(sensorPin);
  
  if (value > 255 || value < 0) {
    printf("Invalid data from wiringPi library\n");
    exit(EXIT_FAILURE);
  }

  return (uint8_t)value;
}

static int readData() {
  uint8_t lastState = HIGH;
  uint8_t j = 0;
  int data[5] = {0,0,0,0,0};

  pinMode(SENSOR_PIN, OUTPUT);
  digitalWrite(SENSOR_PIN, LOW);
  delay(18);
  digitalWrite(SENSOR_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(SENSOR_PIN, INPUT);

  // detect change and read data
  for (uint8_t i=0; i< MAXTIMINGS; i++) {
    uint8_t counter = 0;

    while (readPin(SENSOR_PIN) == lastState) {
      counter++;
      delayMicroseconds(1);

      if (counter == 255) {
        break;
      }
    }

    lastState = readPin(SENSOR_PIN);

    if (counter == 255) {
		break;
	}

    // ignore first 3 transitions
    if ((i >= 4) && (i%2 == 0)) {
      // shove each bit into the storage bytes
      data[j/8] <<= 1;

      if (counter > 16) {
        data[j/8] |= 1;
      }

      j++;
    }
  }

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
  // print it out if data is good
  if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    float humidity = ((float)data[0] * 256 + (float)data[1]) / 10;
    float temperature = ((float)(data[2] & 0x7F) * 256 + (float)data[3]) / 10.0;

    if ((data[2] & 0x80) != 0) {
        temperature *= -1;
    }

    printf("%.1f,%.1f\n", humidity, temperature);
    return 1;
  }
  else
  {
    printf("Data not good, skip\n");
    return 0;
  }
}

int main (int argc, char *argv[]) {
  int tries = 100;

  if (argc < 2) {
    printf("usage: %s <pin> (<tries>)\ndescription: pin is the wiringPi pin number\nusing 2 (GPIO 27)\nOptional: tries is the number of times to try to obtain a read (default 100)", argv[0]);
  }
  else {
    SENSOR_PIN = atoi(argv[1]);
  }

  if (argc == 3) {
    tries = atoi(argv[2]);
  }

  if (tries < 1) {
    printf("Invalid tries supplied\n");
    exit(EXIT_FAILURE);
  }

  if (wiringPiSetup () == -1) {
    exit(EXIT_FAILURE) ;
  }

  if (setuid(getuid()) < 0) {
    perror("Dropping privileges failed\n");
    exit(EXIT_FAILURE);
  }

  while (readData() == 0 && tries--)  {
     delay(500); // wait 0.5 sec before retry
  }

  return 0;
}