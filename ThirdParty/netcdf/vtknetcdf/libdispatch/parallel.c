/*
 Copyright 2010 University Corporation for Atmospheric
 Research/Unidata. See COPYRIGHT file for more info.

 This file has the parallel I/O functions.

 "$Id: parallel.c,v 1.1 2010/06/01 15:46:50 ed Exp $" 
*/

#include <config.h>
#include <netcdf_f.h>
#include "ncdispatch.h"

/* This function creates a file for use with parallel I/O. */
int
nc_create_par(const char *path, int cmode, MPI_Comm comm, 
	      MPI_Info info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else   
   NC_MPI_INFO data;
   MPI_Comm comm_c = 0;
   MPI_Info info_c = 0;

#ifdef HAVE_MPI_COMM_F2C
   comm_c = MPI_Comm_f2c(comm);
   info_c = MPI_Info_f2c(info);
#else
   comm_c = (MPI_Comm)comm;
   info_c = (MPI_Info)info;
#endif

   data.comm = comm_c;
   data.info = info_c;
   return NC_create(path, cmode, 0, 0, NULL, 1, &data, ncidp);
#endif /* USE_PARALLEL */
}

/* This function opens a file for parallel I/O. */
int
nc_open_par(const char *path, int mode, MPI_Comm comm, 
	    MPI_Info info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else
   NC_MPI_INFO mpi_data;

   mpi_data.comm = comm;
   mpi_data.info = info;

   return NC_open(path, mode, 0, NULL, 1, &mpi_data, ncidp);
#endif /* USE_PARALLEL */
}

/* Fortran needs to pass MPI comm/info as integers. */
int
nc_open_par_fortran(const char *path, int mode, int comm, 
		    int info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else

   MPI_Comm comm_c = 0;
   MPI_Info info_c = 0;

   /* Convert fortran comm and info to C comm and info, if there is a
    * function to do so. Otherwise just pass them. */
#ifdef HAVE_MPI_COMM_F2C
   comm_c = MPI_Comm_f2c(comm);
   info_c = MPI_Info_f2c(info);
#else
   comm_c = (MPI_Comm)comm;
   info_c = (MPI_Info)info;
#endif

   return nc_open_par(path, mode, comm_c, info_c, ncidp);
#endif
}

/* This function will change the parallel access of a variable from
 * independent to collective. */
int
nc_var_par_access(int ncid, int varid, int par_access)
{
    NC* ncp;
    int stat;

    if ((stat = NC_check_id(ncid, &ncp)))
       return stat;

#ifndef USE_PARALLEL
    return NC_ENOPAR;
#else
    return ncp->dispatch->var_par_access(ncid,varid,par_access);
#endif
}

/* when calling from fortran: convert MPI_Comm and MPI_Info to C */
int
nc_create_par_fortran(const char *path, int cmode, int comm, 
		      int info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else
   MPI_Comm comm_c = 0;
   MPI_Info info_c = 0;
#ifdef USE_PARALLEL
#ifdef HAVE_MPI_COMM_F2C
   comm_c = MPI_Comm_f2c(comm);
   info_c = MPI_Info_f2c(info);
#else
   comm_c = (MPI_Comm)comm;
   info_c = (MPI_Info)info;
#endif
#endif
   return nc_create_par(path, cmode, comm_c, info_c, ncidp);
#endif
}



