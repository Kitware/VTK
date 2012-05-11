/* ----------------------------------------------------------------------
   MR-MPI = MapReduce-MPI library
   http://www.cs.sandia.gov/~sjplimp/mapreduce.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2009) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level MapReduce directory.
------------------------------------------------------------------------- */

#ifndef MRMPI_CONFIG_H
#define MRMPI_CONFIG_H

#include "vtkConfigure.h"
#include "vtkSystemIncludes.h"

#if defined(WIN32) && defined(VTK_BUILD_SHARED_LIBS)
 #if defined(MapReduceMPI_EXPORTS)
  #define MRMPI_EXPORT __declspec( dllexport ) 
 #else
  #define MRMPI_EXPORT __declspec( dllimport ) 
 #endif
#else
 #define MRMPI_EXPORT
#endif

#endif
