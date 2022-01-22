#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int tap_quiet    = 0;
int global_err   = 0;
int test_counter = 0;
//expect a
//actual b
int assert_int_eq(int a, int b, const char *testcase, int line)
{
    test_counter++;
    int err = a!=b;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected %d, but observed %d instead (line %d)\n", a, b, line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_char_eq(char a, char b, const char *testcase, int line)
{
    test_counter++;
    int err = a!=b;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected %c, but observed %c instead (line %d)\n", a, b, line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_ptr_eq(const void* a, const void* b, const char *testcase, int line)
{
    test_counter++;
    int err = a!=b;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected %p, but observed %p instead (line %d)\n", a, b, line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_true(int a, const char *testcase, int line)
{
    test_counter++;
    int err = !a;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Failure on line %d\n", line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_false(int a, const char *testcase, int line)
{
    return assert_true(!a, testcase, line);
}


int assert_str_eq(const char *a, const char *b, const char *testcase, int line)
{
    test_counter++;
    int err = strcmp(a,b);
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected '%s', but observed '%s' instead (line %d)\n", a, b, line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_null(const void *v, const char *testcase, int line)
{
    test_counter++;
    int err = !!v;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected NULL value, but observed Non-NULL instead (line %d)\n", line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_non_null(const void *v, const char *testcase, int line)
{
    test_counter++;
    int err = !v;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected Non-NULL value, but observed NULL instead (line %d)\n", line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_f32_eq(float a, float b,const char *testcase, int line)
{
    test_counter++;
    int err = a!=b;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected %f, but observed %f instead (line %d)\n", a, b, line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_f32_sim(float a, float b, float t, const char *testcase, int line)
{
    test_counter++;
    float tmp = (a-b);
    tmp = tmp<0?-tmp:tmp;
    int err = tmp > t;
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Expected %f+-%f, but observed %f instead (line %d)\n", a, t, b, line);
        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

//produce a xxd style hexdump with sections highlighted using escape sequences
//
//e.g.
//0000000: 2369 6e63 6c75 6465 203c 7374 6469 6f2e  #include <stdio.
//0000010: 683e 0a23 696e 636c 7564 6520 3c73 7472  h>.#include <str
//0000020: 696e 672e 683e 0a0a 696e 7420 676c 6f62  ing.h>..int glob
//0000030: 616c 5f65 7272 203d 2030 3b0a 696e 7420  al_err = 0;.int
//0000040: 7465 7374 5f63 6f75 6e74 6572 203d 2030  test_counter = 0
//0000050: 3b0a 2f2f 6578 7065 6374 2061 0a2f 2f61  ;.//expect a.//a
//
void hexdump(const char *data, const char *mask, size_t len)
{
    const char *bold_gray = "\x1b[30;1m";
    const char *reset      = "\x1b[0m";
    int offset = 0;
    while(1)
    {
        //print line
        printf("#%07x: ", offset);

        int char_covered = 0;

        //print hex groups (8)
        for(int i=0; i<8; ++i) {

            //print doublet
            for(int j=0; j<2; ++j) {
                int loffset = offset + 2*i + j;
                if(loffset >= (int)len)
                    goto escape;

                //print hex
                {
                    //start highlight
                    if(mask && mask[loffset]){printf("%s", bold_gray);}

                    //print chars
                    printf("%02x", 0xff&data[loffset]);

                    //end highlight
                    if(mask && mask[loffset]){printf("%s", reset);}
                    char_covered += 2;
                }
            }
            printf(" ");
            char_covered += 1;
        }
escape:

        //print filler if needed
        for(int i=char_covered; i<41; ++i)
            printf(" ");

        //print ascii (16)
        for(int i=0; i<16; ++i) {
            if(isprint(data[offset+i]))
                printf("%c", data[offset+i]);
            else
                printf(".");
        }
        printf("\n");
        offset += 16;
        if(offset >= (int)len)
            return;
    }
}


int assert_hex_eq(const char *a, const char *b, size_t size_a, size_t size_b,
                  const char *testcase, int line)
{
    test_counter++;
    int err = (size_a != size_b) || memcmp(a, b, size_a);
    if(err) {
        printf("not ok %d - %s...\n", test_counter, testcase);
        printf("# Error on line %d\n", line);
        //printf("# Expected '%s', but observed '%s' instead (line %d)\n", a, b, line);

        //create difference mask
        const int longer  = size_a > size_b ? size_a : size_b;
        const int shorter = size_a < size_b ? size_a : size_b;
        char mask[longer];
        memset(mask, 0, longer);
        for(int i=0; i<shorter; ++i)
            if(a[i] != b[i])
                mask[i] = 1;

        printf("#\n");
        printf("# Expected:\n");
        hexdump(a, mask, size_a);
        printf("#\n");
        printf("# Observed:\n");
        hexdump(b, mask, size_b);

        global_err++;
    } else if(!tap_quiet)
        printf("ok %d - %s...\n", test_counter, testcase);
    return err;
}

int assert_flt_eq(float a, float b, const char *testcase, int line)
{
    int ret = assert_hex_eq((char*)&a, (char*)&b, sizeof(float), sizeof(float),
            testcase, line);
    if(ret)
        printf("#expected=%f actual %f\n", a, b);
    return ret;
}

int test_summary(void)
{
    printf("# %d test(s) failed out of %d (currently passing %f%% tests)\n",
            global_err, test_counter, 100.0-global_err*100./test_counter);
    return global_err ? EXIT_FAILURE : EXIT_SUCCESS;
}
