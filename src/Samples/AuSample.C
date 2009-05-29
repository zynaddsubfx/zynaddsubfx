#include "AuSample.h"

AuSample::AuSample(const int &length)
    : Sample(length){}

AuSample::AuSample(float *input,const int &length)
    : Sample(input,length){}

