#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#define URL "http://%s/dts/test.02"
#define VAR "i32"

#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

#undef DEBUG

int
main()
{
    int ncid, varid;
    int retval;
    int i32[100];
    size_t start[1];
    size_t count[1];
    int ok = 1;    
    char url[1024];

    {
    char* evv = getenv("REMOTETESTSERVER");
    if(evv == NULL)
	evv = "remotetest.unidata.ucar.edu";
    snprintf(url,sizeof(url),URL,evv);
    }

    if ((retval = nc_open(url, 0, &ncid)))
       ERR(retval);
    if ((retval = nc_inq_varid(ncid, VAR, &varid)))
       ERR(retval);

    start[0] = 0;
    count[0] = 26;
    if ((retval = nc_get_vara_int(ncid, varid, start, count, i32)))
    if(retval != NC_EINVALCOORDS) {
	printf("nc_get_vara_int did not return NC_EINVALCOORDS");
	ok = 0;
    }

    nc_close(ncid);

    printf(ok?"*** PASS\n":"*** FAIL\n");
    return 0;
}
