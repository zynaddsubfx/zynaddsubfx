#ifndef PREISACH_H
#define PREISACH_H

class PreisachModel {
public:
    // Constructor to initialize the PreisachModel with given parameters
    PreisachModel(float coercivity, float remanence, float alpha=0.1f, float beta=-0.02f);

    // Apply magnetic hysteresis using the PreisachModel
    float applyHysteresis(float audio_signal);
    
    // Setter functions to set the remanence and coercivity values
    void setMr(float remanence);
    void setHc(float coercivity);
    void setAlpha(float alpha);
    void setBeta(float beta);

private:
    float drive_;          // The driving force or saturation magnetization in the Preisach model
    float alpha_;          // Parameter affecting the spread of the hysteresis loop
    float beta_;           // Parameter affecting the symmetry of the hysteresis loop
    float coercivity_;     // Coercivity of the magnetic material
    float remanence_;      // Remanence of the magnetic material
    float prev_output_;    // Previous output field used in Preisach calculation
    float prev_input_;     // Previous input field used in Preisach calculation
};

#endif // PREISACH_H
