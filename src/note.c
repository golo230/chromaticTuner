#include "note.h"
#include "lcd.h"
#include <math.h>

//array to store note names for findNote
static char notes[12][3]={"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
static char octaves[10][3] = {"0 ", "1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 "};
static char centnums[51][3] = {"0 ", "1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ",
		"10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
		"20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50"
};

//finds and prints note of frequency and deviation from note
void findNote(float f, int a) {
	float c = 0.5946 * a;
//	float c=261.63;
	float r;
	float temp;
	float cents;
	int n;
	int oct=4;
	int note=0;
	int neg;
	//determine which octave frequency is in
	if(f >= c) {
		while(f > c*2) {
			c=c*2;
			oct++;
		}
	}
	else { //f < C4
		while(f < c) {
			c=c/2;
			oct--;
		}
	
	}

	//find note below frequency
	//c=middle C
	r=c*root2;
	while(f > r) {
		c=c*root2;
		r=r*root2;
		note++;
	}
//	setColor(0, 0, 0);
//	fillRect(70, 90, 170, 110);
//	setColor(231, 50, 19);
//	fillRect(((int)(f) % 100)+69, 90, ((int)(f) % 100)+70, 110);


	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
//		WriteString("N:");
//		WriteString(notes[note]);
//		WriteInt(oct);
//		WriteString(" D:+");
//		WriteInt((int)(f-c+.5));
//		WriteString("Hz");
	}
	else { //f closer to right note
		note++;
		if(note >=12) {
			note=0;
			oct++;
		}
//		WriteString("N:");
//		WriteString(notes[note]);
//		WriteInt(oct);
//		WriteString(" D:-");
//		WriteInt((int)(r-f+.5));
//		WriteString("Hz");
	}
	n = 12 * (oct - 1) + (note + 1) + 3;
	temp = powf(2.0, ((n-49)/12.0));
	float temp2 = temp * (float)(a);
	cents = 1200 * (log(f / temp2) / log(2));

	if ((cents >= 0.8) && (cents < 1.0)) {
		cents = 1.0;
	}

	neg = 0;
//	xil_printf("Cents: %d\n\r", (int)(cents));
	OnScreenText(notes[note]);
	OctaveText(octaves[oct]);

	if (cents < 0) {
		cents = cents * -1;
		CentText(centnums[(int)cents], cents);
		CentBar((int)(cents * -1));
	}
	else {
		CentText2(centnums[(int)cents], cents);
		CentBar((int)cents);
	}

//	freqText(centnums[(int)cents]);


//	printf(notes[oct][note]);
//	return notes[oct][note];
}
