
#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H
#include <cstdlib>

/**
 * C++ thread safe lockless queue
 * Based off of jack's ringbuffer*/
template <class T>
class SafeQueue
{
    public:
        SafeQueue(size_t maxlen);
        ~SafeQueue();

        /**Return read size*/
        unsigned int size() const;

        /**Returns 0 for normal
         * Returns -1 on error*/
        int push(const T &in);
        int pop(T &out);

        //clears reading space
        void clear();

    private:
        unsigned int wSpace() const;
        unsigned int rSpace() const;

        //next writting spot
        volatile size_t writePtr;
        //next reading spot
        volatile size_t readPtr;
        const size_t bufSize;
        T *buffer;
};

#include "SafeQueue.cpp"
#endif
