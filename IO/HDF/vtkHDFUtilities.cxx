// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFUtilities.h"

#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const std::map<int, std::string> ARRAY_OFFSET_GROUPS = { { 0, "PointDataOffsets" },
  { 1, "CellDataOffsets" }, { 2, "FieldDataOffsets" } };

/**
 * Used to store HDF native types in a map
 */
struct TypeDescription
{
  int Class;
  size_t Size;
  int Sign;
  TypeDescription()
    : Class(H5T_NO_CLASS)
    , Size(0)
    , Sign(H5T_SGN_ERROR)
  {
  }
  bool operator<(const TypeDescription& other) const
  {
    return Class < other.Class || (Class == other.Class && Size < other.Size) ||
      (Class == other.Class && Size == other.Size && Sign < other.Sign);
  }
};

//------------------------------------------------------------------------------
/**
 * Associates a struct of three integers with HDF type. This can be used as
 * key in a map.
 */
TypeDescription GetTypeDescription(hid_t type)
{
  TypeDescription td;
  td.Class = H5Tget_class(type);
  td.Size = H5Tget_size(type);
  if (td.Class == H5T_INTEGER)
  {
    td.Sign = H5Tget_sign(type);
  }
  return td;
}

//------------------------------------------------------------------------------
/**
 * Create a vtkDataArray based on the C++ template type T.
 * For instance, for a float we create a vtkFloatArray.
 * this can be constexpr in C++17 standard
 */
template <typename T>
vtkDataArray* NewVtkDataArray()
{
  if (std::is_same<T, char>::value)
  {
    return vtkCharArray::New();
  }
  else if (std::is_same<T, signed char>::value)
  {
    return vtkSignedCharArray::New();
  }
  else if (std::is_same<T, unsigned char>::value)
  {
    return vtkUnsignedCharArray::New();
  }
  else if (std::is_same<T, short>::value)
  {
    return vtkShortArray::New();
  }
  else if (std::is_same<T, unsigned short>::value)
  {
    return vtkUnsignedShortArray::New();
  }
  else if (std::is_same<T, int>::value)
  {
    return vtkIntArray::New();
  }
  else if (std::is_same<T, unsigned int>::value)
  {
    return vtkUnsignedIntArray::New();
  }
  else if (std::is_same<T, long>::value)
  {
    return vtkLongArray::New();
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    return vtkUnsignedLongArray::New();
  }
  else if (std::is_same<T, long long>::value)
  {
    return vtkLongLongArray::New();
  }
  else if (std::is_same<T, unsigned long long>::value)
  {
    return vtkUnsignedLongLongArray::New();
  }
  else if (std::is_same<T, float>::value)
  {
    return vtkFloatArray::New();
  }
  else if (std::is_same<T, double>::value)
  {
    return vtkDoubleArray::New();
  }
  else
  {
    vtkErrorWithObjectMacro(nullptr, "Invalid type: " << typeid(T).name());
    return nullptr;
  }
}

//------------------------------------------------------------------------------
template <typename T>
bool NewArray(
  hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents, T* data)
{
  hid_t nativeType = vtkHDFUtilities::TemplateTypeToHdfNativeType<T>();
  std::vector<hsize_t> count(fileExtent.size() / 2), start(fileExtent.size() / 2);
  for (size_t i = 0; i < count.size(); ++i)
  {
    count[i] = fileExtent[i * 2 + 1] - fileExtent[i * 2];
    start[i] = fileExtent[i * 2];
  }
  if (numberOfComponents > 1)
  {
    count.push_back(numberOfComponents);
    start.push_back(0);
  }
  vtkHDF::ScopedH5SHandle memspace =
    H5Screate_simple(static_cast<int>(count.size()), count.data(), nullptr);
  if (memspace < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Screate_simple for memory space");
    return false;
  }
  // create the filespace and select the required fileExtent
  vtkHDF::ScopedH5SHandle filespace = H5Dget_space(dataset);
  if (filespace < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Dget_space for array");
    return false;
  }
  if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr) <
    0)
  {
    std::ostringstream ostr;
    std::ostream_iterator<int> oi(ostr, " ");
    ostr << "Error selecting hyperslab, \nstart: ";
    std::copy(start.begin(), start.end(), oi);
    ostr << "\ncount: ";
    std::copy(count.begin(), count.end(), oi);
    vtkErrorWithObjectMacro(nullptr, << ostr.str());
    return false;
  }

  // read hyperslab
  if (H5Dread(dataset, nativeType, memspace, filespace, H5P_DEFAULT, data) < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Dread "
                                     << "start: " << start[0] << ", " << start[1] << ", "
                                     << start[2] << " count: " << count[0] << ", " << count[1]
                                     << ", " << count[2]);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
template <typename T>
vtkDataArray* NewArray(
  hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents)
{
  int numberOfTuples = 1;
  size_t ndims = fileExtent.size() / 2;
  for (size_t i = 0; i < ndims; ++i)
  {
    size_t j = i << 1;
    numberOfTuples *= (fileExtent[j + 1] - fileExtent[j]);
  }
  auto array = vtkAOSDataArrayTemplate<T>::SafeDownCast(::NewVtkDataArray<T>());
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfTuples);
  T* data = array->GetPointer(0);
  if (!::NewArray(dataset, fileExtent, numberOfComponents, data))
  {
    array->Delete();
    array = nullptr;
  }
  return array;
}

using ArrayReader = vtkDataArray*(
  hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents);
using TypeReaderMap = std::map<::TypeDescription, ArrayReader*>;

//------------------------------------------------------------------------------
/**
 * Builds a map between native types and GetArray routines for that type.
 */
::TypeReaderMap BuildTypeReaderMap()
{
  ::TypeReaderMap readerMap;

  readerMap[::GetTypeDescription(H5T_NATIVE_CHAR)] = &::NewArray<char>;
  readerMap[::GetTypeDescription(H5T_NATIVE_SCHAR)] = &::NewArray<signed char>;
  readerMap[::GetTypeDescription(H5T_NATIVE_UCHAR)] = &::NewArray<unsigned char>;
  readerMap[::GetTypeDescription(H5T_NATIVE_SHORT)] = &::NewArray<short>;
  readerMap[::GetTypeDescription(H5T_NATIVE_USHORT)] = &::NewArray<unsigned short>;
  readerMap[::GetTypeDescription(H5T_NATIVE_INT)] = &::NewArray<int>;
  readerMap[::GetTypeDescription(H5T_NATIVE_UINT)] = &::NewArray<unsigned int>;
  if (!readerMap[::GetTypeDescription(H5T_NATIVE_LONG)])
  {
    // long may be the same as int
    readerMap[::GetTypeDescription(H5T_NATIVE_LONG)] = &::NewArray<long>;
    readerMap[::GetTypeDescription(H5T_NATIVE_ULONG)] = &::NewArray<unsigned long>;
  }
  if (!readerMap[::GetTypeDescription(H5T_NATIVE_LLONG)])
  {
    // long long may be the same as long
    readerMap[::GetTypeDescription(H5T_NATIVE_LLONG)] = &::NewArray<long long>;
    readerMap[::GetTypeDescription(H5T_NATIVE_ULLONG)] = &::NewArray<unsigned long long>;
  }
  readerMap[::GetTypeDescription(H5T_NATIVE_FLOAT)] = &::NewArray<float>;
  readerMap[::GetTypeDescription(H5T_NATIVE_DOUBLE)] = &::NewArray<double>;
  return readerMap;
}

//------------------------------------------------------------------------------
/**
 * Return a pointer on function to use GetArray routines corresponding to native type.
 */
ArrayReader* GetArrayBuilder(hid_t type)
{
  static const ::TypeReaderMap readerMap = ::BuildTypeReaderMap();
  auto it = readerMap.find(::GetTypeDescription(type));
  if (it == readerMap.end())
  {
    return nullptr;
  }
  return it->second;
}

//-----------------------------------------------------------------------------
herr_t AddName(hid_t group, const char* name, const H5L_info_t*, void* op_data)
{
  auto array = static_cast<std::vector<std::string>*>(op_data);
  herr_t status = -1;
  H5O_info_t infobuf;
  if ((status = H5Oget_info_by_name(group, name, &infobuf, H5P_DEFAULT)) >= 0 &&
    infobuf.type == H5O_TYPE_DATASET)
  {
    array->push_back(name);
  }
  return status;
}
}

//------------------------------------------------------------------------------
/**
 * Return the dataset type mapped to the "Type" attribute
 * of the groupID group.
 * Return true if a valid data type was found.
 */
bool vtkHDFUtilities::ReadDataSetType(hid_t groupID, int& dataSetType)
{
  if (H5Aexists(groupID, "Type") <= 0)
  {
    vtkDebugWithObjectMacro(nullptr, "Can't find the `Type` attribute.");
    return false;
  }

  std::string typeName;
  vtkHDFUtilities::GetStringAttribute(groupID, "Type", typeName);

  if (typeName == "OverlappingAMR")
  {
    dataSetType = VTK_OVERLAPPING_AMR;
  }
  else if (typeName == "ImageData")
  {
    dataSetType = VTK_IMAGE_DATA;
  }
  else if (typeName == "UnstructuredGrid")
  {
    dataSetType = VTK_UNSTRUCTURED_GRID;
  }
  else if (typeName == "PolyData")
  {
    dataSetType = VTK_POLY_DATA;
  }
  else if (typeName == "HyperTreeGrid")
  {
    dataSetType = VTK_HYPER_TREE_GRID;
  }
  else if (typeName == "PartitionedDataSetCollection")
  {
    dataSetType = VTK_PARTITIONED_DATA_SET_COLLECTION;
  }
  else if (typeName == "MultiBlockDataSet")
  {
    dataSetType = VTK_MULTIBLOCK_DATA_SET;
  }
  else
  {
    vtkErrorWithObjectMacro(nullptr, "Unknown data set type: " << typeName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFUtilities::GetStringAttribute(
  hid_t groupID, const std::string& name, std::string& attribute)
{
  if (!H5Aexists(groupID, name.c_str()))
  {
    vtkErrorWithObjectMacro(nullptr, "Attribute '" << name << "' not found.");
    return false;
  }
  vtkHDF::ScopedH5AHandle typeAttributeHID = H5Aopen_name(groupID, name.c_str());
  if (typeAttributeHID < 0)
  {
    vtkErrorWithObjectMacro(nullptr, "Can't open '" << name << "' attribute.");
    return false;
  }

  vtkHDF::ScopedH5THandle hdfType = H5Aget_type(typeAttributeHID);
  if (hdfType == H5I_INVALID_HID)
  {
    vtkErrorWithObjectMacro(nullptr, "Invalid type when reading " << name << " attribute.");
    return false;
  }

  H5T_class_t attributeClass = H5Tget_class(hdfType);
  if (attributeClass != H5T_STRING)
  {
    vtkErrorWithObjectMacro(nullptr, "Can't get class type of attribute.");
    return false;
  }

  H5T_cset_t characterType = H5Tget_cset(hdfType);
  if (characterType != H5T_CSET_ASCII && characterType != H5T_CSET_UTF8)
  {
    vtkErrorWithObjectMacro(
      nullptr, "Not an ASCII or UTF-8 string character type: " << characterType);
    return false;
  }

  hsize_t stringLength = H5Aget_storage_size(typeAttributeHID);
  if (stringLength < 1 || stringLength > 32)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Wrong length of " << name << " attribute (expected between 1 and 32): " << stringLength);
    return false;
  }

  if (H5Tis_variable_str(hdfType) > 0)
  {
    char* buffer = nullptr;
    if (H5Aread(typeAttributeHID, hdfType, &buffer) < 0)
    {
      vtkErrorWithObjectMacro(
        nullptr, "H5Aread failed while reading " << name << " attribute (variable-length)");
      return false;
    }
    attribute = std::string(buffer);
    H5free_memory(buffer);
  }
  else if (H5Tis_variable_str(hdfType) == 0)
  {
    std::array<char, 64> buffer;
    if (H5Aread(typeAttributeHID, hdfType, buffer.data()) < 0)
    {
      vtkErrorWithObjectMacro(
        nullptr, "H5Aread failed while reading " << name << " attribute (fixed-length)");
      return false;
    }
    attribute = std::string(buffer.data(), stringLength);
  }
  else
  {
    vtkErrorWithObjectMacro(
      nullptr, "H5Tis_variable_str failed while reading " << name << " attribute");
    return false;
  }

  // Handle null-terminated strings
  attribute.erase(std::find(attribute.begin(), attribute.end(), '\0'), attribute.end());

  return true;
}

//------------------------------------------------------------------------------
hid_t vtkHDFUtilities::getH5TypeFromVtkType(int dataType)
{
  switch (dataType)
  {
    case VTK_DOUBLE:
      return H5T_NATIVE_DOUBLE;

    case VTK_FLOAT:
      return H5T_NATIVE_FLOAT;

#if VTK_ID_TYPE_IMPL == VTK_LONG_LONG
    case VTK_ID_TYPE:
#endif
    case VTK_LONG_LONG:
      return H5T_NATIVE_LLONG;

    case VTK_UNSIGNED_LONG_LONG:
      return H5T_NATIVE_ULLONG;

#if VTK_ID_TYPE_IMPL == VTK_LONG
    case VTK_ID_TYPE:
#endif
    case VTK_LONG:
      return H5T_NATIVE_LONG;

    case VTK_UNSIGNED_LONG:
      return H5T_NATIVE_ULONG;

#if VTK_ID_TYPE_IMPL == VTK_INT
    case VTK_ID_TYPE:
#endif
    case VTK_INT:
      return H5T_NATIVE_INT;

    case VTK_UNSIGNED_INT:
      return H5T_NATIVE_UINT;

    case VTK_SHORT:
      return H5T_NATIVE_SHORT;

    case VTK_UNSIGNED_SHORT:
      return H5T_NATIVE_USHORT;

    case VTK_CHAR:
      return H5T_NATIVE_CHAR;

    case VTK_SIGNED_CHAR:
      return H5T_NATIVE_SCHAR;

    case VTK_UNSIGNED_CHAR:
      return H5T_NATIVE_UCHAR;

    case VTK_STRING:
      return H5T_C_S1;

    default:
      return H5I_INVALID_HID;
  }
}

//------------------------------------------------------------------------------
void vtkHDFUtilities::MakeObjectNameValid(std::string& objectName)
{
  bool containASlash = objectName.find('/') != std::string::npos;
  bool containADot = objectName.find('.') != std::string::npos;

  if (containASlash || containADot)
  {
    vtkLog(WARNING,
      "Array name : " + objectName +
        " contains illegal character (slash or dot) in hdf5. These characters will be replaced by "
        "an underscore.");
  }

  if (containASlash)
  {
    std::replace(objectName.begin(), objectName.end(), '/', '_');
  }

  if (containADot)
  {
    std::replace(objectName.begin(), objectName.end(), '.', '_');
  }
}

//------------------------------------------------------------------------------
bool vtkHDFUtilities::Open(const char* fileName, hid_t& fileID)
{
  if (!fileName)
  {
    vtkErrorWithObjectMacro(nullptr, "fileName is empty.");
    return false;
  }

  fileID = H5Fopen(fileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fileID < 0)
  {
    // we try to read a non-HDF file
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkHDFUtilities::NewStringArray(
  hid_t dataset, std::vector<hsize_t> dims, std::vector<hsize_t> fileExtent)
{
  hsize_t size = dims[0];
  std::vector<char*> rdata(size);

  /*
   * Create the memory datatype.
   */
  hid_t memtype = H5Tcopy(H5T_C_S1);
  if (H5Tset_size(memtype, H5T_VARIABLE) < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Tset_size");
    return nullptr;
  }

  hsize_t numberOfComponents = 0;
  if (dims.size() == (fileExtent.size() / 2))
  {
    numberOfComponents = 1;
  }
  else
  {
    numberOfComponents = dims.back();
    if (dims.size() > (fileExtent.size() / 2) + 1)
    {
      vtkErrorWithObjectMacro(
        nullptr, << "Dataset: ndims: " << dims.size()
                 << " greater than expected ndims: " << (fileExtent.size() / 2) << " plus one.");
    }
    if (numberOfComponents == 1)
    {
      fileExtent.resize(dims.size() * 2, 0);
      fileExtent[fileExtent.size() - 1] = numberOfComponents;
    }
  }

  std::vector<hsize_t> count(fileExtent.size() / 2), start(fileExtent.size() / 2);
  for (size_t i = 0; i < count.size(); ++i)
  {
    count[i] = fileExtent[i * 2 + 1] - fileExtent[i * 2];
    start[i] = fileExtent[i * 2];
  }

  // make sure to read the whole row in case of non 1D array
  if (numberOfComponents > 1)
  {
    count.push_back(numberOfComponents);
    start.push_back(0);
  }

  // create the filespace and select the required fileExtent
  vtkHDF::ScopedH5SHandle filespace = H5Dget_space(dataset);
  if (filespace < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Dget_space for array");
  }
  if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr) <
    0)
  {
    vtkErrorWithObjectMacro(nullptr, << "error when trying to read the hyperslab");
  }

  vtkHDF::ScopedH5SHandle memspace =
    H5Screate_simple(static_cast<int>(count.size()), count.data(), nullptr);
  if (memspace < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Screate_simple for memory space");
    return nullptr;
  }
  if (H5Dread(dataset, memtype, memspace, filespace, H5P_DEFAULT, rdata.data()) < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "Error H5Dread");
  }

  auto array = vtkStringArray::New();
  array->SetNumberOfTuples(size);
  for (size_t i = 0; i < size; ++i)
  {
    array->SetValue(i, rdata[i]);
  }

  return array;
}

//------------------------------------------------------------------------------
std::size_t vtkHDFUtilities::GetNumberOfSteps(hid_t groupID)
{
  if (groupID < 0)
  {
    vtkErrorWithObjectMacro(nullptr, "Cannot get number of steps if the group is not open");
    return 0;
  }

  if (H5Lexists(groupID, "Steps", H5P_DEFAULT) <= 0)
  {
    // Steps group does not exist and so there is only 1 step
    return 1;
  }

  // Steps group does exist
  vtkHDF::ScopedH5GHandle steps = H5Gopen(groupID, "Steps", H5P_DEFAULT);
  if (steps < 0)
  {
    vtkErrorWithObjectMacro(nullptr, "Could not open steps group");
    return 1;
  }

  int nSteps = 1;
  vtkHDFUtilities::GetAttribute(steps, "NSteps", 1, &nSteps);
  return nSteps > 0 ? static_cast<std::size_t>(nSteps) : 1;
}

//------------------------------------------------------------------------------
std::vector<hsize_t> vtkHDFUtilities::GetDimensions(hid_t fileID, const char* datasetName)
{
  std::vector<hsize_t> dims;

  vtkHDF::ScopedH5DHandle dataset = H5Dopen(fileID, datasetName, H5P_DEFAULT);
  if (dataset < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot open ") + datasetName);
    return dims;
  }

  vtkHDF::ScopedH5SHandle dataspace = H5Dget_space(dataset);
  if (dataspace < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot get space for dataset ") + datasetName);
    return dims;
  }

  int rank = H5Sget_simple_extent_ndims(dataspace);
  if (rank < 0)
  {
    vtkErrorWithObjectMacro(
      nullptr, << std::string(datasetName) + " dataset: get_simple_extent_ndims error");
    return dims;
  }

  if (rank > 0)
  {
    dims.resize(rank, 0);
    if (H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr) < 0)
    {
      vtkErrorWithObjectMacro(nullptr, << std::string("Cannot find dimension for ") + datasetName);
      dims.clear();
      return dims;
    }
  }

  return dims;
}

//------------------------------------------------------------------------------
bool vtkHDFUtilities::RetrieveHDFInformation(hid_t& fileID, hid_t& groupID,
  const std::string& rootName, std::array<int, 2>& version, int& dataSetType, int& numberOfPieces,
  std::array<hid_t, 3>& attributeDataGroup)
{
  // turn off error logging and save error function
  H5E_auto_t f;
  void* client_data;
  H5Eget_auto(H5E_DEFAULT, &f, &client_data);
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

  bool error = false;
  if ((groupID = H5Gopen(fileID, rootName.c_str(), H5P_DEFAULT)) < 0)
  {
    // we try to read a non-VTKHDF file
    return false;
  }

  H5Eset_auto(H5E_DEFAULT, f, client_data);
  if (!vtkHDFUtilities::ReadDataSetType(groupID, dataSetType))
  {
    return false;
  }
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

  std::fill(attributeDataGroup.begin(), attributeDataGroup.end(), -1);
  std::fill(version.begin(), version.end(), 0);

  std::array<const char*, 3> groupNames = { "/PointData", "/CellData", "/FieldData" };

  if (dataSetType == VTK_OVERLAPPING_AMR)
  {
    groupNames = { "/Level0/PointData", "/Level0/CellData", "/Level0/FieldData" };
  }

  // try to open cell or point group. Its OK if they don't exist.
  for (size_t i = 0; i < attributeDataGroup.size(); ++i)
  {
    std::string path = rootName + groupNames[i];
    attributeDataGroup[i] = H5Gopen(fileID, path.c_str(), H5P_DEFAULT);
  }
  // turn on error logging and restore error function
  H5Eset_auto(H5E_DEFAULT, f, client_data);
  if (!vtkHDFUtilities::GetAttribute(groupID, "Version", version.size(), version.data()))
  {
    return false;
  }

  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
  // get temporal information if there is any
  vtkIdType nSteps = vtkHDFUtilities::GetNumberOfSteps(groupID);
  H5Eset_auto(H5E_DEFAULT, f, client_data);

  try
  {
    if (dataSetType == VTK_UNSTRUCTURED_GRID || dataSetType == VTK_POLY_DATA ||
      dataSetType == VTK_HYPER_TREE_GRID)
    {
      std::string datasetName;
      if (dataSetType == VTK_HYPER_TREE_GRID)
      {
        datasetName = rootName + "/NumberOfTrees";
      }
      else
      {
        datasetName = rootName + "/NumberOfPoints";
      }
      std::vector<hsize_t> dims = vtkHDFUtilities::GetDimensions(fileID, datasetName.c_str());
      if (dims.size() != 1)
      {
        throw std::runtime_error(datasetName + " dataset should have 1 dimension");
      }
      // Case where the data set has the same number of pieces for  all steps in the dataset
      numberOfPieces = dims[0] / nSteps;
    }
    else if (dataSetType == VTK_IMAGE_DATA || dataSetType == VTK_OVERLAPPING_AMR)
    {
      numberOfPieces = 1;
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorWithObjectMacro(nullptr, << e.what());
    error = true;
  }

  return !error;
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkHDFUtilities::GetArrayNames(
  const std::array<hid_t, 3>& attributeDataGroup, int attributeType)
{
  std::vector<std::string> array;
  hid_t group = attributeDataGroup[attributeType];
  if (group > 0)
  {
    // H5_INDEX_CRT_ORDER failed with: no creation order index to query
    H5Literate(group, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, ::AddName, &array);
  }
  return array;
}

//-----------------------------------------------------------------------------
herr_t vtkHDFUtilities::FileInfoCallBack(
  hid_t vtkNotUsed(loc_id), const char* name, const H5L_info_t* vtkNotUsed(info), void* opdata)
{
  std::vector<std::string>* names = reinterpret_cast<std::vector<std::string>*>(opdata);
  assert(names != nullptr);
  names->emplace_back(name);
  return 0;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkHDFUtilities::GetOrderedChildrenOfGroup(
  hid_t groupID, const std::string& path)
{
  vtkHDF::ScopedH5GHandle pathHandle = H5Gopen(groupID, path.c_str(), H5P_DEFAULT);
  std::vector<std::string> childrenPath;
  H5Literate_by_name(pathHandle, path.c_str(), H5_INDEX_CRT_ORDER, H5_ITER_INC, nullptr,
    vtkHDFUtilities::FileInfoCallBack, &childrenPath, H5P_DEFAULT);
  return childrenPath;
}

//------------------------------------------------------------------------------
hid_t vtkHDFUtilities::OpenDataSet(
  hid_t group, const char* name, hid_t* nativeType, std::vector<hsize_t>& dims)
{
  hid_t dataset = H5Dopen(group, name, H5P_DEFAULT);
  if (dataset < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot open ") + name);
    return -1;
  }

  vtkHDF::ScopedH5THandle datatype = H5Dget_type(dataset);
  if (datatype < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot get_type for dataset: ") + name);
    return -1;
  }

  vtkHDF::ScopedH5SHandle dataspace = H5Dget_space(dataset);
  if (dataspace < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot get space for dataset ") + name);
    return -1;
  }

  if ((*nativeType = H5Tget_native_type(datatype, H5T_DIR_ASCEND)) < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot get type for dataset ") + name);
    return -1;
  }
  int ndims = H5Sget_simple_extent_ndims(dataspace);
  if (ndims <= 0)
  {
    vtkErrorWithObjectMacro(
      nullptr, << std::string(name) + " dataset: get_simple_extent_ndims error");
    return -1;
  }
  dims.resize(ndims);
  if (H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr) < 0)
  {
    vtkErrorWithObjectMacro(nullptr, << std::string("Cannot find dimension for ") + name);
    return -1;
  }
  return dataset;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFUtilities::NewArrayForGroup(hid_t dataset, hid_t nativeType,
  const std::vector<hsize_t>& dims, const std::vector<hsize_t>& parameterExtent)
{
  vtkDataArray* array = nullptr;
  try
  {
    // used for field arrays
    std::vector<hsize_t> extent = parameterExtent;
    if (extent.empty())
    {
      extent.resize(2, 0);
      extent[1] = dims[0];
      if (dims.size() > 2)
      {
        throw std::runtime_error("Field arrays cannot have more than 2 dimensions.");
      }
    }

    if (dims.size() < (extent.size() >> 1))
    {
      std::ostringstream ostr;
      ostr << "Dataset: Expecting ndims >= " << (extent.size() >> 1) << ", got: " << dims.size();
      throw std::runtime_error(ostr.str());
    }

    hsize_t numberOfComponents = 0;
    if (dims.size() == (extent.size() >> 1))
    {
      numberOfComponents = 1;
    }
    else
    {
      numberOfComponents = dims[dims.size() - 1];
      if (dims.size() > (extent.size() >> 1) + 1)
      {
        std::ostringstream ostr;
        ostr << "Dataset: ndims: " << dims.size()
             << " greater than expected ndims: " << (extent.size() >> 1) << " plus one.";
        throw std::runtime_error(ostr.str());
      }
      if (numberOfComponents == 1)
      {
        extent.resize(dims.size() * 2, 0);
        extent[extent.size() - 1] = numberOfComponents;
      }
    }
    ::ArrayReader* builder = ::GetArrayBuilder(nativeType);
    if (!builder)
    {
      vtkErrorWithObjectMacro(nullptr, "Unknown native datatype: " << nativeType);
    }
    else
    {
      array = builder(dataset, extent, numberOfComponents);
    }
  }
  catch (const std::exception& e)
  {
    vtkGenericWarningMacro(<< e.what());
  }

  return array;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFUtilities::NewArrayForGroup(
  hid_t group, const char* name, const std::vector<hsize_t>& parameterExtent)
{
  std::vector<hsize_t> dims;
  hid_t tempNativeType = H5I_INVALID_HID;
  vtkHDF::ScopedH5DHandle dataset =
    vtkHDFUtilities::OpenDataSet(group, name, &tempNativeType, dims);
  vtkHDF::ScopedH5THandle nativeType = tempNativeType;
  if (dataset < 0)
  {
    return nullptr;
  }

  return vtkHDFUtilities::NewArrayForGroup(dataset, nativeType, dims, parameterExtent);
}

//------------------------------------------------------------------------------
std::vector<vtkIdType> vtkHDFUtilities::GetMetadata(
  hid_t group, const char* name, hsize_t size, hsize_t offset)
{
  std::vector<vtkIdType> v;
  std::vector<hsize_t> fileExtent = { offset, offset + size };
  auto array = vtk::TakeSmartPointer(vtkHDFUtilities::NewArrayForGroup(group, name, fileExtent));
  if (!array)
  {
    return v;
  }
  v.resize(array->GetNumberOfTuples() * array->GetNumberOfComponents());
  auto range = vtk::DataArrayValueRange(array);
  std::copy(range.begin(), range.end(), v.begin());
  return v;
}

//------------------------------------------------------------------------------
std::array<vtkIdType, 2> vtkHDFUtilities::GetFieldArraySize(
  hid_t group, vtkIdType step, std::string name)
{
  std::array<vtkIdType, 2> size = { -1, 1 };
  if (group < 0)
  {
    return size;
  }
  std::string path = "Steps";
  if (H5Lexists(group, path.c_str(), H5P_DEFAULT) <= 0)
  {
    return size;
  }
  path += "/FieldDataSizes";
  if (H5Lexists(group, path.c_str(), H5P_DEFAULT) <= 0)
  {
    return size;
  }
  path += "/" + name;
  if (H5Lexists(group, path.c_str(), H5P_DEFAULT) <= 0)
  {
    return size;
  }
  std::vector<vtkIdType> buffer = vtkHDFUtilities::GetMetadata(group, path.c_str(), 1, step);
  if (buffer.empty() || buffer.size() != 2)
  {
    return size;
  }
  size[0] = buffer[0];
  size[1] = buffer[1];

  return size;
}

//------------------------------------------------------------------------------
vtkIdType vtkHDFUtilities::GetArrayOffset(
  hid_t group, vtkIdType step, int attributeType, std::string name)
{
  if (group < 0)
  {
    return -1;
  }
  if (H5Lexists(group, "Steps", H5P_DEFAULT) <= 0)
  {
    return -1;
  }
  std::string path = "Steps/";
  path += ::ARRAY_OFFSET_GROUPS.at(attributeType);
  if (H5Lexists(group, path.c_str(), H5P_DEFAULT) <= 0)
  {
    return -1;
  }
  path += "/" + name;
  if (H5Lexists(group, path.c_str(), H5P_DEFAULT) <= 0)
  {
    return -1;
  }
  std::vector<vtkIdType> buffer = vtkHDFUtilities::GetMetadata(group, path.c_str(), 1, step);
  if (buffer.empty())
  {
    return -1;
  }
  return buffer[0];
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkHDFUtilities::NewFieldArray(const std::array<hid_t, 3>& attributeDataGroup,
  const char* name, vtkIdType offset, vtkIdType size, vtkIdType dimMaxSize)
{
  hid_t tempNativeType = H5I_INVALID_HID;
  std::vector<hsize_t> dims;
  vtkHDF::ScopedH5DHandle dataset = vtkHDFUtilities::OpenDataSet(
    attributeDataGroup[vtkDataObject::FIELD], name, &tempNativeType, dims);
  vtkHDF::ScopedH5THandle nativeType = tempNativeType;
  if (dataset < 0)
  {
    return nullptr;
  }

  // empty fileExtent means read all values from the file
  // field arrays are always 1D
  std::vector<hsize_t> fileExtent;
  if (offset >= 0 || size > 0)
  {
    // XXX(gcc-12): GCC 12 has some weird warning triggered here where
    // `fileExtent.resize()` warns about writing out-of-bounds. Instead,
    // just reserve ahead of time and push into the vector.
    fileExtent.reserve(2);
    fileExtent.push_back(offset);
    fileExtent.push_back(offset + size);
  }

  if (size > 0)
  {
    dims[0] = size;
  }

  if (dims.size() >= 2 && dimMaxSize > 0 && static_cast<int>(dims[1]) > dimMaxSize)
  {
    dims[1] = dimMaxSize;
  }

  ::TypeDescription td = ::GetTypeDescription(nativeType);
  if (td.Class == H5T_STRING)
  {
    vtkStringArray* array = nullptr;
    array = vtkHDFUtilities::NewStringArray(dataset, dims, fileExtent);
    array->SetName(name);

    return array;
  }

  return vtkHDFUtilities::NewArrayForGroup(dataset, nativeType, dims, fileExtent);
}

VTK_ABI_NAMESPACE_END
