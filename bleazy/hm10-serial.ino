#include <SoftwareSerial.h>

#define INPUTBUFFERSIZE 32
#define SENDBUFFERSIZE 32

#define STATE_WAIT 0
#define STATE_WAITOK 1
#define STATE_WAITCONN 2
#define STATE_CONNECTED 3
#define STATE_SLEEP 4
#define STATE_WAITSLEEP 5
#define STATE_WAKE 6
#define STATE_WAITWAKE 7

//						  RX, TX
SoftwareSerial softSerial(10, 11);

char InputBuffer[32];
char SendBuffer[32];
int buffPos = 0;
int sendCount = 0;
int state = STATE_WAIT;


void setup(){
	Serial.begin(9600);
	softSerial.begin(9600);
	Serial.print("Hello, please wait...");
	delay(100);
	//check ack
	softSerial.print("AT");
	delay(100);
	//setname
	//Serial.print("Set name: ");
	softSerial.print("AT+NAMEBLEASY01");
	delay(100);
	//set service to health thermomenter:
	softSerial.print("AT+UUID0x1809");
	delay(100);
	//set characteristic to temperature
	softSerial.print("AT+CHAR0x2A1C");
	delay(100);
	//set rate to 1 minute
	//Serial.print("Set send rate: ");
	softSerial.print("AT+RAT01");
	delay(100);
	softSerial.print("AT+TCON000000");
	delay(100);
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

int parseAnswer(char* answer){
	int res = strncmp(InputBuffer+buffPos-strlen(answer), answer, strlen(answer));
	if(res==0){
		Serial.print("\nACK!");
		Serial.print(answer);
		Serial.print(state);
		Serial.print("\n");
		clearBuf(InputBuffer);
		buffPos=0;
		return 1;
	}
	else
		return 0;
}


void loop(){
	if(state==STATE_CONNECTED && !softSerial.available()){
		delay(500);
		Serial.print("sending temp\n");
		softSerial.print("42grad");
		softSerial.print(sendCount++);
		delay(500);
	}
	if(sendCount >5){
		softSerial.print("AT");
		delay(200);
		sendCount=0;
		state=STATE_WAITSLEEP;
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
				state=STATE_WAITSLEEP;
				delay(500);
			}
			break;
		case STATE_WAITSLEEP:
			if(parseAnswer("LOST")==1){
				softSerial.print("AT+SLEEP");
			}
			if(parseAnswer("SLEEP")==1){
				Serial.print("modul sleeps\n");
				sendCount=0;
				state=STATE_SLEEP;
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
	if (Serial.available())
		softSerial.write(Serial.read());
}

