#ifndef __HIGH_SHELF_H
#define __HIGH_SHELF_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

class chighShelf {
public:

private:
    unsigned long   SampleRate;
    float           gain;
    unsigned long   freq_ofs;
    float           freq_pitch;
    float           reso_ofs;
    float           dBgain_ofs;

    double buf[4];


public:
    chighShelf(unsigned long Sample_Rate, unsigned long freq, float gain_, float pitch, float reso, float reso_gain);
    ~chighShelf();
    void Run(unsigned long SampleCount, float *input, float *output);
};

#endif //__HIGH_SHELF_H
