
#define POWERPIN 8
#define RXPIN 6
#define TXPIN 7

#define POWER_LED_PIN 13

#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include <util/delay.h>
#include <SoftwareSerial.h>
#include <math.h>

#define INPUTBUFFERSIZE 32

#define STATE_WAIT 0
#define STATE_WAITOK 1
#define STATE_WAITCONN 2
#define STATE_CONNECTED 3
#define STATE_SLEEP 4
#define STATE_WAITSLEEP 5
#define STATE_WAKE 6
#define STATE_WAITWAKE 7


//		  RX, TX
SoftwareSerial softSerial(RXPIN, TXPIN);

char InputBuffer[INPUTBUFFERSIZE];
int buffPos = 0;
int sendCount = 0;
int state = STATE_WAIT;


// Watchdog interrupt routine
ISR (WDT_vect) {
	wdt_disable();
}

void enable_watchdog() {
	cli();

	wdt_enable(WDTO_8S);
	// WDT interrupt enable
	WDTCSR |= (1 << WDIE);

	sei();
}

void power_down() {
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	cli();

	sleep_enable();
	sei();

	sleep_cpu();
	sleep_disable();
}

void disable_peripheral() {
	power_usart0_disable();
	power_spi_disable();
	power_twi_disable();

	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();

	// Disable all digital input buffers on ADC pins
	DIDR0 = 0b00111111;
}

void prescale_cpu() {
	// Enable clock prescale changes
	CLKPR = (1 << CLKPCE);

	// Prescale by 16 --> avr runs at 1MHz
	CLKPR = (1 << CLKPS2);
}



void confCmd(char* cmd){
	softSerial.print(cmd);
	delay(100);
}

void setup(){
	//prescale_cpu();
	Serial.begin(9600);
	pinMode(POWERPIN, OUTPUT);
	digitalWrite(POWERPIN, HIGH);
	softSerial.begin(9600);
	powerOnHM10();
	setupHM10();

	//disable timer
	//        power_timer0_disable();
	//        power_timer1_disable();
	//        power_timer2_disable();

	//watchdog on
	enable_watchdog();
	Serial.print("Start configuration");

}

void setupHM10(){
	//check ack
	confCmd("AT");
	//setname
	confCmd("AT+NAMEBLEASY01");
	//AT+PWRM 1 autosleep

	// temperatur
	//	//set service to health thermomenter:
	//	softSerial.print("AT+UUID0x1809");
	//	delay(100);
	//	//set characteristic to temperature
	//	softSerial.print("AT+CHAR0x2A1C");

	////heatrate
	confCmd("AT+UUID0x180D");
	confCmd("AT+CHAR0x2A37");

	//set rate to 1 minute
	confCmd("AT+RAT01");
	// connection timeout
	confCmd("AT+TCON000000");

	//notify on connecttion
	confCmd("AT+NOTI1");
	clearBuf(InputBuffer);
	state=STATE_WAITCONN;
	Serial.print("\nfinisted setup\n");
}

void clearBuf(char* buf){
	int i;
	for(i=0; i<INPUTBUFFERSIZE; i++)  {
		InputBuffer[i]=0;
	}
}

void wakeHM10(){
	int i;
	for(i=0; i<80; i++){
		softSerial.print("a"); 
	}
}

void powerOffHM10(){
	delay(100);
	Serial.print("power off");
	digitalWrite(POWERPIN, LOW);
	digitalWrite(RXPIN, LOW);
	digitalWrite(TXPIN, LOW);
	delay(100);
}

void powerOnHM10(){
	Serial.print("power on");
	digitalWrite(POWERPIN, HIGH);
	delay(100);
}

int parseAnswer(char* answer){
	int res = strncmp(InputBuffer+buffPos-strlen(answer), answer, strlen(answer));
	if(res==0){
		Serial.print("\nACK!");
		Serial.print(answer);
		Serial.print(" state: ");
		Serial.print(state);
		Serial.print("\n");
		clearBuf(InputBuffer);
		buffPos=0;
		return 1;
	}
	else{
		return 0;
	}
}


void sendPayload(){
	delay(200);
	char t = 80.0f+5.0*cos(sendCount++*0.2f);
	char b = 0;
	softSerial.write(b);
	softSerial.print(t);
	softSerial.write(b);
	softSerial.write(b);
	Serial.print("sending value ");
	Serial.print(t);
	Serial.print("\n");
}

void loop(){
	if(state==STATE_CONNECTED && !softSerial.available()){
		sendPayload();
	}

	if (softSerial.available()){
		char c = softSerial.read();
		Serial.write(c);
		InputBuffer[buffPos++]=c;
		parseAnswer("OK");
		switch(state){
		case STATE_WAITCONN:
			if(parseAnswer("CONN")==1){
				Serial.print("connected!\n");
				state = STATE_CONNECTED;
			}
			break;
		case STATE_CONNECTED:
			if(parseAnswer("LOST")==1){
				Serial.print("connection lost\n");
				sendCount=0;
				softSerial.print("AT+SLEEP");
				state=STATE_WAITSLEEP;
				delay(500);
			}
			break;
		case STATE_WAITSLEEP:
			if(parseAnswer("LOST")==1){

			}
			if(parseAnswer("SLEEP")==1){
				Serial.print("modul sleeps\n");
				sendCount=0;
				state=STATE_SLEEP;
				enable_watchdog();
				digitalWrite(POWER_LED_PIN, LOW);
				power_down();
				digitalWrite(POWER_LED_PIN, HIGH);
				powerOnHM10();
                                wakeHM10();
			}
			delay(100);

			break;
		case STATE_SLEEP:
			if(parseAnswer("WAKE")==1){
				Serial.print("modul is awake!\n");
				sendCount=0;
				state=STATE_WAITCONN;
			}

		default:
			break;
		}
	}
	if (Serial.available()){
		//softSerial.write(Serial.read());
		char c = Serial.read();
		if(c=='w'){
			wakeHM10();
		}
		if(c=='o'){
			powerOffHM10();
		}
		if(c=='i'){
			powerOnHM10();
		}
	}
}

