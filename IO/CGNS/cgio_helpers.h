/*=========================================================================

  Program:   Visualization Toolkit
  Module:    cgio_helpers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
//  Copyright 2013-2014 Mickael Philit.

// .NAME cgio_helpers -- function used by vtkCGNSReader
//                       and vtkCGNSReaderInternal
// .SECTION Description
//     provide function to simplify "CGNS" reading through cgio
//
// .SECTION Caveats
//
//
// .SECTION Thanks
// Thanks to .

#ifndef cgio_helpers_h
#define cgio_helpers_h

#include <map>
#include <string.h> // for inline strcmp
#include <string>
#include <vector>

#include "vtkCGNSReaderInternal.h"

namespace CGNSRead
{

//------------------------------------------------------------------------------
template <typename T>
inline int readNodeData(int cgioNum, double nodeId, std::vector<T>& data)
{
  int n;
  cgsize_t size = 1;
  cgsize_t dimVals[12];
  int ndim;
  constexpr const char* dtName = CGNSRead::detail::cgns_type_name<T>();

  if (cgio_get_dimensions(cgioNum, nodeId, &ndim, dimVals) != CG_OK)
  {
    cgio_error_exit("cgio_get_dimensions");
    return 1;
  }

  // allocate data
  for (n = 0; n < ndim; n++)
  {
    size *= dimVals[n];
  }
  if (size <= 0)
  {
    return 1;
  }
  data.resize(size);

  // read data
  if (cgio_read_all_data_type(cgioNum, nodeId, dtName, &data[0]) != CG_OK)
  {
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
/*
 * Converts data read from the file using native type to the type specified
 * as the template argument. Just uses static_cast to do type conversion.
 */
template <typename T>
inline int readNodeDataAs(int cgioNum, double nodeId, std::vector<T>& data)
{
  // let's get type in file.
  char dtype[CGIO_MAX_DATATYPE_LENGTH + 1];
  if (cgio_get_data_type(cgioNum, nodeId, dtype) != CG_OK)
  {
    cgio_error_exit("cgio_get_data_type");
    return 1;
  }

  if (strcmp(dtype, "I4") == 0)
  {
    std::vector<vtkTypeInt32> i32vector;
    readNodeData<vtkTypeInt32>(cgioNum, nodeId, i32vector);
    data.resize(i32vector.size());
    std::copy(i32vector.begin(), i32vector.end(), data.begin());
  }
  else if (strcmp(dtype, "I8") == 0)
  {
    std::vector<vtkTypeInt64> i64vector;
    readNodeData<vtkTypeInt64>(cgioNum, nodeId, i64vector);
    data.resize(i64vector.size());
    std::copy(i64vector.begin(), i64vector.end(), data.begin());
  }
  else if (strcmp(dtype, "R4") == 0)
  {
    std::vector<float> fvector;
    readNodeData<float>(cgioNum, nodeId, fvector);
    data.resize(fvector.size());
    std::copy(fvector.begin(), fvector.end(), data.begin());
  }
  else if (strcmp(dtype, "R8") == 0)
  {
    std::vector<double> dvector;
    readNodeData<double>(cgioNum, nodeId, dvector);
    data.resize(dvector.size());
    std::copy(dvector.begin(), dvector.end(), data.begin());
  }
  else
  {
    return 1;
  }
  return CG_OK;
}

//------------------------------------------------------------------------------
// Specialize char array
template <>
int readNodeData<char>(int cgioNum, double nodeId, std::vector<char>& data);

//------------------------------------------------------------------------------
int readNodeStringData(int cgioNum, double nodeId, std::string& data);

//------------------------------------------------------------------------------
int getNodeChildrenId(int cgioNum, double fatherId, std::vector<double>& childrenIds);

//------------------------------------------------------------------------------
int readBaseIds(int cgioNum, double rootId, std::vector<double>& baseIds);

//------------------------------------------------------------------------------
int readBaseCoreInfo(int cgioNum, double baseId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readBaseIteration(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readZoneIterInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readSolInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readBaseFamily(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo,
  const std::string& parentPath = "");

//------------------------------------------------------------------------------
int readBaseReferenceState(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readZoneInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
/**
 * Fills up ZoneInformation using the zoneId for the Zone_t node.
 */
int readZoneInfo(int cgioNum, double zoneId, CGNSRead::ZoneInformation& zoneInfo);
//------------------------------------------------------------------------------
/**
 * release all ids in the vector.
 */
void releaseIds(int cgioNum, const std::vector<double>& ids);
}
#endif // cgio_helpers_h
