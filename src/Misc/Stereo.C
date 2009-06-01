
template <class T>
Stereo<T>::Stereo(const T &left, const T &right)
    :leftChannel(left),rightChannel(right)
{}

template <class T>
Stereo<T>::Stereo(const T &val)
    :leftChannel(val),rightChannel(val)
{}

template <class T>
void Stereo<T>::operator=(const Stereo<T> & nstr)
{
    leftChannel=nstr.leftChannel;
    rightChannel=nstr.rightChannel;
}
