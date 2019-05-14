/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2Types.h
 *
 *  Created on: May 14, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2TYPES_H_
#define VTK_IO_ADIOS2_ADIOS2TYPES_H_

#include <map>
#include <vector>

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"

namespace adios2vtk
{
namespace types
{

struct DataArray
{
  std::map<std::string, vtkSmartPointer<vtkDataArray> > Vector;
  vtkSmartPointer<vtkDataArray> Scalar;
};

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
