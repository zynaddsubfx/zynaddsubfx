#include <math.h>
#include "JilesAtherton.h"

namespace zyn {
    
// Constructor.
JilesAtherton::JilesAtherton(float _Hc, float _Mr) {
    Hc = _Hc;
    Mr = _Mr;
    H = 0.0f;
    //~ updateParameters();
}

// Set the coercive field.
void JilesAtherton::setHc(float _Hc) {
    Hc = _Hc;
    //~ updateParameters();
}

// Set the remanence magnetization.
void JilesAtherton::setMr(float _Mr) {
    Mr = _Mr;
    //~ updateParameters();
}

inline float JilesAtherton::tanhX(const float x)
{
    // Pade approximation of tanh(x) bound to [-1 .. +1]
    // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
    const float x2 = x*x;
    return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2));
}

// Get the magnetization as a function of the applied field.
float JilesAtherton::applyHysteresis(const float _H) {
    if (_H != H) {
      dH = _H - H;
      H = _H;
    }
    
    // effective field strength (Heff) takes into consideration both 
    // the magnetic field intensity and the magnetisation level (M)
    //~ const float c = 2.0f
    const float Heff = H + alpha * M;
    const float Man = tanhX(Heff);
    const float delta = (dH>0.0f?1.0f:-1.0f);
    const float dM = dH/(1+Mr)*(Man-M)/(delta*Hc-(Man-M))+Mr/(1.0f+Mr) * Man-M;
    
    M += dM;
    
    return M;
    
    //~ // Calculate the Brillouin function.
    //~ const float Bpos = tanhX(H-Hc);
    //~ const float Bneg = tanhX(H+Hc);
    //~ B *= 0.99f;
    //~ const float dB = 0.01f * (dH>0.0f == B>0.0f ? Bpos : Bneg );
    //~ B += ( B>0.0f ? dB : -dB);
    //~ //float B = Ms * (1.0f - exp(-alpha * H / (dH>0.0f ? Hc : -Hc)));
    //~ // Calculate the magnetization.
    //~ //float M = (B / (1.0f + beta * B)/;
    //~ // Return the magnetization.
    //~ return -B /(1.5f + Mr); // gain compensation for convenient usage
}

//~ inline float JilesAtherton::getMagnetization(float _H) {
    //~ return Ms * (1.0f - exp(-alpha * H / Hc));
//~ }

//~ float JilesAtherton::Brillouin(float x) {
    //~ const float MrOverMs = Mr / Ms;
    //~ const float logMrOverMs = log(MrOverMs);
    //~ alpha = 2.0f * Hc / (Ms * logMrOverMs);// + H * beta);
    //~ beta = (1.0f - MrOverMs) / (Hc * logMrOverMs);// + H * beta);
//~ }

//~ void JilesAtherton::init() {
    //~ alpha = 0.0f;
    //~ beta = 0.0f;
    //~ updateParameters();
//~ }

}
