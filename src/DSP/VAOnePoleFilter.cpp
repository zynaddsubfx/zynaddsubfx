#include "VAOnePoleFilter.h"
#include <math.h>

#define  pi 3.1415926535897932384626433832795

CVAOnePoleFilter::CVAOnePoleFilter(void)
{
	m_fAlpha = 1.0;
	m_fBeta = 1.0;

	m_uFilterType = LPF1;
	reset();
}

CVAOnePoleFilter::~CVAOnePoleFilter(void)
{
}

// do the filter
double CVAOnePoleFilter::doFilter(double xn)
{
    // calculate v(n)
    double vn = (xn - m_fZ1)*m_fAlpha;

    // form LP output
    double lpf = vn + m_fZ1;

    // do the HPF
    double hpf = xn - lpf;

    // update memory
    m_fZ1 = vn + lpf;

    if(m_uFilterType == LPF1)
        return lpf;
    else if(m_uFilterType == HPF1)
        return hpf;

    // default
    return lpf;

}


