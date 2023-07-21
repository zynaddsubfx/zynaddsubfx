// Jiles-Atherton model header file.

#ifndef JILES_ATHERTON_MODEL_H
#define JILES_ATHERTON_MODEL_H

namespace zyn {

class JilesAtherton {
public:
  // Constructor.
  JilesAtherton(float Hc, float Mr);

  // Set the coercive field.
  void setHc(float Hc);

  // Set the remanence magnetization.
  void setMr(float Mr);

  // Get the magnetization as a function of the applied field.
  float getMagnetization(float H);
  
  //
  void init(void);

private:

  void updateParameters();
  // The coercive field.
  // This is the strength of the applied field required to reduce the magnetization to zero.
  float Hc;

  // The saturation magnetization.
  // This is the maximum magnetization of the material.
  float Ms = 1.0;

  // The remanence magnetization.
  // This is the magnetization of the material when the applied field is zero.
  float Mr;

  // alpha and beta parameters implicitely store the state of the magnetization.
  // The beta parameter controls the width of the hysteresis loop.
  float beta;

  // The alpha parameter controls the steepness of the hysteresis loop.
  float alpha;
  
  // store last magnetic field
  float H;
};

}

#endif  // JILES_ATHERTON_MODEL_H
