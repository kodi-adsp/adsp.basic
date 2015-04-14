#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mkfilter.h"
#include "filter.h"

Cfilter::Cfilter()
{
	m_SwapIndex = 0;
	m_Gain[m_SwapIndex] = 1.0;
	r_NumZero[m_SwapIndex] = 0;
	r_NumPole[m_SwapIndex] = 0;
	m_X[m_SwapIndex][0] = 0.0;
	m_Y[m_SwapIndex][0] = 0.0;
}

Cfilter::~Cfilter()
{
}


bool Cfilter::Config(unsigned int nzero, double *xcoeff, unsigned int npole, double *ycoeff, double gain)
{
	unsigned int i;

	if ((nzero>=MAXPZ) || (npole>=MAXPZ)) return 1;

	int nextSwapIndex = (m_SwapIndex+1)%MAXSWAPBUFFER;
	m_Gain[nextSwapIndex] = gain;
	r_NumZero[nextSwapIndex] = nzero;
	r_NumPole[nextSwapIndex] = npole;

	for(i=0 ; i<=nzero ; i++)
	{
		m_X[nextSwapIndex][i] = 0.0;
	}
	for(i=0 ; i<=npole ; i++)
	{
		m_Y[nextSwapIndex][i] = 0.0;
	}

	for(i=0 ; i<=nzero ; i++)
	{
		m_XCoeff[nextSwapIndex][i] = xcoeff[i];
	}
	for(i=0 ; i<=npole ; i++) // Use to be less than now less than or equal, for response ploting
	{
		m_YCoeff[nextSwapIndex][i] = ycoeff[i];
	}

	m_SwapIndex = nextSwapIndex;
	return 0;
}

double Cfilter::GetGain(void)
{
	return m_Gain[m_SwapIndex];
}

unsigned int Cfilter::GetNZero(void)
{
	return r_NumZero[m_SwapIndex];
}

double * Cfilter::GetXCoeff(void)
{
	return m_XCoeff[m_SwapIndex];
}

unsigned int Cfilter::GetNPole(void)
{
	return r_NumPole[m_SwapIndex];
}

double * Cfilter::GetYCoeff(void)
{
	return m_YCoeff[m_SwapIndex];
}

double Cfilter::GetNext(double in)
{
	int i;
	int SwapIndex = m_SwapIndex;

	for(i=0 ; i<r_NumZero[SwapIndex] ; i++)
	{
		m_X[SwapIndex][i] = m_X[SwapIndex][i+1];
	}
	m_X[SwapIndex][r_NumZero[SwapIndex]] = in / m_Gain[SwapIndex];
	for(i=0 ; i<r_NumPole[SwapIndex] ; i++)
	{
		m_Y[SwapIndex][i] = m_Y[SwapIndex][i+1];
	}

	//double a = m_X[SwapIndex][r_NumZero[SwapIndex]];
	double a = 0.0;
	for (i=0; i<=r_NumZero[SwapIndex]; i++)
	{
		a += m_XCoeff[SwapIndex][i]*m_X[SwapIndex][i];
	}
	for (i=0; i<r_NumPole[SwapIndex]; i++)
	{
		a += m_YCoeff[SwapIndex][i]*m_Y[SwapIndex][i];
	}
	m_Y[SwapIndex][r_NumPole[SwapIndex]] = a;

	return (a);
}
