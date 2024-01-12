/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include <stdio.h>
#include <math.h>
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"
#include <xio.h>
#include "xtmrctr.h"
#include "xintc.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include <xbasic_types.h>
#include "qpn_port.h"                                       /* QP-nano port */                               /* application interface */		                /* Cache Drivers */
#include "lcd.h"

/*****************************/

/* Define all variables and Gpio objects here  */

XIntc sys_intctr;
XGpio enc_gpio;
XGpio twist_Gpio;
XGpio led_gpio;
XGpio rgb_led_gpio;
XTmrCtr sys_tmrctr;
XGpio dc;
XSpi spi;
XGpio btn;

#define RESET_VALUE 0x5F5E100
#define GPIO_CHANNEL1 1
#define LED_CHANNEL 1
#define BUTTON_CHANNEL 1

int toggle = 1;
int NumOfTri = 0;
int volume = 0;
int count = 0;
int TimerFlag = 0;
int EncTrig = 0;
static int encoder_count = 4;
static Xuint16 state = 0b11;
volatile u16 led_16 = 1;
int VolumeTimeOut = 0;
int TextTimeOut = 0;
volatile int a = 440;
volatile int printFrequency = 1;
volatile int pause = 0;
volatile histPrint = 0;

// #define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
// #define M 9 //2^m=samples
//#define SAMPLES 1024
//#define M 10
#define SAMPLES 2048 // AXI4 Streaming Data FIFO has size 512
#define M 11 //2^m=samples
#define CLOCK 100000000.0 //clock speed

int tenth_avg = 0;
int reducer = 2; // originally 2
int temp_avg = 0;
int avg_count = 0;
static float q[SAMPLES];
static float w[SAMPLES];

volatile int octLim = 0;
volatile int octCur = 4;
volatile int octSwitch2 = 0;

volatile int histSwitch = 0;
volatile int octSwitch = 0;
volatile int fSwitch = 0;
volatile int aSwitch = 1;

float sample_f;
int l;
int ticks; //used for timer
uint32_t Control;
float frequency;
float tot_time; //time to run program

static enum STATES {
		S0 = 0b11,
		S1 = 0b01,
		S2 = 0b00,
		S3 = 0b10
};

void debounceInterrupt(); // Write This function

// Create ONE interrupt controllers XIntc
// Create two static XGpio variables
// Suggest Creating two int's to use for determining the direction of twist

/*..........................................................................*/

/* ----- New Timer Handler ----- */
void TimerCounterHandler(void *CallBackRef)
{

	if (MainVolumeFlag == 1) {
	if (VolumeFlag == 1) {
		setColor(255, 165, 0);
		fillRect(70, 90, act_volume+70, 110);
		VolumeTimeOut = 0;
		VolumeFlag = 0;
	}
	if (VolumeTimeOut > 3069) {
		xil_printf("Timed Out\n\r");
		BG_Fill_Loop();
		MainVolumeFlag = 0;
	}
	VolumeTimeOut++;
	}

	if (MainTextFlag == 1) {
		if (TextFlag == 1) {
			TextTimeOut = 0;
			TextFlag = 0;
		}
		if (TextTimeOut > 3069) {
			xil_printf("Timed Out\n\r");
			BG_Fill_Loop();
			MainTextFlag = 0;
		}
		TextTimeOut++;
	}
}


void BSP_init(void) {
/* Setup LED's, etc */
/* Setup interrupts and reference to interrupt handler function(s)  */

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */
	XStatus Status;
	Status = XST_SUCCESS;
	u32 status;
	u32 controlReg;
	XSpi_Config *spiConfig;

	/* ----- Initialize Interrupt ----- */
	Status = XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);

	/* ----- Initialize LED ----- */
	Status = XGpio_Initialize(&led_gpio, XPAR_AXI_GPIO_LED_DEVICE_ID);

	/* ----- Initialize RGB LED ----- */
	Status = XGpio_Initialize(&rgb_led_gpio, XPAR_AXI_GPIO_RGBLEDS_DEVICE_ID);

	/* ----- Initialize ENCODER ----- */
	Status = XGpio_Initialize(&enc_gpio, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);

	XIntc_Connect(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR,
			(Xil_ExceptionHandler)TwistHandler, &enc_gpio);
	XIntc_Enable(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XGpio_InterruptEnable(&enc_gpio, GPIO_CHANNEL1);
	XGpio_InterruptGlobalEnable(&enc_gpio);

	Status = XGpio_Initialize(&btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);

	XIntc_Connect(&sys_intctr, XPAR_INTC_0_GPIO_1_VEC_ID,
			(Xil_ExceptionHandler)GpioHandler, &btn);
	XIntc_Enable(&sys_intctr, XPAR_INTC_0_GPIO_1_VEC_ID);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XGpio_InterruptEnable(&btn, BUTTON_CHANNEL);
	XGpio_InterruptGlobalEnable(&btn);

	/* ----- New Timer Setup ----- */
	Status = XTmrCtr_Initialize(&sys_tmrctr, XPAR_AXI_TIMER_0_DEVICE_ID);
	Status = XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);
	Status = XIntc_Connect(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
				(XInterruptHandler)XTmrCtr_InterruptHandler,
				(void *)&sys_tmrctr);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XIntc_Enable(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
				microblaze_enable_interrupts();
	XTmrCtr_SetHandler(&sys_tmrctr, TimerCounterHandler, &sys_tmrctr);
	XTmrCtr_SetOptions(&sys_tmrctr, 0,
				XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&sys_tmrctr, 0, 0xFFFF0000);
	XTmrCtr_Start(&sys_tmrctr, 0);



	status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	XGpio_SetDataDirection(&dc, 1, 0x0);
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	XSpi_Reset(&spi);
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	/* ----- Initialize MICROBLAZE ----- */
	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
			(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	microblaze_enable_interrupts();
		
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

	initLCD();
	clrScr();
	BG_Fill();
	printMenu();
	histBackground();
	setColor(239,230,213);
	drawHLine(160, 270, 80);

	// Press Knob
	// Enable interrupt controller
	// Start interupt controller
	// register handler with Microblaze
	// Global enable of interrupt
	// Enable interrupt on the GPIO

	// Twist Knob

	// General
	// Initialize Exceptions
	// Press Knob
	// Register Exception
	// Twist Knob
	// Register Exception
	// General
	// Enable Exception

	// Variables for reading Microblaze registers to debug your interrupts.
//	{
//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
//	}
}

void DrawBorder() {
	for (int width = 0; width<6;width++) {
		drawHLine(0, width, DISP_X_SIZE);
		drawHLine(0, DISP_Y_SIZE-width-10, DISP_X_SIZE);
		drawVLine(width, 0, DISP_Y_SIZE);
		drawVLine(DISP_X_SIZE-width, 0, DISP_Y_SIZE);
	}
}

void histBackground() {
	setColor(0, 0, 0);
	fillRect(160, 240, 239, 300);
}

void drawHist(int bin1, int bin2, int bin3, int bin4, int bin5, int bin6, int bin7, int bin8, int bin9, int bin10, int f) {
//	xil_printf("testing!\n\r");
//	void histBackground() {
//		setColor(0, 0, 0);
//		fillRect(160, 240, 239, 319);
//	}
	histBackground();
	setColor(0,0,255);
	drawHLine(162, 242, bin1);
	drawHLine(162, 244, bin2);
	drawHLine(162, 246, bin3);
	drawHLine(162, 248, bin4);
	drawHLine(162, 250, bin5);
	drawHLine(162, 252, bin6);
	drawHLine(162, 254, bin7);
	drawHLine(162, 256, bin8);
	drawHLine(162, 258, bin9);
	drawHLine(162, 260, bin10);
	setFont(SmallFont);
	char temp[20];
	sprintf(temp, "%d", f);
	lcdPrint(temp, 161, 280);
}

void BG_Fill() {
	clrScr();
	for (int Row = 0; Row<8; Row++) {
		int NewRow = Row*40;
		for (int Col = 0; Col<6; Col++) {
			int NewCol = Col*40;
			for (int y = 0; y<40; y++) {
				int blue = 2*ceil(y/2);
				setColor(239,230,213);
				drawHLine(0+NewCol, y+NewRow, 20-(blue/2));
				setColor(157, 190, 183);
				drawHLine(20-(blue/2)+NewCol, y+NewRow, blue);
				setColor(239,230,213);
				drawHLine(20+(blue/2)+NewCol, y+NewRow, 20-(blue/2));
			}
		}
	}
}

void BG_Fill_Loop() {
	for (int Row = 0; Row<5; Row++) {
		int NewRow = Row*40;
		for (int Col = 0; Col<6; Col++) {
			int NewCol = Col*40;
			for (int y = 0; y<40; y++) {
				int blue = 2*ceil(y/2);
				setColor(239,230,213);
				drawHLine(0+NewCol, y+NewRow, 20-(blue/2));
				setColor(157, 190, 183);
				drawHLine(20-(blue/2)+NewCol, y+NewRow, blue);
				setColor(239,230,213);
				drawHLine(20+(blue/2)+NewCol, y+NewRow, 20-(blue/2));

			}
		}
	}
}

void QF_onIdle(void) {        /* entered with interrupts locked */
    QF_INT_UNLOCK();
    float midc = 0.5946 * a;
    float midb = 1.122 * a;
    float low[9] = {midc/16.0, midc/8.0, midc/4.0, midc/2, midc, 2*midc, 4*midc, 8*midc, 16*midc};
    float up[9] = {midb/16.0, midb/8.0, midb/4.0, midb/2, midb, 2*midb, 4*midb, 8*midb, 16*midb};

    if (pause != 1) {
	int reducer_sq = 4;
    read_fsl_values(q, SAMPLES);

    sample_f = 100*1000*1000/2048.0;

    //zero w array
    for(l=0;l<SAMPLES;l++)
       w[l]=0;

    if (histPrint == 1) {
        frequency=fftHist(q,w,SAMPLES/reducer_sq,M-reducer,sample_f/reducer_sq);
    }
    else {
        frequency=fft(q,w,SAMPLES/reducer_sq,M-reducer,sample_f/reducer_sq);
    }

    //ignore noise below set frequency
    	if (printFrequency && (pause != 1)) { xil_printf("frequency: %d Hz\r\n", (int)(frequency)); }
     	if ((frequency > 0) && (pause != 1)) {
     		if (octLim) {
     			if ((low[octCur] <= frequency) && (frequency <= up[octCur])) {
     				xil_printf("within limit\n\r");
     				xil_printf("min: %d f: %d max: %d\n\r", (int)low[octCur], (int)frequency, (int)up[octCur]);
     				findNote(frequency, a);
     			}
     		}
     		else { findNote(frequency, a); }
     	}

     	if ((pause != 1) && printFrequency && (frequency > 0)) {
     		if (octLim) {
     			if ((low[octCur] <= frequency) && (frequency <= up[octCur])) {
     	        	fText((int)frequency);
     			}
     		}
     		else { fText((int)frequency); }
        }
     	else if ((pause != 1) && !printFrequency) {
     		setColor(0, 0, 0);
     		setColorBg(255, 255, 255);
     		setFont(SmallFont);
     		lcdPrint("       ", 165, 106);
     	}

     	if (aSwitch) {
     		aText((int)a);
     		aSwitch = 0;
     	}
    }
//	lcdPrint("Top:Frequency", 3, 242);
//	setColor(0, 0, 0);
//	lcdPrint("Left:DecreaseOct", 3, 254);
//	setColor(255, 0, 0);
//	lcdPrint("Center:Octave", 3, 266);
//	setColor(0, 0, 0);
//	lcdPrint("Right:IncreaseOct", 3, 278);
//	setColor(255, 0, 0);
//	lcdPrint("Bottom:Hist", 3, 290);
    if (fSwitch) {
		if (printFrequency) {
			setColor(0, 255, 0);
			setFont(SmallFont);
			lcdPrint("Top:Frequency", 3, 242);
		}
		else {
			setColor(255,0,0);
			setFont(SmallFont);
			lcdPrint("Top:Frequency", 3, 242);
		}
		fSwitch = 0;
    }

    if (octSwitch) {
		if (octLim) {
			setColor(0, 255, 0);
			setFont(SmallFont);
			lcdPrint("Center:Octave", 3, 266);
			setColor(1, 130, 32);
			char hola[3];
			sprintf(hola, "%d", octCur);
			lcdPrint(hola, 110, 266);
		}
		else {
			setColor(255,0,0);
			setFont(SmallFont);
			lcdPrint("Center:Octave", 3, 266);
			lcdPrint(" ", 110, 266);
		}
		octSwitch = 0;
    }

    if (octSwitch2) {
		setColor(1, 130, 32);
		char hola[3];
		sprintf(hola, "%d", octCur);
		lcdPrint(hola, 110, 266);
		octSwitch2 = 0;
    }

    if (histSwitch) {
		if (histPrint) {
			setColor(0, 255, 0);
			setFont(SmallFont);
			lcdPrint("Bottom:Hist", 3, 290);
		}
		else {
			setColor(255,0,0);
			setFont(SmallFont);
			lcdPrint("Bottom:Hist", 3, 290);
			histBackground();
			setColor(239,230,213);
			drawHLine(160, 270, 80);
		}
		histSwitch = 0;
    }



    	// Write code to increment your interrupt counter here.
    	// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM



// 			Useful for Debugging, and understanding your Microblaze registers.
//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//
//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
//
//    	    // Expect to see 0x80000000 in GIER
//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;

}

/* Q_onAssert is called only when the program encounters an error*/
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* name of the file that throws the error */
    (void)line;                                   /* line of the code that throws the error */
    QF_INT_LOCK();
    printDebugLog();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/
void GpioHandler(void *CallbackRef) {
	int y = 0;
	// Increment A counter
	XGpio *GpioPtr = (XGpio *)CallbackRef;

	XGpio_InterruptClear(GpioPtr, BUTTON_CHANNEL);	// Clearing interrupt

	Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	Xuint32 finish = start;

	while (finish < (start + RESET_VALUE/10000)) {
		finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
		XGpio_InterruptClear(GpioPtr, BUTTON_CHANNEL);	// Clearing interrupt
	}

	while (y < 1000) {
		y++;
	}

	Xuint32 ButtonPressStatus = 0;
	ButtonPressStatus = XGpio_DiscreteRead(&btn, BUTTON_CHANNEL); // Check GPIO output
	if (ButtonPressStatus == 0x04) {
		//RIGHT
//		print("Button Pressed");
		//TimerFlag = 1;
		if (octLim) {
			if (octCur < 8) {
				octCur++;
				octSwitch2 = 1;
				xil_printf("Octave Incremented: %d\n\r", octCur);
			}
			else {
				xil_printf("Octave Limit Reached\n\r");
			}
		}
		else {
			xil_printf("Please enable octave limiting to change octave\n\r");
		}
		QActive_postISR((QActive *)&AO_Lab2A, BTNR);
	}
	else if (ButtonPressStatus == 0x02) {
		//LEFT
//		print("Button Pressed");
		//TimerFlag = 1;
		if (octLim) {
			if (octCur > 0) {
				octCur--;
				octSwitch2 = 1;
				xil_printf("Octave Decremented: %d\n\r", octCur);
			}
			else {
				xil_printf("Octave Limit Reached\n\r");
			}
		}
		else {
			xil_printf("Please enable octave limiting to change octave\n\r");
		}
		QActive_postISR((QActive *)&AO_Lab2A, BTNL);
	}
	else if (ButtonPressStatus == 0x10) {
		//CENTER
		//TimerFlag = 1;
		if (octLim == 0) {
			xil_printf("DEBUG: Octave Limiting Enabled\n\r");
			octLim = 1;
			octCur = 4;
			octSwitch = 1;
//			setColor(0, 255, 0);
//			setFont(SmallFont);
//			lcdPrint("Center:Octave", 3, 266);
		}
		else {
//			setColor(255, 0, 0);
//			setFont(SmallFont);
//			lcdPrint("Benter:Octave", 3, 266);
			xil_printf("DEBUG: Octave Limiting Disabled\n\r");
			octLim = 0;
			octSwitch = 1;
		}
		QActive_postISR((QActive *)&AO_Lab2A, BTNC);
	}
	else if (ButtonPressStatus == 0x01) {
		//Up
//		print("Button Pressed");
		//TimerFlag = 1;

		if (printFrequency == 1) {
			printFrequency = 0;
//			setColor(255, 0, 0);
//			setFont(SmallFont);
//			lcdPrint("Top:Frequency", 3, 242);
			fSwitch = 1;
			xil_printf("Frequency disabled!\n\r");
		}
		else {
			printFrequency = 1;
//			setColor(0, 255, 0);
//			setFont(SmallFont);
//			lcdPrint("Top:Frequency", 3, 242);
			fSwitch = 1;
			xil_printf("Frequency enabled!\n\r");
		}
		QActive_postISR((QActive *)&AO_Lab2A, BTNU);
	}
	else if (ButtonPressStatus == 0x08) {
		start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
		finish = start;

		while (finish < (start + RESET_VALUE/10000)) {
			finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
		}
		//Down
//		print("Button Pressed");
		//TimerFlag = 1;
//		BG_Fill_Loop();
//		if (pause == 1) {
//			xil_printf("Tuner unpaused!\n\r");
//			pause = 0;
//		}
//		else {
//			xil_printf("Tuner paused!\n\r");
//			pause = 1;
//			BG_Fill_Loop();
		if (histPrint == 0) {
			xil_printf("DEBUG: Histogram Mode Enabled\n\r");
			histPrint = 1;
			histSwitch = 1;
//			setFont(SmallFont);
//			setColor(0, 255, 0);
//			lcdPrint("Bottom:Hist", 3, 290);
		}
		else {
			xil_printf("DEBUG: Histogram Mode Disabled\n\r");
			histPrint = 0;
			histSwitch = 1;
//	    	histBackground();
//			setFont(SmallFont);
//			setColor(255, 0, 0);
//			lcdPrint("Bottom:Hist", 3, 290);
		}
		QActive_postISR((QActive *)&AO_Lab2A, BTND);
	}
}

void TwistHandler(void *CallbackRef) {
	//XGpio_DiscreteRead( &twist_Gpio, 1);
	XGpio *GpioPtr = (XGpio *)CallbackRef;
	Xuint32 encoderStatus = 0;

	Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	Xuint32 finish = start;

	while (finish < (start + RESET_VALUE/10000)) {
		finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	}

	encoderStatus = XGpio_DiscreteRead(&enc_gpio, 1);

	if (encoderStatus == 7) {
		state = S0;
		encoder_count = 4;
		a = 440;
		aSwitch = 1;
		xil_printf("A frequency reset: %d Hz\n\r", a);
		start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
		finish = start;

		while (finish < (start + RESET_VALUE/10000)) {
			finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
		}
		QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	}

	switch (state) {

		case S0: {
				if (encoder_count == 8 || encoder_count == 0) {
					encoder_count = 4;
				}
			switch(encoderStatus) {
				case 0b01: {
					if(encoder_count == 4) {
						encoder_count = encoder_count + 1;
						state = S1;
					}
					break;
				}
				case 0b10: {
					if(encoder_count == 4) {
						encoder_count = encoder_count - 1;
						state = S3;
					}
					break;
				}
				break;
			}
			break;
		}

		case S1: {
			switch(encoderStatus) {
				case 0b11: {
					if(encoder_count == 1) {
						encoder_count = encoder_count - 1;
						state = S0;
						a -= 1;
						aSwitch = 1;
						xil_printf("A frequency down: %d Hz\n\r", a);
						Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
						Xuint32 finish = start;

						while (finish < (start + RESET_VALUE/10000)) {
							finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
						}
						QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN);
					}
					break;
				}
				case 0b00: {
					if(encoder_count == 5) {
						encoder_count = encoder_count + 1;
						state = S2;
					}
					break;
				}
				break;
			}
			break;
		}

		case S2: {
			switch(encoderStatus) {
				case 0b01: {
					if (encoder_count == 2) {
						encoder_count = encoder_count - 1;
						state = S1;
					}
					break;
				}
				case 0b10: {
					if (encoder_count == 6) {
						encoder_count = encoder_count + 1;
						state = S3;
					}
					break;
				}
				break;
			}
			break;
		}

		case S3: {
			switch(encoderStatus) {
				case 0b00: {
					if(encoder_count == 3) {
						encoder_count = encoder_count - 1;
						state = S2;
					}
					break;
				}
				case 0b11: {
					if(encoder_count == 7) {
						encoder_count = encoder_count + 1;
						state = S0;
						a += 1;
						aSwitch = 1;
						xil_printf("A frequency up: %d Hz\n\r", a);
						Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
						Xuint32 finish = start;

						while (finish < (start + RESET_VALUE/10000)) {
							finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
						}
						QActive_postISR((QActive *)&AO_Lab2A, ENCODER_UP);
					}
					break;
				}
				break;
			}
			break;
		}
		break;
	}
	XGpio_InterruptClear(GpioPtr, GPIO_CHANNEL1);	// Clearing interrupt
}

void debounceTwistInterrupt(){
	// Read both lines here? What is twist[0] and twist[1]?
	// How can you use reading from the two GPIO twist input pins to figure out which way the twist is going?
}

void debounceInterrupt() {
	QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	// XGpio_InterruptClear(&sw_Gpio, GPIO_CHANNEL1); // (Example, need to fill in your own parameters
}
