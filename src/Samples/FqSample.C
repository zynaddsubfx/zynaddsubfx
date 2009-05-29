#include "FqSample.h"

FqSample::FqSample(const int &length)
    :Sample(length)
{}

FqSample::FqSample(float *input,const int &length)
    : Sample(input,length)
{}

FqSample::~FqSample()
{}

