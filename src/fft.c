#include "fft.h"
#include "complex.h"
#include "trig.h"

static float new_[4096];
static float new_im[4096];


float fft(float* q, float* w, int n, int m, float sample_f) {
	int a,b,r,d,e,c;
	int k,place;
	a=n/2;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// ORdering algorithm
	for(i=0; i<(m-1); i++){
		d=0;
		for (j=0; j<b; j++){
			for (c=0; c<a; c++){	
				e=c+d;
				new_[e]=q[(c*2)+d];
				new_im[e]=w[(c*2)+d];
				new_[e+a]=q[2*c+1+d];
				new_im[e+a]=w[2*c+1+d];
			}
			d+=(n/b);
		}		
		for (r=0; r<n;r++){
			q[r]=new_[r];
			w[r]=new_im[r];
		}
		b*=2;
		a=n/(2*b);
	}
	//end ordering algorithm

	b=1;
	k=0;
	int maxb = b;
	int maxk = k;
	for (j=0; j<m; j++){
	//MATH
		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			real=mult_real(q[i+1], w[i+1], get_cos(k,b), get_sin(k,b));
			imagine=mult_im(q[i+1], w[i+1], get_cos(k,b), get_sin(k,b));
//			real=mult_real(q[i+1], w[i+1], cosine(-PI*k/b), sine(-PI*k/b));
//			imagine=mult_im(q[i+1], w[i+1], cosine(-PI*k/b), sine(-PI*k/b));
			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;
//			if (k > maxk) { maxk = k; }
//			if (b > maxb) { maxb = b; }
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END MATH

	//REORDER
		for (i=0; i<n/2; i++){
			new_[i]=q[2*i];
			new_[i+(n/2)]=q[2*i+1];
			new_im[i]=w[2*i];
			new_im[i+(n/2)]=w[2*i+1];
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END REORDER	
		if (k > maxk) { maxk = k; }
		if (b > maxb) { maxb = b; }
		b*=2;
		k=0;		
	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<(n/2);i++) {
		new_[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}

	float s=sample_f/n; //spacing of bins
	
	frequency = (sample_f/n)*place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;
	
	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

//	xil_printf("%d, %d", maxk, maxb);
	return frequency;
}

float fftHist(float* q, float* w, int n, int m, float sample_f) {
	int a,b,r,d,e,c;
	int k,place;
	a=n/2;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;
	int maxk;
	int maxb;

	// ORdering algorithm
	for(i=0; i<(m-1); i++){
		d=0;
		for (j=0; j<b; j++){
			for (c=0; c<a; c++){
				e=c+d;
				new_[e]=q[(c*2)+d];
				new_im[e]=w[(c*2)+d];
				new_[e+a]=q[2*c+1+d];
				new_im[e+a]=w[2*c+1+d];
			}
			d+=(n/b);
		}
		for (r=0; r<n;r++){
			q[r]=new_[r];
			w[r]=new_im[r];
		}
		b*=2;
		a=n/(2*b);
	}
	//end ordering algorithm

	b=1;
	k=0;
	maxk = k;
	maxb = b;
	for (j=0; j<m; j++){
	//MATH
		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			real=mult_real(q[i+1], w[i+1], get_cos(k,b), get_sin(k,b));
			imagine=mult_im(q[i+1], w[i+1], get_cos(k,b), get_sin(k,b));
//			real=mult_real(q[i+1], w[i+1], cosine(-PI*k/b), sine(-PI*k/b));
//			imagine=mult_im(q[i+1], w[i+1], cosine(-PI*k/b), sine(-PI*k/b));
			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;
//			if (k > maxk) { maxk = k; }
//			if (b > maxb) { maxb = b; }
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END MATH

	//REORDER
		for (i=0; i<n/2; i++){
			new_[i]=q[2*i];
			new_[i+(n/2)]=q[2*i+1];
			new_im[i]=w[2*i];
			new_im[i+(n/2)]=w[2*i+1];
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END REORDER
		b*=2;
		k=0;
	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<(n/2);i++) {
		new_[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}

	float s=sample_f/n; //spacing of bins

	frequency = (sample_f/n)*place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

	int bin1 = 0;
	int bin2 = 0;
	int bin3 = 0;
	int bin4 = 0;
	int bin5 = 0;
	int bin6 = 0;
	int bin7 = 0;
	int bin8 = 0;
	int bin9 = 0;
	int bin10 = 0;
	int maxval = new_[0];

	for (int z = 0; z < (n/2); z++) {
		if (new_[z] > maxval) {maxval = new_[z];}
		if ((new_[z] > 0) && (new_[z] < 0.05)) { bin1++; }
		else if ((new_[z] >= 0.05) && (new_[z] < .1)) { bin2++; }
		else if ((new_[z] >= .1) && (new_[z] < .15)) { bin3++; }
		else if ((new_[z] >= .15) && (new_[z] < .2)) { bin4++; }
		else if ((new_[z] >= .2) && (new_[z] < .25)) { bin5++; }
		else if ((new_[z] >= .25) && (new_[z] < .3)) { bin6++; }
		else if ((new_[z] >= .3) && (new_[z] < .35)) { bin7++; }
		else if ((new_[z] >= .35) && (new_[z] < .4)) { bin8++; }
		else if ((new_[z] >= .4) && (new_[z] < .45)) { bin9++; }
		else { bin10++; }
	}

	xil_printf("Bin 1: %d, Bin 2: %d, Bin 3: %d, Bin 4: %d, Bin 5: %d\n\r", bin1, bin2, bin3, bin4, bin5);
	xil_printf("Bin 6: %d, Bin 7: %d, Bin 8: %d, Bin 9: %d, Bin 10: %d\n\r", bin6, bin7, bin8, bin9, bin10);
//	xil_printf("Min: %d Max: %d", (int)(minz * 1000), (int)(maxz));
	xil_printf("Max f: %d Hz\n\r\n\r", (int)maxval);

	drawHist(bin1, bin2, bin3, bin4, bin5, bin6, bin7, bin8, bin9, bin10, (int)maxval);

	return frequency;
}
