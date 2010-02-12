
template<class T>
SafeQueue<T>::SafeQueue(size_t maxlen)
    :writePtr(0),readPtr(0),bufSize(maxlen)
{
    buffer = new T[maxlen];
}

template<class T>
SafeQueue<T>::~SafeQueue()
{
    delete[] buffer;
}

template<class T>
unsigned int SafeQueue<T>::size() const
{
    return rSpace();
}

template<class T>
unsigned int SafeQueue<T>::rSpace() const
{
  size_t w, r;

  w = writePtr;
  r = readPtr;

  if (w > r) {
    return w - r;
  }
  else {
    return (w - r + bufSize) % bufSize;
  }
}

template<class T>
unsigned int SafeQueue<T>::wSpace() const
{
  size_t w, r;

  w = writePtr;
  r = readPtr - 1;

  if (r > w) {
    return r - w;
  }
  else {
    return (r - w + bufSize) % bufSize;
  }
}

template<class T>
int SafeQueue<T>::push(const T &in)
{
    if(!wSpace())
        return -1;

    //ok, there is space to write
    size_t w = (writePtr + 1) % bufSize;
    buffer[w] = in;
    writePtr = w;
    return 0;
}

template<class T>
int SafeQueue<T>::pop(T &out)
{
    if(!rSpace())
        return -1;

    //ok, there is space to read
    size_t r = (readPtr + 1) % bufSize;
    out = buffer[r];
    readPtr = r;
    return 0;
}

template<class T>
void SafeQueue<T>::clear()
{
    readPtr = writePtr;
}
