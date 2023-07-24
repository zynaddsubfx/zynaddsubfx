#include <math.h>
#include "PreisachHysteresis.h"

// Constructor to initialize the PreisachModel with given parameters
PreisachModel::PreisachModel(float alpha, float beta, float coercivity, float remanence)
    : alpha_(alpha), beta_(beta), coercivity_(coercivity), remanence_(remanence),
      prev_output_(0.0), prev_input_(0.0) {}

// Apply magnetic hysteresis using the PreisachModel
float PreisachModel::applyHysteresis(float input) {



        // Preisach function with coercivity and remanence effects
        float output_field;
        
        if (input >= prev_input_) {
            output_field = alpha_ * (input - prev_input_);
        } else if (fabs(input) <= remanence_) {
            output_field = 0.0; // Output is set to zero when |input_field| is less than or equal to remanence
        } else {
            output_field = beta_ * (input - prev_input_);// + coercivity_;
        }

        // Update the previous input and output for the next iteration
        prev_input_ = input;
        prev_output_ += output_field;

        // Scale the output back to the range of the audio signal
        return (prev_output_ );

}

// Setter function to set the remanence value
void PreisachModel::setMr(float remanence) {
    remanence_ = remanence;
}

// Setter function to set the coercivity value
void PreisachModel::setHc(float coercivity) {
    coercivity_ = coercivity;
}

// Setter function to set the alpha value
void PreisachModel::setAlpha(float alpha) {
    alpha_ = alpha;
}

// Setter function to set the beta value
void PreisachModel::setBeta(float beta) {
    beta_ = beta;
}
