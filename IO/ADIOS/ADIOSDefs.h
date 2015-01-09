/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSDefs.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __ADIOSDefs_h
#define __ADIOSDefs_h

#include <string>

namespace ADIOS
{

enum TransportMethod
{
  TransportMethod_NULL          = 0,
  TransportMethod_POSIX         = 1,
  TransportMethod_MPI           = 2,
  TransportMethod_MPI_LUSTRE    = 3,
  TransportMethod_MPI_AGGREGATE = 4,
  TransportMethod_VAR_MERGE     = 5,
  TransportMethod_DataSpaces    = 6,
  TransportMethod_DIMES         = 7,
  TransportMethod_FlexPath      = 8,
  TransportMethod_PHDF5         = 9,
  TransportMethod_NetCDF4       = 10
};
const std::string& ToString(TransportMethod);

enum Transform
{
  Transform_NONE   = 0,
  Transform_ZLIB   = 1,
  Transform_BZLIB2 = 2,
  Transform_SZIP   = 3
};
const std::string& ToString(Transform);

enum ReadMethod
{
  ReadMethod_BP           = 0,
  ReadMethod_BP_AGGREGATE = 1,
  ReadMethod_DataSpaces   = 3,
  ReadMethod_DIMES        = 4,
  ReadMethod_FlexPath     = 5
};

} // end namespace
#endif //__ADIOSDefs_h
// VTK-HeaderTest-Exclude: ADIOSDefs.h
