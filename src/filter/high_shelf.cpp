#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "high_shelf.h"

#define MIN_FREQ              20
#define MAX_FREQ           20000
#define Q_MIN              0.001
#define Q_MAX                1.0
#define DBGAIN_MIN           6.0
#define DBGAIN_MAX          24.0
#define DBGAIN_SCALE         5.0
#define Q_SCALE             32.0

#if !defined(M_PI) && defined(TARGET_WINDOWS)
  #define _USE_MATH_DEFINES 
  #include <cmath>
#endif

chighShelf::chighShelf(unsigned long Sample_Rate, unsigned long freq, float gain_, float pitch, float reso, float reso_gain) {
    SampleRate = Sample_Rate;

    gain        = gain_;
    freq_ofs    = freq;
    freq_pitch  = pitch;
    reso_ofs    = reso;
    dBgain_ofs  = reso_gain;

    for (unsigned i = 0; i < 4; i++) {
        buf[i] = 0;
    }
}

chighShelf::~chighShelf() {

}

void chighShelf::Run(unsigned long SampleCount, float *input, float *output) {

    double f0, q0, f, q, pi2_rate, A, dBgain, iv_beta;
    double iv_sin, iv_cos, iv_alpha, inv_a0, a0, a1, a2, b0, b1, b2;
    double freq_pitch_calc;

    freq_pitch_calc = (freq_pitch > 0) ? 1.0 + freq_pitch / 2.0 : 1.0 / (1.0 - freq_pitch / 2.0);

    pi2_rate = 2.0 * M_PI / SampleRate;
    f0 = freq_ofs;
    q0 = reso_ofs;

    f = f0 * freq_pitch_calc;
    if (f > MAX_FREQ) f = MAX_FREQ;
    q = q0;
    dBgain = dBgain_ofs;
    iv_sin = sin(pi2_rate * f);
    iv_cos = cos(pi2_rate * f);
    iv_alpha = iv_sin/(Q_SCALE * q);
    A = exp(dBgain/40.0 * log(10.0));
    iv_beta = sqrt(A) / q;
    b0 = A * (A + 1.0 + (A - 1.0) * iv_cos + iv_beta * iv_sin);
    b1 = -2.0 * A * (A - 1.0 + (A + 1.0) * iv_cos);
    b2 = A * (A + 1.0 + (A - 1.0) * iv_cos - iv_beta * iv_sin);
    a0 = A + 1.0 - (A - 1.0) * iv_cos + iv_beta * iv_sin;
    a1 = 2.0 * (A - 1.0 - (A + 1.0) * iv_cos);
    a2 = A + 1.0 - (A - 1.0) * iv_cos - iv_beta * iv_sin;
    inv_a0 = 1.0 / a0;
    for (unsigned i = 0; i < SampleCount; i++) {
        buf[1] = buf[0];
        buf[0] = input[i];
        buf[3] = buf[2];
        output[i] = inv_a0 * (gain * (b0 * input[i] + b1 * buf[0] + b2 * buf[1])
                             - a1 * buf[2] - a2 * buf[3]);
        buf[2] = output[i];
    }
    return;
}
