/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "lcd.h"
#include <math.h>




typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

int act_volume = 0;
int MainVolumeFlag = 0;
int VolumeFlag = 0;
int MainTextFlag = 0;
int TextFlag = 0;

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState Lab2A_stateA  (Lab2A *me);
// static QState Lab2A_stateB  (Lab2A *me);


/**********************************************************************/


Lab2A AO_Lab2A;


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_stateA);
			}
	}
	
	return Q_SUPER(&QHsm_top);
}


void VolumeUp() {
	printf("Volume: %d\n\r", act_volume);
	setColorBg(231, 50, 19);
	fillRect(act_volume+69, 90, act_volume+70, 110);
	MainVolumeFlag = 1;
	VolumeFlag = 1;
}

void VolumeDown() {
	printf("Volume: %d\n\r", act_volume);
	setColor(0, 0, 0);
	fillRect(act_volume+70, 90, act_volume+71, 110);
	MainVolumeFlag = 1;
	VolumeFlag = 1;
}

void VolumeToggle() {
	setColor(0, 0, 0);
	fillRect(70, 90, 170, 110);
	MainVolumeFlag = 1;
	VolumeFlag = 1;
	act_volume = 0;
}

void OnScreenText(char* s1) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(BigFont);
	lcdPrint(s1, 93, 102);
//	MainTextFlag = 1;
//	TextFlag = 1;
}

void OctaveText(char* s1) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(BigFont);
	lcdPrint(s1, 125, 102);
//	MainTextFlag = 1;
//	TextFlag = 1;
}

void freqText(char *s1) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(BigFont);
	lcdPrint(s1, 165, 102);
//	MainTextFlag = 1;
//	TextFlag = 1;
}

void CentText(char* s1, int cent) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(BigFont);
	lcdPrint("Cents:-", 50, 140);
	lcdPrint(s1, 162, 140);
//		lcdPrint("Cents: ", 58, 140);
//	MainTextFlag = 1;
//	TextFlag = 1;
}

void CentText2(char* s1, int cent) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(BigFont);
	lcdPrint("Cents: ", 50, 140);
	lcdPrint(s1, 162, 140);
//	MainTextFlag = 1;
//	TextFlag = 1;
}

void CentBar(int cent) {
	setColor(0, 0, 0);
	fillRect(70, 175, 170, 195);

	if (cent > 0) {
		setColor(0, 255, 0);
		fillRect(120, 175, 120+(cent-1), 195);
	}
	else {
		setColor(255, 0, 0);
		fillRect(120+(cent+1), 175, 120, 195);
	}
//	MainVolumeFlag = 1;
//	VolumeFlag = 1;
}

void fText(int a) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(SmallFont);
	lcdPrint("     Hz", 165, 106);
	char buffer[20];
	sprintf(buffer, "%d", a);
	lcdPrint(buffer, 167, 106);
}

void aText(int a) {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(SmallFont);
	lcdPrint("A4:   Hz", 157, 88);
	char buffer[20];
	sprintf(buffer, "%d", a);
	lcdPrint(buffer, 180, 88);
}

void printMenu() {
	setColor(0, 0, 0);
	setColorBg(255, 255, 255);
	setFont(SmallFont);
	lcdPrint("Controls:", 3, 230);
	setColor(0, 255, 0);
	lcdPrint("Top:Frequency", 3, 242);
	setColor(0, 0, 0);
	lcdPrint("Left:DecreaseOct", 3, 254);
	setColor(255, 0, 0);
	lcdPrint("Center:Octave", 3, 266);
	setColor(0, 0, 0);
	lcdPrint("Right:IncreaseOct", 3, 278);
	setColor(255, 0, 0);
	lcdPrint("Bottom:Hist", 3, 290);
}

/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

QState Lab2A_stateA(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Startup State\n\r");
			return Q_HANDLED();
		}

		case ENCODER_UP: {
//			xil_printf("Encoder Up from State A\n\r");
			if (act_volume < 100) {
				act_volume ++;
			} else if (act_volume >= 100) {		//Worst case if bugged
				act_volume = 100;
			}
//			VolumeUp();
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
//			xil_printf("Encoder Down from State A\n\r");
			if (act_volume > 0) {
				act_volume --;
			} else if (act_volume <= 0) {		//Worst case if bugged
				act_volume = 0;
			}
//			VolumeDown();
			return Q_HANDLED();
		}

		case ENCODER_CLICK:  {
//			xil_printf("Changing State\n\r");
//			VolumeToggle();
//			return Q_TRAN(&Lab2A_stateB);
			return Q_HANDLED();
		}
		case BTNU: { // UP
//			xil_printf("Up Button Pressed\n\r");
//			OnScreenText("MODE:1");
			return Q_HANDLED();
		}
		case BTND: { // DOWN
			xil_printf(":3\n\r");
//			BG_Fill_Loop();
//			OnScreenText("MODE:5");
			return Q_HANDLED();
		}
		case BTNC: { // CENTER
//			xil_printf("Center Button Pressed\n\r");
//			OnScreenText("MODE:3");
			return Q_HANDLED();
		}
		case BTNL: { // LEFT
//			xil_printf("Left Button Pressed\n\r");
//			OnScreenText("MODE:2");
			return Q_HANDLED();
		}
		case BTNR: { // RIGHT
//			xil_printf("Right Button Pressed\n\r");
//			OnScreenText("MODE:4");
			return Q_HANDLED();
		}

	}

	return Q_SUPER(&Lab2A_on);

}

