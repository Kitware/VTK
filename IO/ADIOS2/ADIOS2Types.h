/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2Types.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2Types.h : header-only type definitions needed by the VTK::IOADIOS2 module
 *
 *  Created on: May 14, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2TYPES_H_
#define VTK_IO_ADIOS2_ADIOS2TYPES_H_

#include <map>
#include <vector>

#include "ADIOS2DataArray.h"

#include <adios2.h>

namespace adios2vtk
{
namespace types
{

/** key: variable name, value: DataArray */
using DataSet = std::map<std::string, DataArray>;

enum class DataSetType
{
  CellData,
  PointData,
  Points,
  Coordinates,
  Cells,
  Verts,
  Lines,
  Strips,
  Polys
};

using Piece = std::map<DataSetType, DataSet>;

#define ADIOS2_VTK_ARRAY_TYPE(MACRO)                                                               \
  MACRO(int)                                                                                       \
  MACRO(unsigned int)                                                                              \
  MACRO(long int)                                                                                  \
  MACRO(unsigned long int)                                                                         \
  MACRO(float)                                                                                     \
  MACRO(double)

#define ADIOS2_VTK_TIME_TYPE(MACRO)                                                                \
  MACRO(int)                                                                                       \
  MACRO(unsigned int)                                                                              \
  MACRO(long int)                                                                                  \
  MACRO(unsigned long int)                                                                         \
  MACRO(long long int)                                                                             \
  MACRO(unsigned long long int)                                                                    \
  MACRO(float)                                                                                     \
  MACRO(double)

} // end namespace types
} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_ADIOS2TYPES_H_ */
