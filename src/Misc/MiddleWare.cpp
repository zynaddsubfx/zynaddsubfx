#include "../Params/PADnoteParameters.h"
#include <cstring>
#include <cstdio>

#include <rtosc/thread-link.h>

extern rtosc::ThreadLink *uToB;
namespace MiddleWare {
void preparePadSynth(const char *path, PADnoteParameters *p)
{
    char path_buffer[1024];
    strncpy(path_buffer, path, 1024);
    if(char *old_end = rindex(path_buffer, '/')) {
        unsigned max = 0;
        p->sampleGenerator([&max,&old_end,&path_buffer]
                (unsigned N, PADnoteParameters::Sample &s)
                {
                max = max<N ? N : max;
                sprintf(old_end, "/sample%d", N);
                uToB->write(path_buffer, "ifb",
                    s.size, s.basefreq, sizeof(float*), &s.smp);
                });
        //clear out unused samples
        for(unsigned i = max+1; i < PAD_MAX_SAMPLES; ++i) {
            sprintf(old_end, "/sample%d", i);
            uToB->write(path_buffer, "ifb",
                    0, 440.0f, sizeof(float*), NULL);
        }
    }
}
}
