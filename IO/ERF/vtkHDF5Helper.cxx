// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDF5Helper.h"

#include "vtkHDF5ScopedHandle.h"

#include "vtkAbstractArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <cassert>
#include <numeric>

//----------------------------------------------------------------------------
std::vector<std::string> vtkHDF5Helper::GetChildren(const hid_t& id, const std::string& name)
{
  std::vector<std::string> results;
  H5Giterate(id, name.c_str(), nullptr, vtkHDF5Helper::FileInfoCallBack, &results);

  return results;
}

//----------------------------------------------------------------------------
std::string vtkHDF5Helper::GetPathFromName(
  const hid_t& id, const std::string& currentName, const std::string& name)
{
  std::string fullPath;
  if (currentName.empty())
  {
    return fullPath;
  }

  std::vector<std::string> results = vtkHDF5Helper::GetChildren(id, currentName);
  for (const auto& result : results)
  {
    std::string child = currentName + result + "/";
    if (result == name)
    {
      fullPath = child;
      break;
    }

    fullPath = vtkHDF5Helper::GetPathFromName(id, child, name);
    if (!fullPath.empty())
    {
      break;
    }
  }

  return fullPath;
}

//----------------------------------------------------------------------------
bool vtkHDF5Helper::ArrayExists(hid_t fileId, const char* pathName)
{
  return (H5Lexists(fileId, pathName, H5P_DEFAULT) > 0);
}

//----------------------------------------------------------------------------
bool vtkHDF5Helper::GroupExists(hid_t fileId, const char* groupName)
{
  // Same implementation as ArrayExists, but that's okay.
  return (H5Lexists(fileId, groupName, H5P_DEFAULT) > 0);
}

//----------------------------------------------------------------------------
hsize_t vtkHDF5Helper::GetDataLength(hid_t arrayId)
{
  hid_t dataspace = H5Dget_space(arrayId);
  hid_t ndims = H5Sget_simple_extent_ndims(dataspace);
  if (ndims != 1)
  {
    return ndims;
  }

  hsize_t length = 0;
  int numDimensions = H5Sget_simple_extent_dims(dataspace, &length, nullptr);
  if (numDimensions < 0)
  {
    vtkGenericWarningMacro("Failed to get length of array");
    return 0;
  }

  return length;
}

//----------------------------------------------------------------------------
std::vector<hsize_t> vtkHDF5Helper::GetDataDimensions(hid_t arrayId)
{
  hid_t dataspace = H5Dget_space(arrayId);
  hsize_t numDims = H5Sget_simple_extent_ndims(dataspace);
  std::vector<hsize_t> dims;
  dims.resize(numDims);

  if (H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr) == -1)
  {
    vtkGenericWarningMacro("Failed to get length of array");
    return dims;
  }

  return dims;
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkHDF5Helper::CreateDataArray(const hid_t& fileId, const std::string& pathName)
{
  vtkSmartPointer<vtkAbstractArray> dataArray;

  if (!vtkHDF5Helper::ArrayExists(fileId, pathName.c_str()))
  {
    vtkWarningWithObjectMacro(nullptr, "Array name '" << pathName << "' isn't available.");
    return nullptr;
  }

  vtkHDF::ScopedH5DHandle arrayId = H5Dopen(fileId, pathName.c_str());
  if (arrayId < 0)
  {
    vtkWarningWithObjectMacro(nullptr, "No array named " << pathName << " available");
    return nullptr;
  }

  hsize_t length = vtkHDF5Helper::GetDataLength(arrayId);
  if (length <= 0)
  {
    return nullptr;
  }

  std::vector<hsize_t> dims = vtkHDF5Helper::GetDataDimensions(arrayId);
  if (dims.size() > 2)
  {
    vtkWarningWithObjectMacro(nullptr, "Only 1D or 2D array are supported.");
    return nullptr;
  }

  // Now the type
  hid_t rawType = H5Dget_type(arrayId);
  hid_t dataType = H5Tget_native_type(rawType, H5T_DIR_ASCEND);

  if (H5Tequal(dataType, H5T_NATIVE_INT))
  {
    dataArray = vtkIntArray::New();
    dataArray->SetNumberOfTuples(dims[0]);
    if (dims.size() == 2)
    {
      dataArray->SetNumberOfComponents(dims[1]);
      dataArray->SetNumberOfTuples(dims[0]);
    }

    int* arrayPtr = static_cast<int*>(vtkArrayDownCast<vtkIntArray>(dataArray)->GetPointer(0));
    H5Dread(arrayId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
  }
  else if (H5Tequal(dataType, H5T_NATIVE_FLOAT))
  {
    dataArray = vtkFloatArray::New();
    dataArray->SetNumberOfTuples(dims[0]);
    if (dims.size() == 2)
    {
      dataArray->SetNumberOfComponents(dims[1]);
      dataArray->SetNumberOfTuples(dims[0]);
    }
    else
    {
      dataArray->SetNumberOfComponents(1);
    }
    float* arrayPtr =
      static_cast<float*>(vtkArrayDownCast<vtkFloatArray>(dataArray)->GetPointer(0));
    H5Dread(arrayId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
  }
  else if (H5Tequal(dataType, H5T_NATIVE_DOUBLE))
  {
    dataArray = vtkDoubleArray::New();
    dataArray->SetNumberOfTuples(dims[0]);
    if (dims.size() == 2)
    {
      dataArray->SetNumberOfComponents(dims[1]);
      dataArray->SetNumberOfTuples(dims[0]);
    }

    double* arrayPtr =
      static_cast<double*>(vtkArrayDownCast<vtkDoubleArray>(dataArray)->GetPointer(0));
    H5Dread(arrayId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
  }
  else if (H5Tget_class(dataType) == H5T_STRING)
  {
    dataArray = vtkStringArray::New();
    dataArray->SetNumberOfTuples(dims[0]);
    if (dims.size() == 2)
    {
      dataArray->SetNumberOfComponents(dims[1]);
      dataArray->SetNumberOfTuples(dims[0]);
    }

    size_t sdim = H5Tget_size(dataType);
    hid_t space = H5Dget_space(arrayId);
    hsize_t dim;
    int ndims = H5Sget_simple_extent_dims(space, &dim, nullptr);
    if (ndims != 1)
    {
      return nullptr;
    }

    char** rdata = new char*[dim];
    rdata[0] = new char[dim * sdim];
    for (hsize_t i = 1; i < dim; ++i)
    {
      rdata[i] = rdata[0] + i * sdim;
    }

    vtkHDF::ScopedH5THandle memtype = H5Tcopy(H5T_C_S1);
    H5Tset_size(memtype, sdim);
    if (H5Dread(arrayId, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rdata[0]) < 0)
    {
      return nullptr;
    }

    for (hsize_t i = 0; i < dim; ++i)
    {
      dataArray->SetVariantValue(i, (vtkVariant)rdata[i]);
    }

    delete[] rdata[0];
    delete[] rdata;
  }

  return dataArray;
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkHDF5Helper::CreateDataArray(
  const hid_t& fileId, const std::string& path, const std::string& dataSetName)
{
  std::string pathName = path + "/" + dataSetName;
  vtkAbstractArray* dataArray = vtkHDF5Helper::CreateDataArray(fileId, pathName);
  if (dataArray)
  {
    dataArray->SetName(dataSetName.c_str());
  }

  return dataArray;
}

//-----------------------------------------------------------------------------
herr_t vtkHDF5Helper::FileInfoCallBack(hid_t vtkNotUsed(loc_id), const char* name, void* opdata)
{
  std::vector<std::string>* names = reinterpret_cast<std::vector<std::string>*>(opdata);
  assert(names != nullptr);
  names->emplace_back(name);

  return 0;
}
