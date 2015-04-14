/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "mkfilter.h"
#include "complex.h"

#define opt_be 0x00001	/* -Be		Bessel characteristic	       */
#define opt_bu 0x00002	/* -Bu		Butterworth characteristic     */
#define opt_ch 0x00004	/* -Ch		Chebyshev characteristic       */
#define opt_re 0x00008	/* -Re		Resonator		       */
#define opt_pi 0x00010	/* -Pi		proportional-integral	       */

#define opt_lp 0x00020	/* -Lp		lowpass			       */
#define opt_hp 0x00040	/* -Hp		highpass		       */
#define opt_bp 0x00080	/* -Bp		bandpass		       */
#define opt_bs 0x00100	/* -Bs		bandstop		       */
#define opt_ap 0x00200	/* -Ap		allpass			       */

#define opt_a  0x00400	/* -a		alpha value		       */

#define opt_o  0x01000	/* -o		order of filter		       */
#define opt_p  0x02000	/* -p		specified poles only	       */
#define opt_w  0x04000	/* -w		don't pre-warp		       */
#define opt_z  0x08000	/* -z		use matched z-transform	       */
#define opt_Z  0x10000	/* -Z		additional zero		       */

struct pzrep
{
	complex poles[MAXPZ];
	complex zeros[MAXPZ];
	int numpoles;
	int numzeros;
};

static pzrep splane, zplane;
static double raw_alphaz;
static uint options;
static double warped_alpha1, warped_alpha2;
static bool infq;
static uint polemask;

/* table produced by /usr/fisher/bessel --	N.B. only one member of each C.Conj. pair is listed */
static c_complex bessel_poles[] =
{
    { -1.00000000000e+00, 0.00000000000e+00}, { -1.10160133059e+00, 6.36009824757e-01},
    { -1.32267579991e+00, 0.00000000000e+00}, { -1.04740916101e+00, 9.99264436281e-01},
    { -1.37006783055e+00, 4.10249717494e-01}, { -9.95208764350e-01, 1.25710573945e+00},
    { -1.50231627145e+00, 0.00000000000e+00}, { -1.38087732586e+00, 7.17909587627e-01},
    { -9.57676548563e-01, 1.47112432073e+00}, { -1.57149040362e+00, 3.20896374221e-01},
    { -1.38185809760e+00, 9.71471890712e-01}, { -9.30656522947e-01, 1.66186326894e+00},
    { -1.68436817927e+00, 0.00000000000e+00}, { -1.61203876622e+00, 5.89244506931e-01},
    { -1.37890321680e+00, 1.19156677780e+00}, { -9.09867780623e-01, 1.83645135304e+00},
    { -1.75740840040e+00, 2.72867575103e-01}, { -1.63693941813e+00, 8.22795625139e-01},
    { -1.37384121764e+00, 1.38835657588e+00}, { -8.92869718847e-01, 1.99832584364e+00},
    { -1.85660050123e+00, 0.00000000000e+00}, { -1.80717053496e+00, 5.12383730575e-01},
    { -1.65239648458e+00, 1.03138956698e+00}, { -1.36758830979e+00, 1.56773371224e+00},
    { -8.78399276161e-01, 2.14980052431e+00}, { -1.92761969145e+00, 2.41623471082e-01},
    { -1.84219624443e+00, 7.27257597722e-01}, { -1.66181024140e+00, 1.22110021857e+00},
    { -1.36069227838e+00, 1.73350574267e+00}, { -8.65756901707e-01, 2.29260483098e+00},
};

//static double getfarg(char*);
//static int getiarg(char*);
//static void checkoptions();
//static void opterror(const char*, int = 0, int = 0);
static void setdefaults(filter_pass_t pass, double *alpha1, double *alpha2);
static void compute_s_bessel(int order, double ripple);
static void compute_s_butterworth(int order, double ripple);
static void compute_s_chebyshev(int order, double ripple);
static void choosepole(complex);
static void prewarp(double alpha1, double alpha2);
static void normalize(filter_pass_t pass);
static void normalize_low_pass();
static void normalize_high_pass();
static void normalize_band_pass();
static void normalize_band_stop();
static void compute_z_blt();
static complex blt(complex);
static void compute_z_mzt();
static void compute_notch(double alpha1, double qfactor);
static void compute_apres(double alpha1, double qfactor);
static complex reflect(complex);
static void compute_bpres(double alpha1, double qfactor);
static void add_extra_zero();
static void expandpoly(double alpha1, double alpha2, int *numzero, double xcoeffs[], int *numpole, double ycoeffs[], complex *dc_gain, complex *fc_gain, complex *hf_gain);
static void expand(complex[], int, complex[]), multin(complex, int, complex[]);
//static void printfilter(double alpha1, double alpha2, double xcoeffs[], double ycoeffs[], complex dc_gain, complex fc_gain, complex hf_gain);
//static void printgain(const char*, complex);
//static void printrat_s(), printrat_z(), printpz(complex*, int);
//static void printrecurrence(int numzero, double xcoeffs[], int numpole, double ycoeffs[]);
//static void prcomplex(complex);

int mkfilter(filter_type_t type,
			 filter_pass_t pass,
			 int order,
			 double alpha1,
			 double alpha2,
			 double ripple,
			 int *numzero,
			 double xcoeffs[],
			 int *numpole,
			 double ycoeffs[],
			 double *gain,
			 double qfactor)
{
	static complex dc_gain, fc_gain, hf_gain;

    setdefaults(pass, &alpha1, &alpha2);

	switch (type)
	{
	case BESSEL:
		compute_s_bessel(order, ripple);
		prewarp(alpha1, alpha2);
		normalize(pass);
		if (options & opt_z) compute_z_mzt();
		else compute_z_blt();
		break;

	case BUTTERWORTH:
		compute_s_butterworth(order, ripple);
		prewarp(alpha1, alpha2);
		normalize(pass);
		if (options & opt_z)
		{
			compute_z_mzt();
		}
		else
		{
			compute_z_blt();
		}
		break;

	case CHEBYSHEV:
		compute_s_chebyshev(order, ripple);
		prewarp(alpha1, alpha2);
		normalize(pass);
		if (options & opt_z)
		{
			compute_z_mzt();
		}
		else
		{
			compute_z_blt();
		}
		break;

	case RESONATOR:
		switch (pass)
		{
		case BAND_PASS:
			compute_bpres(alpha1, qfactor);	   /* bandpass resonator	 */
			break;
		case BAND_STOP:
			compute_notch(alpha1, qfactor);	   /* bandstop resonator (notch) */
			break;
		case ALL_PASS:
			compute_apres(alpha1, qfactor);	   /* allpass resonator		 */
			break;
        case LOW_PASS:
        case HIGH_PASS:
            break;
		}
		break;

	case PROPORTIONAL_INTEGRAL:
		prewarp(alpha1, alpha2);
		splane.poles[0] = 0.0;
		splane.zeros[0] = -TWOPI * warped_alpha1;
		splane.numpoles = splane.numzeros = 1;
		if (options & opt_z)
		{
			compute_z_mzt();
		}
		else
		{
			compute_z_blt();
		}
		break;
	}

	if (options & opt_Z) add_extra_zero();

    expandpoly(alpha1, alpha2, numzero, xcoeffs, numpole, ycoeffs, &dc_gain, &fc_gain, &hf_gain);

	switch (pass)
	{
	case LOW_PASS:
		*gain = hypot(dc_gain);
		break;
	case HIGH_PASS:
		*gain = hypot(hf_gain);
		break;
	case BAND_PASS:
	case ALL_PASS:
		*gain = hypot(fc_gain);
		break;
	case BAND_STOP:
		*gain = hypot(csqrt(dc_gain * hf_gain));
		break;
	default:
		*gain = 1.0;
		break;
	}

	return(1);
}
//
//#ifdef COMMENTED_OUT
//
//static void readcmdline(char *argv[])
//{
//	options = order = polemask = 0;
//    int ap = 0;
//    unless (argv[ap] == NULL) ap++; /* skip program name */
//    until (argv[ap] == NULL) {
//		uint m = decodeoptions(argv[ap++]);
//		if (m & opt_ch)	chebrip = getfarg(argv[ap++]);
//		if (m & opt_a)
//		{
//			raw_alpha1 = getfarg(argv[ap++]);
//			raw_alpha2 = (argv[ap] != NULL && argv[ap][0] != '-') ? getfarg(argv[ap++]) : raw_alpha1;
//		}
//		if (m & opt_Z) raw_alphaz = getfarg(argv[ap++]);
//		if (m & opt_o) order = getiarg(argv[ap++]);
//		if (m & opt_p)
//		{
//			while (argv[ap] != NULL && argv[ap][0] >= '0' && argv[ap][0] <= '9')
//			{
//				int p = atoi(argv[ap++]);
//				if (p < 0 || p > 31) p = 31; /* out-of-range value will be picked up later */
//				polemask |= (1 << p);
//			}
//		}
//		if (m & opt_re)
//		{
//			char *s = argv[ap++];
//			if (s != NULL && (strcmp(s,"Inf")==0))
//			{
//				infq = true;
//			}
//			else
//			{
//				qfactor = getfarg(s);
//				infq = false;
//			}
//		}
//		options |= m;
//	}
//}
//
//#endif
//
//static double getfarg(char *s)
//{
//    return atof(s);
//}

//static int getiarg(char *s)
//{
//    return atoi(s);
//}

//static bool optsok;
//
//static void checkoptions(int order)
//{
//	optsok = true;
//    if (options & opt_re)
//	{
//		unless (onebit(options & (opt_bp | opt_bs | opt_ap)))
//		{
//			opterror("must specify exactly one of -Bp, -Bs, -Ap with -Re");
//		}
//		if (options & (opt_lp | opt_hp | opt_o | opt_p | opt_w | opt_z))
//		{
//			opterror("can't use -Lp, -Hp, -o, -p, -w, -z with -Re");
//		}
//    }
//    else if (options & opt_pi)
//	{
//		if (options & (opt_lp | opt_hp | opt_bp | opt_bs | opt_ap))
//		{
//	  		opterror("-Lp, -Hp, -Bp, -Bs, -Ap illegal in conjunction with -Pi");
//		}
//		unless ((options & opt_o) && (order == 1)) opterror("-Pi implies -o 1");
//    }
//	else
//	{
//		unless (onebit(options & (opt_lp | opt_hp | opt_bp | opt_bs)))
//		{
//	  		opterror("must specify exactly one of -Lp, -Hp, -Bp, -Bs");
//		}
//		if (options & opt_ap)
//		{
//			opterror("-Ap implies -Re");
//		}
//		if (options & opt_o)
//		{
//			unless (order >= 1 && order <= MAXORDER) opterror("order must be in range 1 .. %d", MAXORDER);
//			if (options & opt_p)
//			{
//				uint m = (1 << order) - 1; /* "order" bits set */
//				if ((polemask & ~m) != 0)
//				{
//					opterror("order=%d, so args to -p must be in range 0 .. %d", order, order-1);
//				}
//			}
//
//		}
//		else
//		{
//			opterror("must specify -o");
//		}
//	}
//    unless (options & opt_a) opterror("must specify -a");
//    unless (optsok)	exit(1);
//}

//static void opterror(const char *msg, int p1, int p2)
//{
//	fprintf(stderr, "mkfilter: ");
//	fprintf(stderr, msg, p1, p2);
//	putc('\n', stderr);
//    optsok = false;
//}

static void setdefaults(filter_pass_t pass, double *alpha1, double *alpha2)
{
	unless (options & opt_p) polemask = ~0; /* use all poles */

	switch (pass)
	{
	case LOW_PASS:
	case HIGH_PASS:
	case ALL_PASS:
		*alpha2 = *alpha1;
		break;
    case BAND_PASS:
    case BAND_STOP:
        break;
	}
}

static void compute_s_bessel(int order, double ripple) /* compute S-plane poles for prototype LP filter */
{
	int i;
	int p;

	splane.numpoles = 0;

	p = (order*order)/4; /* ptr into table */
	if (order & 1) choosepole(bessel_poles[p++]);
	for (i = 0; i < order/2; i++)
	{
		choosepole(bessel_poles[p]);
		choosepole(cconj(bessel_poles[p]));
		p++;
	}
}

static void compute_s_butterworth(int order, double ripple)  /* compute S-plane poles for prototype LP filter */
{
	int i;

	splane.numpoles = 0;

	for (i = 0; i < 2*order; i++)
	{
		double theta = (order & 1) ? (i*PI) / order : ((i+0.5)*PI) / order;
		choosepole(expj(theta));
	}
}

static void compute_s_chebyshev(int order, double ripple)  /* compute S-plane poles for prototype LP filter */
{
	int i;

	splane.numpoles = 0;

	for (i = 0; i < 2*order; i++)
	{
		double theta = (order & 1) ? (i*PI) / order : ((i+0.5)*PI) / order;
		choosepole(expj(theta));
	}
	if (ripple >= 0.0)
	{
		fprintf(stderr, "mkfilter: Chebyshev ripple is %g dB; must be .lt. 0.0\n", ripple);
		exit(1);
	}
	double rip = pow(10.0, -ripple / 10.0);
	double eps = sqrt(rip - 1.0);
	double y = asinh(1.0 / eps) / double(order);
	if (y <= 0.0)
	{
		fprintf(stderr, "mkfilter: bug: Chebyshev y=%g; must be .gt. 0.0\n", y);
		exit(1);
	}
	for (int i = 0; i < splane.numpoles; i++)
	{
		splane.poles[i].re *= sinh(y);
		splane.poles[i].im *= cosh(y);
	}
}

static void choosepole(complex z)
{
	if (z.re < 0.0)
	{
		if (polemask & 1)
			splane.poles[splane.numpoles++] = z;
		polemask >>= 1;
	}
}

static void prewarp(double alpha1, double alpha2) { /* for bilinear transform, perform pre-warp on alpha values */
    if (options & (opt_w | opt_z))
	{
		warped_alpha1 = alpha1;
		warped_alpha2 = alpha2;
	}
	else
	{
		warped_alpha1 = tan(PI * alpha1) / PI;
		warped_alpha2 = tan(PI * alpha2) / PI;
	}
}


static void normalize(filter_pass_t pass)
{
	switch (pass)
	{
	case LOW_PASS:
		normalize_low_pass();
		break;
	case HIGH_PASS:
		normalize_high_pass();
		break;
	case BAND_PASS:
		normalize_band_pass();
		break;
	case BAND_STOP:
		normalize_band_stop();
		break;
    case ALL_PASS:
		break;
	}
}


static void normalize_low_pass()
{
	int i;

	double w1 = TWOPI * warped_alpha1;
	//double w2 = TWOPI * warped_alpha2;

	for (i = 0; i < splane.numpoles; i++)
	{
		splane.poles[i] = splane.poles[i] * w1;
	}
    splane.numzeros = 0;
}

static void normalize_high_pass()
{
	int i;

	double w1 = TWOPI * warped_alpha1;
	//double w2 = TWOPI * warped_alpha2;

	for (i=0; i < splane.numpoles; i++)
	{
		splane.poles[i] = w1 / splane.poles[i];
	}
	for (i=0; i < splane.numpoles; i++)
	{
		splane.zeros[i] = 0.0;	 /* also N zeros at (0,0) */
	}
	splane.numzeros = splane.numpoles;
}

static void normalize_band_pass()
{
	int i;
	double w0;
	double bw;

	double w1 = TWOPI * warped_alpha1;
	double w2 = TWOPI * warped_alpha2;

	w0 = sqrt(w1*w2);
	bw = w2-w1;
    for (i=0; i < splane.numpoles; i++)
	{
		complex hba = 0.5 * (splane.poles[i] * bw);
		complex temp = csqrt(1.0 - sqr(w0 / hba));
		splane.poles[i] = hba * (1.0 + temp);
		splane.poles[splane.numpoles+i] = hba * (1.0 - temp);
	}
	for (i=0; i < splane.numpoles; i++)
	{
		splane.zeros[i] = 0.0;	 /* also N zeros at (0,0) */
	}
	splane.numzeros = splane.numpoles;
	splane.numpoles *= 2;
}

static void normalize_band_stop()
{
	int i;
	double w0;
	double bw;

	double w1 = TWOPI * warped_alpha1;
	double w2 = TWOPI * warped_alpha2;

	w0 = sqrt(w1*w2);
	bw = w2-w1;
    for (i=0; i < splane.numpoles; i++)
	{
		complex hba = 0.5 * (bw / splane.poles[i]);
		complex temp = csqrt(1.0 - sqr(w0 / hba));
		splane.poles[i] = hba * (1.0 + temp);
		splane.poles[splane.numpoles+i] = hba * (1.0 - temp);
	}
	for (i=0; i < splane.numpoles; i++)   /* also 2N zeros at (0, +-w0) */
	{
		splane.zeros[i] = complex(0.0, +w0);
		splane.zeros[splane.numpoles+i] = complex(0.0, -w0);
	}
	splane.numpoles *= 2;
	splane.numzeros = splane.numpoles;
}

static void compute_z_blt()  /* given S-plane poles & zeros, compute Z-plane poles & zeros, by bilinear transform */
{
	int i;
    zplane.numpoles = splane.numpoles;
    zplane.numzeros = splane.numzeros;
    for (i=0; i < zplane.numpoles; i++)
	{
		zplane.poles[i] = blt(splane.poles[i]);
	}
	for (i=0; i < zplane.numzeros; i++)
	{
		zplane.zeros[i] = blt(splane.zeros[i]);
	}
	while (zplane.numzeros < zplane.numpoles)
	{
		zplane.zeros[zplane.numzeros++] = -1.0;
	}
}

static complex blt(complex pz)
{
	return (2.0 + pz) / (2.0 - pz);
}

static void compute_z_mzt()  /* given S-plane poles & zeros, compute Z-plane poles & zeros, by matched z-transform */
{
	int i;
	zplane.numpoles = splane.numpoles;
	zplane.numzeros = splane.numzeros;
	for (i=0; i < zplane.numpoles; i++)
	{
		zplane.poles[i] = cexp(splane.poles[i]);
	}
	for (i=0; i < zplane.numzeros; i++)
	{
		zplane.zeros[i] = cexp(splane.zeros[i]);
	}
 }

/* compute Z-plane pole & zero positions for bandstop resonator (notch filter) */
static void compute_notch(double alpha1, double qfactor)
{
	compute_bpres(alpha1, qfactor);		/* iterate to place poles */
	double theta = TWOPI * alpha1;
	complex zz = expj(theta);	/* place zeros exactly */
	zplane.zeros[0] = zz; zplane.zeros[1] = cconj(zz);
}

/* compute Z-plane pole & zero positions for allpass resonator */
static void compute_apres(double alpha1, double qfactor)
{
	compute_bpres(alpha1, qfactor);		/* iterate to place poles */
	zplane.zeros[0] = reflect(zplane.poles[0]);
	zplane.zeros[1] = reflect(zplane.poles[1]);
}

static complex reflect(complex z)
{
	double r = hypot(z);
	return z / sqr(r);
}

/* compute Z-plane pole & zero positions for bandpass resonator */
static void compute_bpres(double alpha1, double qfactor)
{
	zplane.numpoles = zplane.numzeros = 2;
	zplane.zeros[0] = 1.0;
	zplane.zeros[1] = -1.0;
    double theta = TWOPI * alpha1; /* where we want the peak to be */
    if (infq)
	{
		/* oscillator */
		complex zp = expj(theta);
		zplane.poles[0] = zp;
		zplane.poles[1] = cconj(zp);
    }
	/* must iterate to find exact pole positions */
	else
	{
		complex topcoeffs[MAXPZ+1];
		expand(zplane.zeros, zplane.numzeros, topcoeffs);
		double r = exp(-theta / (2.0 * qfactor));
		double thm = theta, th1 = 0.0, th2 = PI;
		bool cvg = false;
		for (int i=0; i < 50 && !cvg; i++)
		{
			complex zp = r * expj(thm);
			zplane.poles[0] = zp; zplane.poles[1] = cconj(zp);
			complex botcoeffs[MAXPZ+1];
			expand(zplane.poles, zplane.numpoles, botcoeffs);
			complex g = evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, expj(theta));
			double phi = g.im / g.re; /* approx to atan2 */
			if (phi > 0.0)
			{
				th2 = thm;
			}
			else
			{
				th1 = thm;
			}
			if (fabs(phi)< EPS) cvg = true;
			thm = 0.5 * (th1+th2);
		}
		unless (cvg) fprintf(stderr, "mkfilter: warning: failed to converge\n");
	}
}

static void add_extra_zero()
{
	if (zplane.numzeros+2 > MAXPZ)
	{
		fprintf(stderr, "mkfilter: too many zeros; can't do -Z\n");
		exit(1);
	}
	double theta = TWOPI * raw_alphaz;
	complex zz = expj(theta);
	zplane.zeros[zplane.numzeros++] = zz;
	zplane.zeros[zplane.numzeros++] = cconj(zz);
	while (zplane.numpoles < zplane.numzeros)
	{
		zplane.poles[zplane.numpoles++] = 0.0;	 /* ensure causality */
	}
}

/* given Z-plane poles & zeros, compute top & bot polynomials in Z, and then recurrence relation */
static void expandpoly(double alpha1, double alpha2, int *numzero, double xcoeffs[], int *numpole, double ycoeffs[], complex *dc_gain, complex *fc_gain, complex *hf_gain)
{
	complex topcoeffs[MAXPZ+1], botcoeffs[MAXPZ+1]; int i;
	expand(zplane.zeros, zplane.numzeros, topcoeffs);
	expand(zplane.poles, zplane.numpoles, botcoeffs);
	*dc_gain = evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, 1.0);
	double theta = TWOPI * 0.5 * (alpha1 + alpha2); /* "jwT" for centre freq. */
	*fc_gain = evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, expj(theta));
	*hf_gain = evaluate(topcoeffs, zplane.numzeros, botcoeffs, zplane.numpoles, -1.0);
	for (i = 0; i <= zplane.numzeros; i++)
	{
		xcoeffs[i] = +(topcoeffs[i].re / botcoeffs[zplane.numpoles].re);
	}
	*numzero = zplane.numzeros;
	for (i = 0; i <= zplane.numpoles; i++)
	{
		ycoeffs[i] = -(botcoeffs[i].re / botcoeffs[zplane.numpoles].re);
	}
	*numpole = zplane.numpoles;
}

static void expand(complex pz[], int npz, complex coeffs[]) {
	/* compute product of poles or zeros as a polynomial of z */
	int i;
	coeffs[0] = 1.0;
	for (i=0; i < npz; i++)
	{
		coeffs[i+1] = 0.0;
	}
	for (i=0; i < npz; i++)
	{
		multin(pz[i], npz, coeffs);
	}
	/* check computed coeffs of z^k are all real */
	for (i=0; i < npz+1; i++)
	{
		if (fabs(coeffs[i].im) > EPS)
		{
			fprintf(stderr, "mkfilter: coeff of z^%d is not real; poles/zeros are not complex conjugates\n", i);
			exit(1);
		}
	}
}

static void multin(complex w, int npz, complex coeffs[]) {
	/* multiply factor (z-w) into coeffs */
	complex nw = -w;
	for (int i = npz; i >= 1; i--)
	{
		coeffs[i] = (nw * coeffs[i]) + coeffs[i-1];
	}
	coeffs[0] = nw * coeffs[0];
}
/*
static void printfilter(double alpha1, double alpha2, int numzero, double xcoeffs[], int numpole, double ycoeffs[], complex dc_gain, complex fc_gain, complex hf_gain)
{
	printf("raw alpha1    = %14.10f\n", alpha1);
    printf("raw alpha2    = %14.10f\n", alpha2);
    unless (options & (opt_re | opt_w | opt_z))
	{
		printf("warped alpha1 = %14.10f\n", warped_alpha1);
		printf("warped alpha2 = %14.10f\n", warped_alpha2);
	}
	printgain("dc    ", dc_gain);
	printgain("centre", fc_gain);
	printgain("hf    ", hf_gain);
	putchar('\n');
	unless (options & opt_re) printrat_s();
	printrat_z();
	printrecurrence(numzero, xcoeffs, numpole, ycoeffs);
}
*/
//static void printgain(const char *str, complex gain)
//{
//	double r = hypot(gain);
//	printf("gain at %s:   mag = %15.9e", str, r);
//	if (r > EPS) printf("   phase = %14.10f pi", atan2(gain) / PI);
//	putchar('\n');
//}

//static void printrat_s()  /* print S-plane poles and zeros */
//{
//	printf("S-plane zeros:\n");
//	printpz(splane.zeros, splane.numzeros);
//	printf("S-plane poles:\n");
//	printpz(splane.poles, splane.numpoles);
//}
//
//static void printrat_z()  /* print Z-plane poles and zeros */
//{
//	printf("Z-plane zeros:\n");
//	printpz(zplane.zeros, zplane.numzeros);
//	printf("Z-plane poles:\n");
//	printpz(zplane.poles, zplane.numpoles);
//}
//
//static void printpz(complex *pzvec, int num)
//{
//	int n1 = 0;
//	while (n1 < num)
//	{
//		putchar('\t');
//		prcomplex(pzvec[n1]);
//		int n2 = n1+1;
//		while (n2 < num && pzvec[n2] == pzvec[n1])
//		{
//			n2++;
//		}
//		if (n2-n1 > 1) printf("\t%d times", n2-n1);
//		putchar('\n');
//		n1 = n2;
//	}
//	putchar('\n');
//}

//
//static void printrecurrence(int numzero, double xcoeffs[], int numpole, double ycoeffs[])  /* given (real) Z-plane poles & zeros, compute & print recurrence relation */
//{
//	printf("Recurrence relation:\n");
//	printf("y[n] = ");
//	int i;
//	for (i = 0; i < numzero+1; i++)
//	{
//		if (i > 0) printf("     + ");
//		double x = xcoeffs[i];
//		double f = fmod(fabs(x), 1.0);
//		const char *fmt = (f < EPS || f > 1.0-EPS) ? "%3g" : "%14.10f";
//		putchar('(');
//		printf(fmt, x);
//		printf(" * x[n-%2d])\n", numzero-i);
//	}
//    putchar('\n');
//    for (i = 0; i < numpole; i++)
//	{
//		printf("     + (%14.10f * y[n-%2d])\n", ycoeffs[i], numpole-i);
//	}
//    putchar('\n');
//}

//static void prcomplex(complex z)
//{
//	printf("%14.10f + j %14.10f", z.re, z.im);
//}
