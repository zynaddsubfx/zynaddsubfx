#include "../Misc/Master.h"

#include <cassert>

void check_files_are_equal(const char* lfilename, const char* rfilename)
{
    const int BUFFER_SIZE = 16 * 1024;

    std::ifstream lFile(lfilename);
    std::ifstream rFile(rfilename);
    assert(lFile.is_open());
    assert(rFile.is_open());

    char lBuffer[BUFFER_SIZE];
    char rBuffer[BUFFER_SIZE];
    do {
        lFile.read(lBuffer, BUFFER_SIZE);
        rFile.read(rBuffer, BUFFER_SIZE);
        numberOfRead = lFile.gcount();//I check the files with the same size
        assert(numberOfRead == rFile.gcount());

        if (memcmp(lBuffer, rBuffer, numberOfRead) != 0)
        {
//            memset(lBuffer,0,numberOfRead);
//            memset(rBuffer,0,numberOfRead);
            return false;
        }
    } while (lFile.good() || rFile.good());
}

int main()
{
    assert(argc == 2);
    const char *filename = argv[1];

    const char* tmp_omz = "/tmp/zynaddsubfx_test_master.omz";
    const char* tmp_xmz = "/tmp/zynaddsubfx_test_master.xmz";

    {
        Master master;

        int tmp = master.loadXML(filename);
        if(tmp < 0) {
            cerr << "ERROR: Could not load master file " << filename
                 << "." << endl;
            exit(1);
        }

        master.saveOSC(tmp_omz);
    }

    {
        Master master;

        master.loadOSC(tmp_omz);

        master.saveXML(tmp_xmz);
        if(tmp < 0) {
            cerr << "ERROR: Could not save master file " << tmp_xmz
                 << "." << endl;
            exit(1);
        }

    }

    if(check_files_are_equal(filename, tmp_xmz))
        return 0;
    else
        return 1;

}
