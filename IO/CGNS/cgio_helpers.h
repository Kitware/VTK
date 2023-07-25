// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2013-2014 Mickael Philit
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This file defines functions used by vtkCGNSReader and vtkCGNSReaderInternal.
 * These functions are provided to simplify CGNS file reading through the low
 * level CGIO interface.
 */
#ifndef cgio_helpers_h
#define cgio_helpers_h

#include "vtkCGNSReaderInternal.h"

#include <string.h> // for inline strcmp
#include <string>
#include <vector>

namespace CGNSRead
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * Read data of the specified type from the given node.
 */
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
  if (cgio_read_all_data_type(cgioNum, nodeId, dtName, data.data()) != CG_OK)
  {
    return 1;
  }

  return 0;
}

/*
 * Converts data read from the file using native type to the type specified
 * as the template argument. Just uses static_cast to do type conversion.
 */
template <typename T>
inline int readNodeDataAs(int cgioNum, double nodeId, std::vector<T>& data)
{
  // Retrieve data type from node
  char dtype[CGIO_MAX_DATATYPE_LENGTH + 1];
  if (cgio_get_data_type(cgioNum, nodeId, dtype) != CG_OK)
  {
    cgio_error_exit("cgio_get_data_type");
    return CG_ERROR;
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
    return CG_ERROR;
  }

  return CG_OK;
}

/*
 * Read data of char type from the given node.
 * Specialization of readNodeData<>().
 */
template <>
int readNodeData<char>(int cgioNum, double nodeId, std::vector<char>& data);

/**
 * Read string data from the given node.
 */
int readNodeStringData(int cgioNum, double nodeId, std::string& data);

/**
 * Read IDs of all children for the node with the given ID.
 */
int getNodeChildrenId(int cgioNum, double fatherId, std::vector<double>& childrenIds);

/**
 * Search for bases under the node with the given ID and read their IDs.
 */
int readBaseIds(int cgioNum, double rootId, std::vector<double>& baseIds);

/**
 * Read name, cell and physical dimensions for the given CGNSBase_t node.
 */
int readBaseCoreInfo(int cgioNum, double baseId, CGNSRead::BaseInformation& baseInfo);

/**
 * Read timesteps information in the given BaseIterativeData_t node.
 */
int readBaseIteration(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

/**
 * Read which type of pointers are used for temporal data in the given ZoneIterativeData_t node.
 */
int readZoneIterInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

/**
 * Read data arrays information in the given FlowSolution_t node.
 */
int readSolInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

/**
 * Read base family information in the given Family_t node.
 */
int readBaseFamily(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo,
  const std::string& parentPath = "");

/**
 * Read reference state information in the given ReferenceState_t node.
 */
int readBaseReferenceState(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

/**
 * Read general data array information in the given Zone_t node.
 */
int readZoneInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

/**
 * Read family name and boundary conditions information in the given Zone_t node.
 */
int readZoneInfo(int cgioNum, double zoneId, CGNSRead::ZoneInformation& zoneInfo);

/**
 * Release all IDs in the vector.
 */
void releaseIds(int cgioNum, const std::vector<double>& ids);

VTK_ABI_NAMESPACE_END
}
#endif // cgio_helpers_h
