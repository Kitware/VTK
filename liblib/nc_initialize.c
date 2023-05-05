/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

#include "config.h"

#ifdef USE_PARALLEL
#include <mpi.h>
#endif

#include "ncdispatch.h"

extern int NC3_initialize(void);
extern int NC3_finalize(void);

#ifdef USE_NETCDF4
#include "nc4internal.h"
extern int NC4_initialize(void);
extern int NC4_finalize(void);
#endif

#ifdef USE_HDF5
#include "hdf5internal.h"
extern int NC_HDF5_initialize(void);
extern int NC_HDF5_finalize(void);
#endif

#ifdef ENABLE_DAP2
extern int NCD2_initialize(void);
extern int NCD2_finalize(void);
#endif

#ifdef ENABLE_DAP4
extern int NCD4_initialize(void);
extern int NCD4_finalize(void);
#endif

#ifdef USE_PNETCDF
extern int NCP_initialize(void);
extern int NCP_finalize(void);
#endif

#ifdef USE_HDF4
extern int NC_HDF4_initialize(void);
extern int NC_HDF4_finalize(void);
#endif

#ifdef ENABLE_S3_SDK
EXTERNL int NC_s3sdkinitialize(void);
EXTERNL int NC_s3sdkfinalize(void);
#endif

#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#endif

int NC_initialized = 0;
int NC_finalized = 1;

#ifdef ENABLE_ATEXIT_FINALIZE
/* Provide the void function to give to atexit() */
static void
finalize_atexit(void)
{
    (void)nc_finalize();
}
#endif

/**
This procedure invokes all defined
initializers, and there is an initializer
for every known dispatch table.
So if you modify the format of NC_Dispatch,
then you need to fix it everywhere.
It also initializes appropriate external libraries.
*/

int
nc_initialize()
{
    int stat = NC_NOERR;

    if(NC_initialized) return NC_NOERR;
    NC_initialized = 1;
    NC_finalized = 0;

    /* Do general initialization */
    if((stat = NCDISPATCH_initialize())) goto done;

    /* Initialize each active protocol */
    if((stat = NC3_initialize())) goto done;
#ifdef ENABLE_DAP
    if((stat = NCD2_initialize())) goto done;
#endif
#ifdef ENABLE_DAP4
    if((stat = NCD4_initialize())) goto done;
#endif
#ifdef USE_PNETCDF
    if((stat = NCP_initialize())) goto done;
#endif
#ifdef USE_NETCDF4
    if((stat = NC4_initialize())) goto done;
#endif /* USE_NETCDF4 */
#ifdef USE_HDF5
    if((stat = NC_HDF5_initialize())) goto done;
#endif
#ifdef USE_HDF4
    if((stat = NC_HDF4_initialize())) goto done;
#endif
#ifdef ENABLE_S3_SDK
    if((stat = NC_s3sdkinitialize())) goto done;
#endif
#ifdef ENABLE_NCZARR
    if((stat = NCZ_initialize())) goto done;
#endif

#ifdef ENABLE_ATEXIT_FINALIZE
    /* Use atexit() to invoke nc_finalize */
    if(atexit(finalize_atexit))
	fprintf(stderr,"atexit failed\n");
#endif

done:
    return stat;
}

/**
This procedure invokes all defined
finalizers, and there should be one
for every known dispatch table.
So if you modify the format of NC_Dispatch,
then you need to fix it everywhere.
It also finalizes appropriate external libraries.
*/

int
nc_finalize(void)
{
    int stat = NC_NOERR;
    int failed = stat;

    if(NC_finalized) goto done;
    NC_initialized = 0;
    NC_finalized = 1;

    /* Finalize each active protocol */

#ifdef ENABLE_DAP2
    if((stat = NCD2_finalize())) failed = stat;
#endif
#ifdef ENABLE_DAP4
    if((stat = NCD4_finalize())) failed = stat;
#endif

#ifdef USE_PNETCDF
    if((stat = NCP_finalize())) failed = stat;
#endif

#ifdef USE_HDF4
    if((stat = NC_HDF4_finalize())) failed = stat;
#endif /* USE_HDF4 */

#ifdef USE_NETCDF4
    if((stat = NC4_finalize())) failed = stat;
#endif /* USE_NETCDF4 */

#ifdef USE_HDF5
    if((stat = NC_HDF5_finalize())) failed = stat;
#endif

#ifdef ENABLE_NCZARR
    if((stat = NCZ_finalize())) failed = stat;
#endif

#ifdef ENABLE_S3_SDK
    if((stat = NC_s3sdkfinalize())) failed = stat;
#endif

    if((stat = NC3_finalize())) failed = stat;

    /* Do general finalization */
    if((stat = NCDISPATCH_finalize())) failed = stat;

done:
    if(failed) fprintf(stderr,"nc_finalize failed: %d\n",failed);
    return failed;
}
