#include <cxxtest/TestSuite.h>
#include "../Samples/AuSample.h"

class SampleTest : public CxxTest::TestSuite
{
    public:
        void testInit()
        {
            AuSample smp(10);
            TS_ASSERT_EQUALS(smp.size(),10);
            for(int i=0;i<20;++i)
                TS_ASSERT_EQUALS(smp[i],0.0);
        }

        void testAssign()
        {
            AuSample smp(3);
            smp[0]=0;
            smp[1]=1;
            smp[2]=2;
            AuSample nsmp(40);
            nsmp=smp;
            TS_ASSERT_EQUALS(smp.size(),nsmp.size());
            for(int i=0;i<29;++i)
                TS_ASSERT_EQUALS(smp[i],nsmp[i]);
        }
        void testBounds()
        {
            AuSample smp(0);
            TS_ASSERT(smp.size()!=0);
        }

        void testAllocDealloc()
        {
            float * fl=new float[50];
            for(int i=0;i<50;++i)
                *(fl+i)=i;
            AuSample smp(2);
            smp = AuSample(fl, 50);
            delete [] fl;
            for(int i=0;i<50;++i)
                TS_ASSERT_DELTA(smp[i],i,0.001);
            smp = AuSample(3);
        }

        void testClear()
        {
            AuSample smp(50);
            for(int i=0;i<50;++i)
                smp[i]=10;
            smp.clear();
            for(int i=0;i<50;++i)
                TS_ASSERT_EQUALS(smp[i],0);
        }

};
