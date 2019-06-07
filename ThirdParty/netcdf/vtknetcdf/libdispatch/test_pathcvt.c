/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

/**
Test the NCpathcvt
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ncwinpath.h"

#undef VERBOSE

typedef struct Test {
    char* path;
    char* expected;
} Test;

/* Path conversion tests */
static Test PATHTESTS[] = {
{"/xxx/a/b","/xxx/a/b"},
{"d:/x/y","d:\\x\\y"},
{"/cygdrive/d/x/y","d:\\x\\y"},
{"/d/x/y","d:\\x\\y"},
{"/cygdrive/d","d:\\"},
{"/d","d:\\"},
{"/cygdrive/d/git/netcdf-c/dap4_test/daptestfiles/test_anon_dim.2.syn","d:\\git\\netcdf-c\\dap4_test\\daptestfiles\\test_anon_dim.2.syn"},
{"[dap4]file:///cygdrive/d/git/netcdf-c/dap4_test/daptestfiles/test_anon_dim.2.syn","[dap4]file:///cygdrive/d/git/netcdf-c/dap4_test/daptestfiles/test_anon_dim.2.syn"},
{NULL,NULL}
};

int
main(int argc, char** argv)
{
    Test* test;
    int failcount = 0;

    for(test=PATHTESTS;test->path;test++) {
	char* cvt = NCpathcvt(test->path);
	if(cvt == NULL) {
	    fprintf(stderr,"TEST returned NULL: %s\n",test->path);
	    exit(1);
	}
	if(strcmp(cvt,test->expected) != 0) {
	    fprintf(stderr,"NCpathcvt failed:: input: |%s| expected=|%s| actual=|%s|\n",test->path,test->expected,cvt);
	    failcount++;
	}
#ifdef VERBOSE
	fprintf(stderr,"NCpathcvt:: input: |%s| actual=|%s|\n",test->path,cvt);
#endif
	free(cvt);
    }

    fprintf(stderr,"%s test_ncuri\n",failcount > 0 ? "***FAIL":"***PASS");
    return (failcount > 0 ? 1 : 0);
}
