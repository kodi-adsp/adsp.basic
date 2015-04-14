#ifndef __FILTER_H
#define __FILTER_H

#define NUM_FILTER_TYPE 3  // 5 TBD
#define NUM_FILTER_PASS 4	// 5 TBD
#define NUM_FILTER_ORDER 10

#define FILTER_RIPPLE_SCALE 10
#define FILTER_RIPPLE_MIN (-10 * FILTER_RIPPLE_SCALE)
#define FILTER_RIPPLE_MAX (-1)

#define OUTPUT_GAIN_SCALE 10
#define OUTPUT_GAIN_MIN (-40 * OUTPUT_GAIN_SCALE)
#define OUTPUT_GAIN_MAX (20 * OUTPUT_GAIN_SCALE)

class Cfilter
{
#define MAXSWAPBUFFER 2

private:
	int m_SwapIndex;
	double m_Gain[MAXSWAPBUFFER];
	int r_NumZero[MAXSWAPBUFFER];
	int r_NumPole[MAXSWAPBUFFER];
	double m_X[MAXSWAPBUFFER][MAXPZ+1], m_Y[MAXSWAPBUFFER][MAXPZ+1];
	double m_XCoeff[MAXSWAPBUFFER][MAXPZ+1], m_YCoeff[MAXSWAPBUFFER][MAXPZ+1];

public:
	Cfilter();
	~Cfilter();

	bool Config(unsigned int nzero, double *xcoeff, unsigned int npole, double *ycoeff, double pbgain);
	double GetNext(double in);

	double GetGain(void);
	unsigned int GetNZero(void);
	double * GetXCoeff(void);
	unsigned int GetNPole(void);
	double * GetYCoeff(void);
};

#endif
