#include "StereoSample.h"

StereoSample::StereoSample(int length)
    :leftSample(length),rightSample(length)
{std::cout<<"StereoSample\n";}

void StereoSample::clear()
{
    std::cout<<"StereoSample::Clear\n";
    leftSample.clear();
    rightSample.clear();
}

int StereoSample::size() const
{
    return(leftSample.size());
}

float & StereoSample::operator[](const int &index)
{
    return(leftSample[index]);
}

StereoSample & StereoSample::operator=(const StereoSample & smp)
{
    std::cout<<"StereoSample::operator=\n";
    leftSample=smp.leftSample;
    rightSample=smp.rightSample;
}
