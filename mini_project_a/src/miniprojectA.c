//Mini Project A
#include "miniprojectA.h"
int hours, mins, secs;
int RTC; //Holds the RTC instance
int main(void)
{
	signal(SIGINT, exiting);
	wiringPiSetup();
	wiringPiSPISetup(SPI_CHAN_DAC, SPI_SPEED_DAC);
	RTC = wiringPiI2CSetup(RTCAddr);
	toggleTime();
	mcp3004Setup(BASE, SPI_CHAN);
	
	// unsigned​ ​char​ ​buffer​[​2​][BUFFER_SIZE][​2​];
	// int​ buffer_location = ​0​;
	// bool​ buffer_reading = ​0​;


	pthread_attr_t tattr;
    pthread_t thread_id;
    int newprio = 99;
    sched_param param;
    pthread_attr_init (&tattr);
    pthread_attr_getschedparam (&tattr, &param);
    param.sched_priority = newprio;
    pthread_attr_setschedparam (&tattr, &param);
    pthread_create(&thread_id, &tattr, read_adc, (void *)1);

	for (;;)
	{
		printf("Humidity: %0.1fV\n", channels[3] * 3.3 / 1023);
		printf("Light Level: %d\n", channels[0]);
		printf("Temperature: %0.0f\n", round(((channels[1] * 3.3 / 1023) - 0.5) / 0.01));
		secs = hexCompensation(wiringPiI2CReadReg8(RTC, SEC) - 0b10000000);
		mins = hexCompensation(wiringPiI2CReadReg8(RTC, MIN));
		hours = decCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		float light = channels[0];
		float hum = channels[3] * 3.3 / 1023;
		int DAC = (int)((light / 1023) * hum * 1023 / 3.3);
		unsigned char * dac_char_array = (unsigned char *) 0b0111<<12 + DAC<<2 + 0b00;
		
		// dac_char_array = 1023;
		unsigned char DAC_VAL[3] = {(DAC & 0b1100000000) >> 8, (DAC & 0b11110000) >> 4, DAC & 0b1111};
		printf("dac_char_array: %d\n",dac_char_array);
		float DAC_VOLTAGE = DAC * 3.3 / 1023;
		printf("DAC Voltage: %f\n", DAC_VOLTAGE);
		printf("ADC_DAC Voltage: %0.1f\n", (channels[2] * 3.3) / 1023);
		printf("The current time is: %dh%dm%ds\n", hours, mins, secs);
		printf("\n");
		wiringPiSPIDataRW(SPI_CHAN_DAC, dac_char_array, 1);
		// wiringPiSPIDataRW(SPI_CHAN_DAC, buffer[buffer_reading][buffer_location],2);
		// buffer_location++;
        // if(buffer_location >= BUFFER_SIZE) {
        //     buffer_location = 0;
        //     buffer_reading = !buffer_reading; 
        // }
		// buffer[buffer_writing][counter][0] = dac_char_array; 
        // buffer[buffer_writing][counter][1] = dac_char_array; 
		delay(1000);
	}
	return 0;
}
void *read_adc(void *threadargs)
{
	while (true)
	{
		for (int chan = 0; chan < 8; ++chan)
		{
			channels[chan] = analogRead(BASE + chan);
		}
		delay(200);
	}
}


void exiting(int x)
{
	printf("Shutting down\n");
	exit(0);
}
/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours)
{
	/*formats to 12h*/
	if (hours >= 24)
	{
		hours = 0;
	}
	else if (hours > 12)
	{
		hours -= 12;
	}
	return (int)hours;
}
int hexCompensation(int units)
{
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic)
	*/
	int unitsU = units % 0x10;

	if (units >= 0x50)
	{
		units = 50 + unitsU;
	}
	else if (units >= 0x40)
	{
		units = 40 + unitsU;
	}
	else if (units >= 0x30)
	{
		units = 30 + unitsU;
	}
	else if (units >= 0x20)
	{
		units = 20 + unitsU;
	}
	else if (units >= 0x10)
	{
		units = 10 + unitsU;
	}
	return units;
}
/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units)
{
	int unitsU = units % 10;

	if (units >= 50)
	{
		units = 0x50 + unitsU;
	}
	else if (units >= 40)
	{
		units = 0x40 + unitsU;
	}
	else if (units >= 30)
	{
		units = 0x30 + unitsU;
	}
	else if (units >= 20)
	{
		units = 0x20 + unitsU;
	}
	else if (units >= 10)
	{
		units = 0x10 + unitsU;
	}
	return units;
}
//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void)
{
	long interruptTime = millis();

	//if (interruptTime - lastInterruptTime>200){
	HH = getHours();
	printf("Hour: %d\n",HH);
	MM = getMins();
	printf("Minutes: %d\n",MM);
	SS = getSecs();
	printf("Seconds: %d\n",SS);

	HH = hFormat(HH);
	HH = decCompensation(HH);
	wiringPiI2CWriteReg8(RTC, HOUR, HH);

	MM = decCompensation(MM);
	wiringPiI2CWriteReg8(RTC, MIN, MM);

	SS = decCompensation(SS);
	wiringPiI2CWriteReg8(RTC, SEC, 0b10000000 + SS);

	//	}
	//	lastInterruptTime = interruptTime;
}
