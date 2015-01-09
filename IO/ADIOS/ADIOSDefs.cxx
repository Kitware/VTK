/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSDefs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ADIOSDefs.h"

namespace ADIOS
{

const std::string&  ToString(TransportMethod method)
{
  static const std::string valueMap[] = {
    "NULL", "POSIX", "MPI", "MPI_LUSTRE", "MPI_AGGREGATE", "VAR_MERGE",
    "Dataspaces", "DIMES", "Flexpath", "PHDF5", "NetCDF4" };
  return valueMap[method];
}

const std::string& ToString(Transform xfm)
{
  static const std::string valueMap[] = { "", "zlib", "bzlib2", "szip" };
  return valueMap[xfm];
}

}
