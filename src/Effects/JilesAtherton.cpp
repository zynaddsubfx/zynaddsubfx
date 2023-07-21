#include <math.h>
#include "JilesAtherton.h"

namespace zyn {
    
// Constructor.
JilesAtherton::JilesAtherton(float _Hc, float _Mr) {
    Hc = _Hc;
    Mr = _Mr;
    H = 0.0f;
    updateParameters();
}

// Set the coercive field.
void JilesAtherton::setHc(float _Hc) {
    Hc = _Hc;
    updateParameters();
}

// Set the remanence magnetization.
void JilesAtherton::setMr(float _Mr) {
    Mr = _Mr;
    updateParameters();
}

// Get the magnetization as a function of the applied field.
float JilesAtherton::getMagnetization(float _H) {
    if (_H != H) {
      H = _H;
      updateParameters();
    }
    // Calculate the Brillouin function.
    float B = Ms * (1.0f - exp(-alpha * H / Hc));
    // Calculate the magnetization.
    float M = B / (1.0f + beta * B);
    // update state
    updateParameters();
    // Return the magnetization.
    return -M /(1.5f + Mr);
}

void JilesAtherton::updateParameters() {
    const float logMrOverMs = log(Mr / Ms);
    alpha = 2 * Hc / (Ms * logMrOverMs + H * beta);
    beta = (1 - Mr / Ms) / (Hc * logMrOverMs + H * beta);
}

void JilesAtherton::init() {
    alpha = 0.0f;
    beta = 0.0f;
    updateParameters();
}

}
