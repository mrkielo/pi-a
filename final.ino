//libs
#include<math.h>
#include <Key.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

//pins definition
int clockpin = 3;
int datapin = 2;

#define L_IS A2
#define R_IS A3
#define R_EN A1
#define L_EN A0
#define LPWM 13
#define RPWM 12

#define lcdLED 8
#define stopButton 9


//lcd
LiquidCrystal lcd(27, 22, 23, 24, 25, 26);


//keypad
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '.', 'D'}
};

byte rowPins[ROWS] = {A8, A9, A10, A11}; 
byte colPins[COLS] = {A12, A13, A14, A15}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

float digits[] = {0,1,2,3,4,5,6,7,8,9};
char digitChars[] = {'0','1','2','3','4','5','6','7','8','9','.'};


//KEPYPAD METHOD
float number;
String string;
bool isEngine = false;



//engine
float time = millis();
float lSpeed = 0;
float rSpeed = 0;

//capiler
int sign;
long value;
unsigned long tempmicros;

//decode
float last;
float lastEngineSpeed = 0;
float difference = 10;
int loops = 0;
float capilerMax = 327.68;

//sterring
float real;
float target = 0;

//parameters
float slowDownOffset = 50;
float slowSpeed = 25;
float highSpeed = 200;
float hesitate = 1;
float maxValue = 1311;


//engine funcs
void FWD() {
	lSpeed = 0;
	if(abs(target-real)<slowDownOffset) rSpeed = slowSpeed;
	else rSpeed = highSpeed;
	isEngine = true;
}

void RWD() {
	rSpeed = 0;
	if(abs(target-real)<slowDownOffset) lSpeed = slowSpeed;
	else lSpeed = highSpeed;
	isEngine = true;
}

void STOP() {
	lSpeed = 0;
	rSpeed = 0;
	isEngine = false;
}

void show() {
	Serial.print("target: ");
	Serial.println(target);
}


float decode(float capilerInput) {

	if(lSpeed>0) lastEngineSpeed = -1;
	else if(rSpeed>0) lastEngineSpeed =1;
	else if(rSpeed==0 && lSpeed==0) lastEngineSpeed = 0;

	if(lastEngineSpeed>0 && capilerInput+difference<last) {
		loops++;
	}

	if(lastEngineSpeed<0 && capilerInput-difference>last) {
		loops--;
	}

	

	last = capilerInput;
	return (2*loops*capilerMax + capilerInput);	
}


float code(float capilerInput) {
	float capilerMax = 327.68; // max capiler value

	float c = capilerInput/capilerMax;
	float k = int(c) * capilerMax - capilerInput;
	
	if(int(c)%2==0)
		return -k;
	else if(k==0)
		return -capilerMax;
	else
		return -(capilerMax + k); 
}



float capiler(int clockpin, int datapin) {

	float result;
	int i;
	while (digitalRead(clockpin) == HIGH)
	{
	} //if clock is LOW wait until it turns to HIGH
	tempmicros = micros();

	while (digitalRead(clockpin) == LOW)
	{
	} //wait for the end of the HIGH pulse
	if ((micros() - tempmicros) > 500)
	{ //if the HIGH pulse was longer than 500 micros we are at the start of a new bit sequence
		sign = 1;

	value = 0;

	for (i = 0; i < 23; i++)
	{

		while (digitalRead(clockpin) == HIGH) {} //wait until clock returns to HIGH- the first bit is not needed

		while (digitalRead(clockpin) == LOW) {}  //wait until clock returns to LOW

		if (digitalRead(datapin) == LOW)
		{
			if (i < 20)
			{
				value |= 1 << i;
			}
			if (i == 20)
			{
				sign = -1;
			}
		}
	}
	result = (value * sign) / 100.00;
	return result;		
	}
}



void keypadStandard() {
	char key = customKeypad.getKey();
	if(key && !isEngine) {

		for(int i=0; i<10;i++) { //only numbers
			if(key==digitChars[i]) {
				string+=digitChars[i];
				break;
			}
		}
		
		if(key == 'B') { //DELETE
			string.remove(string.length()-1,1);
			lcd.clear();
		}

		if(key == 'A') { //START
			number = string.toFloat();
			target = number;
		}

		if(key == 'C') { //CLEAR
			string = "";
			lcd.clear();
		}



	}
}

void setup() {
	pinMode(lcdLED,OUTPUT);
	lcd.begin(16,2);

	Serial.begin(9600);
	pinMode(clockpin, INPUT);
	pinMode(datapin, INPUT);
	analogWrite(lcdLED,80);

	string="";
	lSpeed = 0;
	rSpeed = 0;
	pinMode(L_IS,OUTPUT);
	pinMode(R_IS,OUTPUT);
	pinMode(R_EN,OUTPUT);
	pinMode(L_EN,OUTPUT);
	pinMode(RPWM,OUTPUT);
	pinMode(LPWM,OUTPUT);
	
	
	pinMode(stopButton, INPUT_PULLUP);

	digitalWrite(L_IS,LOW);
	digitalWrite(R_IS,LOW);
	digitalWrite(R_EN,HIGH);
	digitalWrite(L_EN,HIGH);
	loops = 0;
}

void loop() {	

	//engine
	analogWrite(RPWM,rSpeed);
	analogWrite(LPWM,lSpeed);

	real = decode(capiler(clockpin,datapin)); //capiler read


	keypadStandard(); //keypad handler

  
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(string);
	lcd.setCursor(0,2);
	lcd.print(real);

	if(digitalRead(stopButton)==LOW) target=-1; // STOPBUTTON



	
	if(target<maxValue && target>=-1) {		// ENGINE DRIVER
		if((real>=target-hesitate && real<target+hesitate) || target==-1) STOP();
		else if(real<target-hesitate) FWD();
		else if(real>target+hesitate) RWD();
	}




}