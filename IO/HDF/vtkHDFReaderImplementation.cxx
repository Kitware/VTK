/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHDFReaderImplementation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHDFReaderImplementation.h"
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include "vtkCharArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkShortArray.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

//------------------------------------------------------------------------------
namespace
{
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
};

//------------------------------------------------------------------------------
vtkHDFReader::Implementation::TypeDescription vtkHDFReader::Implementation::GetTypeDescription(
  hid_t type)
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
vtkHDFReader::Implementation::Implementation(vtkHDFReader* reader)
  : File(-1)
  , VTKGroup(-1)
  , DataSetType(-1)
  , NumberOfPieces(-1)
  , Reader(reader)
{
  std::fill(this->AttributeDataGroup.begin(), this->AttributeDataGroup.end(), -1);
  std::fill(this->Version.begin(), this->Version.end(), 0);
}

//------------------------------------------------------------------------------
vtkHDFReader::Implementation::~Implementation()
{
  this->Close();
}

std::vector<hsize_t> vtkHDFReader::Implementation::GetDimensions(const char* datasetName)
{
  bool error = false;
  hid_t dataset = -1;
  hid_t dataspace = -1;
  std::vector<hsize_t> dims;
  try
  {
    if ((dataset = H5Dopen(this->File, datasetName, H5P_DEFAULT)) < 0)
    {
      throw std::runtime_error(std::string("Cannot open ") + datasetName);
    }
    if ((dataspace = H5Dget_space(dataset)) < 0)
    {
      throw std::runtime_error(std::string("Cannot get space for dataset ") + datasetName);
    }
    int rank;
    if ((rank = H5Sget_simple_extent_ndims(dataspace)) < 0)
    {
      throw std::runtime_error(
        std::string(datasetName) + " dataset: get_simple_extent_ndims error");
    }
    std::vector<hsize_t> dimsFile(rank);
    if (H5Sget_simple_extent_dims(dataspace, &dimsFile[0], nullptr) < 0)
    {
      throw std::runtime_error(std::string("Cannot find dimension for ") + datasetName);
    }
    dims = dimsFile;
  }
  catch (std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, << e.what());
    error = true;
  }
  if (dataspace >= 0)
  {
    error = H5Sclose(dataspace) < 0 || error;
  }
  if (dataset >= 0)
  {
    error = H5Dclose(dataset) < 0 || error;
  }
  return dims;
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::Open(const char* fileName)
{
  bool error = false;
  if (!fileName)
  {
    vtkErrorWithObjectMacro(this->Reader, "Invalid filename: " << fileName);
    return false;
  }
  if (this->FileName.empty() || this->FileName != fileName)
  {
    this->FileName = fileName;
    if (this->File >= 0)
    {
      this->Close();
    }
    // turn off error logging and save error function
    H5E_auto_t f;
    void* client_data;
    H5Eget_auto(H5E_DEFAULT, &f, &client_data);
    H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
    if ((this->File = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
    {
      // we try to read a non-HDF file
      return false;
    }
    if ((this->VTKGroup = H5Gopen(this->File, "/VTKHDF", H5P_DEFAULT)) < 0)
    {
      // we try to read a non-VTKHDF file
      return false;
    }
    std::array<const char*, 3> groupNames = { "/VTKHDF/PointData", "/VTKHDF/CellData",
      "/VTKHDF/FieldData" };
    // try to open cell or point group. Its OK if they don't exist.
    for (size_t i = 0; i < this->AttributeDataGroup.size(); ++i)
    {
      this->AttributeDataGroup[i] = H5Gopen(this->File, groupNames[i], H5P_DEFAULT);
    }
    // turn on error logging and restore error function
    H5Eset_auto(H5E_DEFAULT, f, client_data);
    if (!GetAttribute("Version", this->Version.size(), &this->Version[0]))
    {
      return false;
    }
    hid_t attr = -1;
    try
    {
      H5Eset_auto(H5E_DEFAULT, nullptr, nullptr); // errors off
      if ((attr = H5Aopen_name(this->VTKGroup, "WholeExtent")) < 0)
      {
        H5Eset_auto(H5E_DEFAULT, f, client_data); // errors on
        this->DataSetType = VTK_UNSTRUCTURED_GRID;
        const char* datasetName = "/VTKHDF/NumberOfPoints";
        std::vector<hsize_t> dims = this->GetDimensions(datasetName);
        if (dims.size() != 1)
        {
          throw std::runtime_error(std::string(datasetName) + " dataset should have 1 dimension");
        }
        this->NumberOfPieces = dims[0];
      }
      else
      {
        H5Eset_auto(H5E_DEFAULT, f, client_data); // errors on
        this->DataSetType = VTK_IMAGE_DATA;
        this->NumberOfPieces = 1;
      }
    }
    catch (const std::exception& e)
    {
      vtkErrorWithObjectMacro(this->Reader, << e.what());
      error = true;
    }
    if (attr >= 0)
    {
      error = H5Aclose(attr) < 0 || error;
    }
  }
  this->BuildTypeReaderMap();
  return !error;
}

//------------------------------------------------------------------------------
void vtkHDFReader::Implementation::Close()
{
  this->DataSetType = -1;
  this->NumberOfPieces = 0;
  std::fill(this->Version.begin(), this->Version.end(), 0);
  for (size_t i = 0; i < this->AttributeDataGroup.size(); ++i)
  {
    if (this->AttributeDataGroup[i] >= 0)
    {
      H5Gclose(this->AttributeDataGroup[i]);
      this->AttributeDataGroup[i] = -1;
    }
  }
  if (this->VTKGroup >= 0)
  {
    H5Gclose(this->VTKGroup);
    this->VTKGroup = -1;
  }
  if (this->File >= 0)
  {
    H5Fclose(this->File);
    this->File = -1;
  }
}

//------------------------------------------------------------------------------
void vtkHDFReader::Implementation::BuildTypeReaderMap()
{
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_CHAR)] =
    &vtkHDFReader::Implementation::NewArray<char>;
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_UCHAR)] =
    &vtkHDFReader::Implementation::NewArray<unsigned char>;
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_SHORT)] =
    &vtkHDFReader::Implementation::NewArray<short>;
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_USHORT)] =
    &vtkHDFReader::Implementation::NewArray<unsigned short>;
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_INT)] =
    &vtkHDFReader::Implementation::NewArray<int>;
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_UINT)] =
    &vtkHDFReader::Implementation::NewArray<unsigned int>;
  if (!this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_LONG)])
  {
    // long may be the same as int
    this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_LONG)] =
      &vtkHDFReader::Implementation::NewArray<long>;
    this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_ULONG)] =
      &vtkHDFReader::Implementation::NewArray<unsigned long>;
  }
  if (!this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_LLONG)])
  {
    // long long may be the same as long
    this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_LLONG)] =
      &vtkHDFReader::Implementation::NewArray<long long>;
    this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_ULLONG)] =
      &vtkHDFReader::Implementation::NewArray<unsigned long long>;
  }
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_FLOAT)] =
    &vtkHDFReader::Implementation::NewArray<float>;
  this->TypeReaderMap[this->GetTypeDescription(H5T_NATIVE_DOUBLE)] =
    &vtkHDFReader::Implementation::NewArray<double>;
}

//------------------------------------------------------------------------------
template <typename T>
hid_t vtkHDFReader::Implementation::TemplateTypeToHdfNativeType()
{
  if (std::is_same<T, char>::value)
  {
    return H5T_NATIVE_CHAR;
  }
  else if (std::is_same<T, unsigned char>::value)
  {
    return H5T_NATIVE_UCHAR;
  }
  else if (std::is_same<T, short>::value)
  {
    return H5T_NATIVE_SHORT;
  }
  else if (std::is_same<T, unsigned short>::value)
  {
    return H5T_NATIVE_USHORT;
  }
  else if (std::is_same<T, int>::value)
  {
    return H5T_NATIVE_INT;
  }
  else if (std::is_same<T, unsigned int>::value)
  {
    return H5T_NATIVE_UINT;
  }
  else if (std::is_same<T, long>::value)
  {
    return H5T_NATIVE_LONG;
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    return H5T_NATIVE_ULONG;
  }
  else if (std::is_same<T, long long>::value)
  {
    return H5T_NATIVE_LLONG;
  }
  else if (std::is_same<T, unsigned long long>::value)
  {
    return H5T_NATIVE_ULLONG;
  }
  else if (std::is_same<T, float>::value)
  {
    return H5T_NATIVE_FLOAT;
  }
  else if (std::is_same<T, double>::value)
  {
    return H5T_NATIVE_DOUBLE;
  }
  else
  {
    vtkErrorWithObjectMacro(this->Reader, "Invalid type: " << typeid(T).name());
    return -1;
  }
}

//------------------------------------------------------------------------------
template <typename T>
vtkDataArray* vtkHDFReader::Implementation::NewVtkDataArray()
{
  if (std::is_same<T, char>::value)
  {
    return vtkCharArray::New();
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
    vtkErrorWithObjectMacro(this->Reader, "Invalid type: " << typeid(T).name());
    return nullptr;
  }
}

//------------------------------------------------------------------------------
bool vtkHDFReader::Implementation::GetPartitionExtent(hsize_t partitionIndex, int* extent)
{
  hid_t dataset = -1;
  hid_t memspace = -1;
  hid_t dataspace = -1;
  const int RANK = 2;
  bool error = false;
  const char* datasetName = "/VTKHDF/Extents";
  try
  {
    // create the memory space
    hsize_t dimsm[RANK];
    dimsm[0] = 1;
    dimsm[1] = 6;
    if ((memspace = H5Screate_simple(RANK, dimsm, nullptr)) < 0)
    {
      throw std::runtime_error("Error H5Screate_simple for memory space");
    }
    // create the file dataspace + hyperslab
    if ((dataset = H5Dopen(this->File, datasetName, H5P_DEFAULT)) < 0)
    {
      throw std::runtime_error(std::string("Cannot open ") + datasetName);
    }
    hsize_t start[RANK] = { partitionIndex, 0 }, count[RANK] = { 1, 2 };

    if ((dataspace = H5Dget_space(dataset)) < 0)
    {
      throw std::runtime_error(std::string("Cannot get space for dataset ") + datasetName);
    }
    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, nullptr, count, nullptr) < 0)
    {
      throw std::runtime_error(std::string("Error selecting hyperslab for ") + datasetName);
    }
    // read hyperslab
    if (H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, extent) < 0)
    {
      throw std::runtime_error(std::string("Error reading hyperslab from ") + datasetName);
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, << e.what());
    error = true;
  }
  if (memspace >= 0)
  {
    error = H5Sclose(memspace) < 0 || error;
  }
  if (dataspace >= 0)
  {
    error = H5Sclose(dataspace) < 0 || error;
  }
  if (dataset >= 0)
  {
    error = H5Dclose(dataset) < 0 || error;
  }
  return !error;
}

//------------------------------------------------------------------------------
template <typename T>
bool vtkHDFReader::Implementation::GetAttribute(
  const char* attributeName, size_t numberOfElements, T* value)
{
  hid_t attr = -1;
  hid_t space = -1;
  bool error = false;
  try
  {
    if ((attr = H5Aopen_name(this->VTKGroup, attributeName)) < 0)
    {
      throw std::runtime_error(std::string(attributeName) + " attribute not found");
    }
    if ((space = H5Aget_space(attr)) < 0)
    {
      throw std::runtime_error(std::string(attributeName) + " attribute: get_space error");
    }
    int ndims;
    if ((ndims = H5Sget_simple_extent_ndims(space)) < 0)
    {
      throw std::runtime_error(
        std::string(attributeName) + " attribute: get_simple_extent_ndims error");
    }
    if (ndims != 1)
    {
      throw std::runtime_error(std::string(attributeName) + " attribute should have rank 1");
    }
    hsize_t ne;
    if (H5Sget_simple_extent_dims(space, &ne, nullptr) < 0)
    {
      throw std::runtime_error(std::string("Cannot find dimension for ") + attributeName);
    }

    if (ne != numberOfElements)
    {
      std::ostringstream ostr;
      ostr << attributeName << " attribute should have " << numberOfElements << " dimensions";
      throw std::runtime_error(ostr.str().c_str());
    }
    hid_t hdfType = 0;
    if ((hdfType = this->TemplateTypeToHdfNativeType<T>()) < 0)
    {
      throw std::runtime_error(std::string("Native type not implemented: ") + typeid(T).name());
    }
    if (H5Aread(attr, hdfType, value) < 0)
    {
      throw std::runtime_error(std::string("Error reading ") + attributeName + " attribute");
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, << e.what());
    error = true;
  }
  if (space >= 0)
  {
    error = H5Sclose(space) < 0 || error;
  }
  if (attr >= 0)
  {
    error = H5Aclose(attr) < 0 || error;
  }
  return !error;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkHDFReader::Implementation::GetArrayNames(int attributeType)
{
  std::vector<std::string> array;
  hid_t group = this->AttributeDataGroup[attributeType];
  if (group > 0)
  {
    // H5_INDEX_CRT_ORDER failed with: no creation order index to query
    H5Literate(group, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, AddName, &array);
  }
  return array;
}

//------------------------------------------------------------------------------
hid_t vtkHDFReader::Implementation::OpenDataSet(
  hid_t group, const char* name, hid_t* nativeType, std::vector<hsize_t>& dims)
{
  bool error = false;
  hid_t dataset = -1;
  hid_t dataspace = -1;
  hid_t datatype = -1;
  try
  {
    if ((dataset = H5Dopen(group, name, H5P_DEFAULT)) < 0)
    {
      throw std::runtime_error(std::string("Cannot open ") + name);
    }
    if ((datatype = H5Dget_type(dataset)) < 0)
    {
      throw std::runtime_error(std::string("Cannot get_type for dataset: ") + name);
    }
    if ((dataspace = H5Dget_space(dataset)) < 0)
    {
      throw std::runtime_error(std::string("Cannot get space for dataset ") + name);
    }
    if ((*nativeType = H5Tget_native_type(datatype, H5T_DIR_ASCEND)) < 0)
    {
      throw std::runtime_error(std::string("Cannot get type for dataset ") + name);
    }
    int ndims;
    if ((ndims = H5Sget_simple_extent_ndims(dataspace)) < 0)
    {
      throw std::runtime_error(std::string(name) + " dataset: get_simple_extent_ndims error");
    }
    dims.resize(ndims);
    if (H5Sget_simple_extent_dims(dataspace, &dims[0], nullptr) < 0)
    {
      throw std::runtime_error(std::string("Cannot find dimension for ") + name);
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, << e.what());
    error = true;
  }
  if (dataspace >= 0)
  {
    error = H5Sclose(dataspace) < 0 || error;
  }
  if (datatype >= 0)
  {
    error = H5Tclose(datatype) < 0 || error;
  }
  if (error && dataset >= 0)
  {
    H5Dclose(dataset);
    dataset = -1;
  }
  return dataset;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewArray(
  int attributeType, const char* name, const std::vector<hsize_t>& fileExtent)
{
  return NewArray(this->AttributeDataGroup[attributeType], name, fileExtent);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewArray(
  int attributeType, const char* name, hsize_t offset, hsize_t size)
{
  std::vector<hsize_t> fileExtent = { offset, offset + size - 1 };
  return NewArray(this->AttributeDataGroup[attributeType], name, fileExtent);
}

//------------------------------------------------------------------------------
vtkStringArray* vtkHDFReader::Implementation::NewStringArray(hid_t dataset, hsize_t size)
{
  std::vector<char*> rdata(size);

  /*
   * Create the memory datatype.
   */
  hid_t memtype = H5Tcopy(H5T_C_S1);
  if (H5Tset_size(memtype, H5T_VARIABLE) < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, << "Error H5Tset_size");
    return nullptr;
  }

  /*
   * Read the data.
   */
  if (H5Dread(dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &rdata[0]) < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, << "Error H5Dread");
  }

  auto array = vtkStringArray::New();
  array->SetNumberOfTuples(size);
  for (size_t i = 0; i < size; ++i)
  {
    array->SetValue(i, rdata[i]);
  }

  /*
   * Close and release resources.  Note that H5Dvlen_reclaim works
   * for variable-length strings as well as variable-length arrays.
   * Also note that we must still free the array of pointers stored
   * in rdata, as H5Tvlen_reclaim only frees the data these point to.
   */
  hid_t space = H5Dget_space(dataset);
  if (H5Dvlen_reclaim(memtype, space, H5P_DEFAULT, &rdata[0]) < 0)
  {
    vtkErrorWithObjectMacro(this->Reader, << "Error H5Dvlen_reclaim");
  }
  H5Sclose(space);
  return array;
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkHDFReader::Implementation::NewFieldArray(const char* name)
{
  hid_t dataset = -1;
  hid_t nativeType = -1;
  std::vector<hsize_t> dims;
  if ((dataset = this->OpenDataSet(
         this->AttributeDataGroup[vtkDataObject::FIELD], name, &nativeType, dims)) < 0)
  {
    return nullptr;
  }
  TypeDescription td = GetTypeDescription(nativeType);
  if (td.Class == H5T_STRING)
  {
    vtkStringArray* array = nullptr;
    if (dims.size() == 1)
    {
      array = this->NewStringArray(dataset, dims[0]);
    }
    else
    {
      vtkErrorWithObjectMacro(this->Reader, << "Error: String array expected "
                                               "dimensions one but got: "
                                            << dims.size());
    }
    H5Dclose(dataset);
    H5Tclose(nativeType);
    return array;
  }
  else
  {
    // empty fileExtent means read all values from the file
    // field arrays are always 1D
    std::vector<hsize_t> fileExtent;
    return NewArray(this->AttributeDataGroup[vtkDataObject::FIELD], name, fileExtent);
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewMetadataArray(
  const char* name, hsize_t offset, hsize_t size)
{
  std::vector<hsize_t> fileExtent = { offset, offset + size - 1 };
  return NewArray(this->VTKGroup, name, fileExtent);
}

//------------------------------------------------------------------------------
std::vector<vtkIdType> vtkHDFReader::Implementation::GetMetadata(const char* name, hsize_t size)
{
  std::vector<vtkIdType> v;
  std::vector<hsize_t> fileExtent = { 0, size - 1 };
  auto a = vtk::TakeSmartPointer(NewArray(this->VTKGroup, name, fileExtent));
  if (!a)
  {
    return v;
  }
  v.resize(a->GetNumberOfTuples());
  auto range = vtk::DataArrayValueRange<1>(a);
  std::copy(range.begin(), range.end(), v.begin());
  return v;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHDFReader::Implementation::NewArray(
  hid_t group, const char* name, const std::vector<hsize_t>& parameterExtent)
{
  hid_t dataset = -1;
  hid_t nativeType = -1;
  hsize_t numberOfComponents = 0;
  std::vector<hsize_t> dims;
  std::vector<hsize_t> extent = parameterExtent;
  if ((dataset = this->OpenDataSet(group, name, &nativeType, dims)) < 0)
  {
    return nullptr;
  }

  vtkDataArray* array = nullptr;
  try
  {
    // used for field arrays
    if (extent.empty())
    {
      extent.resize(2, 0);
      extent[1] = dims[0] - 1;
      if (dims.size() > 2)
      {
        throw std::runtime_error("Field arrays cannot have more than 2 dimensions.");
      }
    }
    if (dims.size() < (extent.size() >> 1))
    {
      std::ostringstream ostr;
      ostr << name << " dataset: Expecting ndims >= " << (extent.size() >> 1)
           << ", got: " << dims.size();
      throw std::runtime_error(ostr.str());
    }
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
        ostr << name << " dataset: ndims: " << dims.size()
             << " greater than expected ndims: " << (extent.size() >> 1) << " plus one.";
        throw std::runtime_error(ostr.str());
      }
    }
    auto it = this->TypeReaderMap.find(this->GetTypeDescription(nativeType));
    if (it == this->TypeReaderMap.end())
    {
      vtkErrorWithObjectMacro(this->Reader, "Unknown native datatype: " << nativeType);
    }
    else
    {
      array = (this->*(it->second))(dataset, extent, numberOfComponents);
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, << e.what());
  }
  H5Dclose(dataset);
  H5Tclose(nativeType);
  return array;
}

//------------------------------------------------------------------------------
template <typename T>
vtkDataArray* vtkHDFReader::Implementation::NewArray(
  hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents)
{
  int numberOfTuples = 1;
  size_t ndims = fileExtent.size() >> 1;
  for (size_t i = 0; i < ndims; ++i)
  {
    size_t j = i << 1;
    numberOfTuples *= (fileExtent[j + 1] - fileExtent[j] + 1);
  }
  auto array = vtkAOSDataArrayTemplate<T>::SafeDownCast(NewVtkDataArray<T>());
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfTuples);
  T* data = array->GetPointer(0);
  if (!this->NewArray(dataset, fileExtent, numberOfComponents, data))
  {
    array->Delete();
    array = nullptr;
  }
  return array;
}

//------------------------------------------------------------------------------
template <typename T>
bool vtkHDFReader::Implementation::NewArray(
  hid_t dataset, const std::vector<hsize_t>& fileExtent, hsize_t numberOfComponents, T* data)
{
  hid_t nativeType = TemplateTypeToHdfNativeType<T>();
  hid_t memspace = -1;
  hid_t filespace = -1;
  bool error = false;
  try
  {
    // create the memory space, reverse axis order for VTK fortran order
    std::vector<hsize_t> count(fileExtent.size() >> 1), start(fileExtent.size() >> 1);
    for (size_t i = 0; i < count.size(); ++i)
    {
      size_t j = (count.size() - 1 - i) << 1;
      count[i] = fileExtent[j + 1] - fileExtent[j] + 1;
      start[i] = fileExtent[j];
    }
    if (numberOfComponents > 1)
    {
      count.push_back(numberOfComponents);
      start.push_back(0);
    }
    if ((memspace = H5Screate_simple(static_cast<int>(count.size()), &count[0], nullptr)) < 0)
    {
      throw std::runtime_error("Error H5Screate_simple for memory space");
    }
    // create the filespace and select the required fileExtent
    if ((filespace = H5Dget_space(dataset)) < 0)
    {
      throw std::runtime_error("Error H5Dget_space for imagedata");
    }
    if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &start[0], nullptr, &count[0], nullptr) < 0)
    {
      std::ostringstream ostr;
      std::ostream_iterator<int> oi(ostr, " ");
      ostr << "Error selecting hyperslab, \nstart: ";
      std::copy(start.begin(), start.end(), oi);
      ostr << "\ncount: ";
      std::copy(count.begin(), count.end(), oi);
      throw std::runtime_error(ostr.str());
    }

    // read hyperslab
    if (H5Dread(dataset, nativeType, memspace, filespace, H5P_DEFAULT, data) < 0)
    {
      std::ostringstream ostr;
      ostr << "Error H5Dread "
           << "start: " << start[0] << ", " << start[1] << ", " << start[2]
           << " count: " << count[0] << ", " << count[1] << ", " << count[2];
      throw std::runtime_error(ostr.str());
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorWithObjectMacro(this->Reader, << e.what());
    error = true;
  }
  if (memspace >= 0)
  {
    error = H5Sclose(memspace) < 0 || error;
  }
  if (filespace >= 0)
  {
    error = H5Sclose(filespace) < 0 || error;
  }
  return !error;
}

//------------------------------------------------------------------------------
// explicit template instantiation
template bool vtkHDFReader::Implementation::GetAttribute<int>(
  const char* attributeName, size_t dim, int* value);
template bool vtkHDFReader::Implementation::GetAttribute<double>(
  const char* attributeName, size_t dim, double* value);
