/*
Renaming is tricky because we have three
different rename regimes.
1. When using NETCDF-4 and not using DAP, then we want to
   rename all references and definitions of procedures in
   netcdf.h to be those in netcdf3.h. This works because
   libsrc4 will invoke the nc3_... procedures and once we
   are inside a libsrc procedure, we want to stay there.
2. When not using NETCDF-4 and using DAP, we want to rename
   selected procedures to be lnc3_..., but references to
   nc_.. procedures by lnc3_...procedures must use the
   nc_... procedures so that the DAP code can always
   intercept them.
3. When using both NETCDF-4 and DAP, we want the mix of the
   above cases.

This leads to the following (as defined in rename.h)
DAP=no  && NETCDF4=no  => use netcdf.h
DAP=no  && NETCDF4=yes => use netcdf3.h then nctonc3.h
DAP=yes && NETCDF4=no  => use netcdf.h plus RENAME macro
DAP=yes && NETCDF4=yes => use netcdf3.h then nctonc3.h plus RENAME macro.
where
nctonc3.h converts nc_* to nc3_
*/

/* When using dispatch, we want to forcibly rename everything */
#ifdef USE_DISPATCH

# define DISPNAME(proc) NC3_##proc
# define RENAME(proc) DISPNAME(proc)

#else /*!USE_DISPATCH*/

# define DISPNAME(proc) nc_##proc

/* Easy case: rename nothing */
#if !defined(USE_NETCDF4) && !defined(USE_DAP)
# define RENAME(proc) nc_##proc
# include "netcdf.h"
#endif

/* NETCDF4 only: rename all procedures taken over by libsrc4 */
#if defined(USE_NETCDF4) && !defined(USE_DAP)
# define RENAME(proc) nc_##proc
# include "netcdf3.h"
# include "nctonc3.h"
#endif

/* DAP ONLY: rename (via RENAME) procedures taken over by libncdap */
#if !defined(USE_NETCDF4) && defined(USE_DAP)
# define RENAME(proc) l3##nc_##proc
# include "netcdf.h"
#endif


/* NETCDF4+DAP: rename all procedures taken over by libsrc4,
   then further rename all procedures taken over by libncdap
*/
#if defined(USE_NETCDF4) && defined(USE_DAP)
#define RENAME(proc) l3##nc_##proc
# include "netcdf3.h"
# include "nctonc3.h"
#endif

#endif /*USE_DISPATCH*/
