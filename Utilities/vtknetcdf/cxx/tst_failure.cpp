/*********************************************************************
 * Copyright 2006, UCAR/Unidata See COPYRIGHT file for copying and
 * redistribution conditions.
 * 
 * This runs the C++ tests for netCDF to test for failure.
 * 
 * $Id: tst_failure.cpp,v 1.2 2007/05/14 14:10:59 ed Exp $
 *********************************************************************/

#include <config.h>
#include <iostream>
using namespace std;

#include <string.h>
#include "netcdfcpp.h"

#define FILE "tst_failure.nc"
#define LAT "lat"

int
main( void )	// test new netCDF interface
{
   const int NLATS = 4;

   // Cause program to exit horribly on failure.
   NcError *err = new NcError(NcError::verbose_fatal);

   // Create a file.
   NcFile nc(FILE, NcFile::Replace, NULL, 0, NcFile::Classic); 

   // Check if the file was opened successfully. But if it wasn't
   // return 0 to cause make check to fail (since make check is
   // expecting this program to return a non-zero value.)
   if (! nc.is_valid()) {
      cerr << "can't create netCDF file " << FILE << "\n";
      return 0;
   }

   // Create a dimension.
   NcDim* latd = nc.add_dim(LAT, NLATS);

   // This will fail, because the dim already exists.
   NcDim* latd1 = nc.add_dim(LAT, NLATS);

   // If we get here, that's bad.
   return 0;
}
