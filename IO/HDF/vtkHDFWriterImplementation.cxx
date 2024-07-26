// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFWriterImplementation.h"

#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFVersion.h"

#include "vtk_hdf5.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteHeader(hid_t group, const char* hdfType)
{
  // Write type attribute to root
  std::string strType{ hdfType };
  vtkHDF::ScopedH5SHandle scalarSpaceAttribute{ H5Screate(H5S_SCALAR) };
  if (scalarSpaceAttribute == H5I_INVALID_HID)
  {
    return false;
  }
  vtkHDF::ScopedH5PHandle utf8PropertyList{ H5Pcreate(H5P_ATTRIBUTE_CREATE) };
  if (utf8PropertyList == H5I_INVALID_HID)
  {
    return false;
  }
  if (H5Pset_char_encoding(utf8PropertyList, H5T_CSET_UTF8) < 0)
  {
    return false;
  }
  vtkHDF::ScopedH5THandle typeOfTypeAttr = H5Tcreate(H5T_STRING, strType.length());
  if (typeOfTypeAttr == H5I_INVALID_HID)
  {
    return false;
  }
  vtkHDF::ScopedH5AHandle typeAttribute{ H5Acreate(
    group, "Type", typeOfTypeAttr, scalarSpaceAttribute, utf8PropertyList, H5P_DEFAULT) };
  if (typeAttribute == H5I_INVALID_HID)
  {
    return false;
  }
  if (H5Awrite(typeAttribute, typeOfTypeAttr, strType.c_str()) < 0)
  {
    return false;
  }

  // Write version attribute to root
  hsize_t versionDimensions[1] = { 2 };
  int versionBuffer[2] = { vtkHDFMajorVersion, vtkHDFMinorVersion };
  vtkHDF::ScopedH5SHandle versionDataspace{ H5Screate(H5S_SIMPLE) };
  if (versionDataspace == H5I_INVALID_HID)
  {
    return false;
  }
  if (H5Sset_extent_simple(versionDataspace, 1, versionDimensions, versionDimensions) < 0)
  {
    return false;
  }
  vtkHDF::ScopedH5AHandle versionAttribute =
    H5Acreate(group, "Version", H5T_STD_I64LE, versionDataspace, H5P_DEFAULT, H5P_DEFAULT);
  if (versionAttribute == H5I_INVALID_HID)
  {
    return false;
  }
  if (H5Awrite(versionAttribute, H5T_NATIVE_INT, versionBuffer) < 0)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateFile(bool overwrite, const std::string& filename)
{
  // Create file
  vtkDebugWithObjectMacro(
    this->Writer, << "Creating file " << this->Writer->Rank << ": " << filename);

  vtkHDF::ScopedH5FHandle file{ H5Fcreate(
    filename.c_str(), overwrite ? H5F_ACC_TRUNC : H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT) };
  if (file == H5I_INVALID_HID)
  {
    return false;
  }

  // Create the root group
  vtkHDF::ScopedH5GHandle root = this->CreateHdfGroupWithLinkOrder(file, "VTKHDF");
  if (root == H5I_INVALID_HID)
  {
    return false;
  }

  this->File = std::move(file);
  this->Root = std::move(root);

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::OpenFile()
{
  const char* filename = this->Writer->GetFileName();
  vtkDebugWithObjectMacro(
    this->Writer, << "Opening file on rank" << this->Writer->Rank << ": " << filename);

  vtkHDF::ScopedH5FHandle file{ H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT) };
  if (file == H5I_INVALID_HID)
  {
    return false;
  }

  this->File = std::move(file);
  this->Root = this->OpenExistingGroup(this->File, "VTKHDF");

  return this->Root != H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
void vtkHDFWriter::Implementation::CloseFile()
{
  vtkDebugWithObjectMacro(this->Writer,
    "Closing current file " << this->File << this->Writer->FileName << " on rank "
                            << this->Writer->Rank);

  // Setting to H5I_INVALID_HID closes the group/file using RAII
  this->StepsGroup = H5I_INVALID_HID;
  this->Root = H5I_INVALID_HID;
  this->File = H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::OpenSubfile(const std::string& filename)
{
  vtkDebugWithObjectMacro(
    this->Writer, << "Opening sub file on rank " << this->Writer->Rank << ": " << filename);

  vtkHDF::ScopedH5FHandle file{ H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT) };
  if (file == H5I_INVALID_HID)
  {
    return false;
  }

  this->Subfiles.emplace_back(std::move(file));
  this->SubfileNames.emplace_back(filename);

  return true;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::OpenExistingGroup(
  hid_t group, const char* name)
{
  vtkDebugWithObjectMacro(this->Writer, << "Opening group " << name);
  return H5Gopen(group, name, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::OpenDataset(hid_t group, const char* name)
{
  return H5Dopen(group, name, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
std::string vtkHDFWriter::Implementation::GetGroupName(hid_t group)
{
  size_t len = H5Iget_name(group, nullptr, 0);
  if (len == 0)
  {
    return std::string("");
  }
  std::string buffer;
  buffer.resize(len);
  H5Iget_name(group, &buffer.front(), len + 1);
  return buffer;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::CreateStepsGroup()
{
  vtkHDF::ScopedH5GHandle stepsGroup{ H5Gcreate(
    this->Root, "Steps", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
  if (stepsGroup == H5I_INVALID_HID)
  {
    return false;
  }

  this->StepsGroup = std::move(stepsGroup);
  return true;
}

//------------------------------------------------------------------------------
std::vector<vtkHDFWriter::Implementation::PolyDataTopos>
vtkHDFWriter::Implementation::GetCellArraysForTopos(vtkPolyData* polydata)
{
  return {
    { "Vertices", polydata->GetVerts() },
    { "Lines", polydata->GetLines() },
    { "Polygons", polydata->GetPolys() },
    { "Strips", polydata->GetStrips() },
  };
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateAndWriteHdfDataset(hid_t group,
  hid_t type, hid_t source_type, const char* name, int rank, const hsize_t dimensions[],
  const void* data)
{
  // Create the dataspace, use the whole extent
  vtkHDF::ScopedH5SHandle dataspace = this->CreateSimpleDataspace(rank, dimensions);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Create the dataset from the dataspace and other arguments
  vtkHDF::ScopedH5DHandle dataset = this->CreateHdfDataset(group, name, type, dataspace);
  if (dataset == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Write to the dataset
  if (data != nullptr && H5Dwrite(dataset, source_type, H5S_ALL, dataspace, H5P_DEFAULT, data) < 0)
  {
    return H5I_INVALID_HID;
  }
  return dataset;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::CreateSimpleDataspace(
  int rank, const hsize_t dimensions[])
{
  vtkHDF::ScopedH5SHandle dataspace{ H5Screate(H5S_SIMPLE) };
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  auto res = H5Sset_extent_simple(dataspace, rank, dimensions, dimensions);
  if (res < 0)
  {
    return H5I_INVALID_HID;
  }
  return dataspace;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5AHandle vtkHDFWriter::Implementation::CreateScalarAttribute(
  hid_t group, const char* name, int value)
{
  if (H5Aexists(group, name))
  {
    return H5Aopen_name(group, name);
  }

  vtkHDF::ScopedH5SHandle scalarSpaceAttribute{ H5Screate(H5S_SCALAR) };
  if (scalarSpaceAttribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  vtkHDF::ScopedH5AHandle nStepsAttribute{ H5Acreate(
    group, name, H5T_STD_I64LE, scalarSpaceAttribute, H5P_DEFAULT, H5P_DEFAULT) };
  if (nStepsAttribute == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  if (H5Awrite(nStepsAttribute, H5T_NATIVE_INT, &value) < 0)
  {
    return H5I_INVALID_HID;
  }

  return nStepsAttribute;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::CreateUnlimitedSimpleDataspace(
  hsize_t numCols)
{
  int numDims = (numCols == 1) ? 1 : 2;
  std::vector<hsize_t> dims{ 0 };
  std::vector<hsize_t> max_dims{ H5S_UNLIMITED };

  if (numDims == 2)
  {
    dims.emplace_back(numCols);
    max_dims.emplace_back(numCols);
  }

  vtkHDF::ScopedH5SHandle dataspace{ H5Screate_simple(numDims, dims.data(), max_dims.data()) };
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  return dataspace;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::CreateHdfGroup(hid_t group, const char* name)
{
  return H5Gcreate(group, name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5GHandle vtkHDFWriter::Implementation::CreateHdfGroupWithLinkOrder(
  hid_t group, const char* name)
{
  vtkHDF::ScopedH5PHandle plist = H5Pcreate(H5P_GROUP_CREATE);
  if (plist == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  H5Pset_link_creation_order(plist, H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED);
  return H5Gcreate(group, name, H5P_DEFAULT, plist, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
herr_t vtkHDFWriter::Implementation::CreateSoftLink(
  hid_t group, const char* groupName, const char* targetLink)
{
  return H5Lcreate_soft(targetLink, group, groupName, H5P_DEFAULT, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
herr_t vtkHDFWriter::Implementation::CreateExternalLink(
  hid_t group, const char* filename, const char* source, const char* targetLink)
{
  return H5Lcreate_external(filename, source, group, targetLink, H5P_DEFAULT, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateHdfDataset(
  hid_t group, const char* name, hid_t type, hid_t dataspace)
{
  return H5Dcreate(group, name, type, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateHdfDataset(
  hid_t group, const char* name, hid_t type, int rank, const hsize_t dimensions[])
{
  vtkHDF::ScopedH5SHandle dataspace = this->CreateSimpleDataspace(rank, dimensions);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  return this->CreateHdfDataset(group, name, type, dataspace);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateChunkedHdfDataset(hid_t group,
  const char* name, hid_t type, hid_t dataspace, hsize_t numCols, hsize_t chunkSize[],
  int compressionLevel)
{
  vtkHDF::ScopedH5PHandle plist = H5Pcreate(H5P_DATASET_CREATE);
  if (plist == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  H5Pset_layout(plist, H5D_CHUNKED);
  if (numCols == 1)
  {
    H5Pset_chunk(plist, 1, chunkSize);
  }
  else
  {
    H5Pset_chunk(plist, 2, chunkSize); // 2-Dimensional
  }

  if (compressionLevel != 0)
  {
    H5Pset_deflate(plist, compressionLevel);
  }

  vtkHDF::ScopedH5DHandle dset =
    H5Dcreate(group, name, type, dataspace, H5P_DEFAULT, plist, H5P_DEFAULT);
  if (dset == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  return dset;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::CreateDataspaceFromArray(
  vtkAbstractArray* dataArray)
{
  const int nComp = dataArray->GetNumberOfComponents();
  const int nTuples = dataArray->GetNumberOfTuples();
  const hsize_t dimensions[] = { (hsize_t)nTuples, (hsize_t)nComp };
  const int rank = nComp > 1 ? 2 : 1;
  return CreateSimpleDataspace(rank, dimensions);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateDatasetFromDataArray(
  hid_t group, const char* name, hid_t type, vtkAbstractArray* dataArray)
{
  // Create dataspace from array
  vtkHDF::ScopedH5SHandle dataspace = CreateDataspaceFromArray(dataArray);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Create dataset from dataspace and other arguments
  vtkHDF::ScopedH5DHandle dataset = this->CreateHdfDataset(group, name, type, dataspace);
  if (dataset == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Get the data pointer
  void* data = dataArray->GetVoidPointer(0);
  // If there is no data pointer, return either an invalid id or the dataset depending on the number
  // of values in the dataArray
  if (data == nullptr)
  {
    if (dataArray->GetNumberOfValues() == 0)
    {
      return dataset;
    }
    else
    {
      return H5I_INVALID_HID;
    }
  }
  // Find which HDF type corresponds to the dataArray type
  // It is different from the `type` in the argument list which defines which type should be used to
  // store the data in the HDF file
  hid_t source_type = vtkHDFUtilities::getH5TypeFromVtkType(dataArray->GetDataType());
  if (source_type == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Write vtkAbstractArray data to the HDF dataset
  if (H5Dwrite(dataset, source_type, H5S_ALL, dataspace, H5P_DEFAULT, data) < 0)
  {
    return H5I_INVALID_HID;
  }
  return dataset;
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateSingleValueDataset(
  hid_t group, const char* name, int value)
{
  hsize_t dimensions[1] = { 1 };
  return this->CreateAndWriteHdfDataset(
    group, H5T_STD_I64LE, H5T_NATIVE_INT, name, 1, dimensions, &value);
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddSingleValueToDataset(
  hid_t dataset, int value, bool offset, bool trim)
{
  vtkDebugWithObjectMacro(this->Writer, "Adding 1 value to " << this->GetGroupName(dataset));
  // Create a new dataspace containing a single value
  const hsize_t addedDims[1] = { 1 };
  vtkHDF::ScopedH5SHandle newDataspace = H5Screate_simple(1, addedDims, nullptr);
  if (newDataspace == H5I_INVALID_HID)
  {
    return false;
  }

  // Recover dataset and dataspace
  vtkHDF::ScopedH5SHandle currentDataspace = H5Dget_space(dataset);
  if (currentDataspace == H5I_INVALID_HID)
  {
    return false;
  }

  // Retrieve current dataspace dimensions
  hsize_t currentdims[1] = { 0 };
  H5Sget_simple_extent_dims(currentDataspace, currentdims, nullptr);
  const hsize_t newdims[1] = { currentdims[0] + addedDims[0] };

  // Add the last value of the dataset if we want an offset (only for arrays of stride 1)
  if (offset && currentdims[0] > 0)
  {
    std::vector<int> allValues;
    allValues.resize(currentdims[0]);
    H5Dread(dataset, H5T_NATIVE_INT, currentDataspace, H5S_ALL, H5P_DEFAULT, allValues.data());
    value += allValues.at(allValues.size() - 1);
  }

  // Resize dataset
  if (!trim)
  {
    if (H5Dset_extent(dataset, newdims) < 0)
    {
      return false;
    }
    currentDataspace = H5Dget_space(dataset);
    if (currentDataspace == H5I_INVALID_HID)
    {
      return false;
    }
  }
  hsize_t start[1] = { currentdims[0] - trim };
  hsize_t count[1] = { addedDims[0] };
  H5Sselect_hyperslab(currentDataspace, H5S_SELECT_SET, start, nullptr, count, nullptr);

  // Write new data to the dataset
  if (H5Dwrite(dataset, H5T_NATIVE_INT, newDataspace, currentDataspace, H5P_DEFAULT, &value) < 0)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddOrCreateSingleValueDataset(
  hid_t group, const char* name, int value, bool offset, bool trim)
{
  // Assume that when subfiles are set, we don't need to write data unless
  // SubFilesReady is set, which means all subfiles have been written.
  if (!this->Subfiles.empty())
  {
    if (this->SubFilesReady)
    {
      return this->CreateVirtualDataset(group, name, H5T_STD_I64LE, 1) != H5I_INVALID_HID;
    }
    return true;
  }

  if (!H5Lexists(group, name, H5P_DEFAULT))
  {
    // Dataset needs to be created
    return this->CreateSingleValueDataset(group, name, value) != H5I_INVALID_HID;
  }
  else
  {
    // Append the value to an existing dataset
    vtkHDF::ScopedH5DHandle dataset = H5Dopen(group, name, H5P_DEFAULT);
    return this->AddSingleValueToDataset(dataset, value, offset, trim);
  }
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddArrayToDataset(
  hid_t dataset, vtkAbstractArray* dataArray, int trim)
{
  // Create dataspace from array
  vtkHDF::ScopedH5SHandle dataspace = this->CreateDataspaceFromArray(dataArray);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  // Recover dataspace
  vtkHDF::ScopedH5SHandle currentDataspace = H5Dget_space(dataset);
  if (currentDataspace == H5I_INVALID_HID)
  {
    return false;
  }

  // Get raw array data
  void* rawArrayData = dataArray->GetVoidPointer(0);
  if (rawArrayData == nullptr)
  {
    if (dataArray->GetNumberOfValues() == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  // Retrieve current dataspace dimensions
  const int nComp = dataArray->GetNumberOfComponents();
  int nTuples = dataArray->GetNumberOfTuples();
  const int numDim = nComp == 1 ? 1 : 2;

  std::vector<hsize_t> addedDims{ (hsize_t)nTuples };
  std::vector<hsize_t> currentdims(numDim, 0);
  if (numDim == 2)
  {
    addedDims.emplace_back(nComp);
  }

  H5Sget_simple_extent_dims(currentDataspace, currentdims.data(), nullptr);
  std::vector<hsize_t> newdims = { currentdims[0] + addedDims[0] };
  if (numDim == 2)
  {
    newdims.emplace_back(currentdims[1]);

    if (currentdims[1] != addedDims[1])
    {
      return false; // Number of components don't match
    }
  }

  hid_t source_type = vtkHDFUtilities::getH5TypeFromVtkType(dataArray->GetDataType());
  if (source_type == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  if (addedDims[0] - trim > 0)
  {
    // Resize existing dataset to make space for the added array
    H5Dset_extent(dataset, newdims.data());
  }
  currentDataspace = H5Dget_space(dataset);
  if (currentDataspace == H5I_INVALID_HID)
  {
    return false;
  }
  std::vector<hsize_t> start{ currentdims[0] - trim };
  std::vector<hsize_t> count{ addedDims[0] };
  if (numDim == 2)
  {
    start.emplace_back(0);
    count.emplace_back(addedDims[1]);
  }
  H5Sselect_hyperslab(
    currentDataspace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr);

  // Write new data to the dataset
  if (H5Dwrite(dataset, source_type, dataspace, currentDataspace, H5P_DEFAULT, rawArrayData) < 0)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::AddOrCreateDataset(
  hid_t group, const char* name, hid_t type, vtkAbstractArray* dataArray)
{
  if (!this->Subfiles.empty())
  {
    if (this->SubFilesReady)
    {
      return this->CreateVirtualDataset(group, name, type, dataArray->GetNumberOfComponents()) !=
        H5I_INVALID_HID;
    }
    return true;
  }

  if (!H5Lexists(group, name, H5P_DEFAULT))
  {
    // Dataset needs to be created
    return this->CreateDatasetFromDataArray(group, name, type, dataArray) != H5I_INVALID_HID;
  }
  else
  {
    // Simply append the array to an existing dataset
    vtkHDF::ScopedH5DHandle dataset = H5Dopen(group, name, H5P_DEFAULT);
    return this->AddArrayToDataset(dataset, dataArray);
  }
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::CreateVirtualDataset(
  hid_t group, const char* name, hid_t type, int numComp)
{
  vtkDebugWithObjectMacro(
    this->Writer, "Creating virtual dataset " << name << " in " << this->GetGroupName(group));

  if (group == this->StepsGroup)
  {
    return this->WriteSumSteps(group, name, type);
  }

  const std::vector<std::string> singles{ "NumberOfPoints", "NumberOfCells",
    "NumberOfConnectivityIds" };
  bool single = std::find(singles.begin(), singles.end(), name) != singles.end();

  bool isOffset = false;
  if (name == std::string("Offsets"))
  {
    isOffset = true;
    vtkDebugWithObjectMacro(
      this->Writer, << name << " is offset array in group " << this->GetGroupName(group));
  }

  // Initialize VDS property
  vtkHDF::ScopedH5PHandle virtualSourceP = H5Pcreate(H5P_DATASET_CREATE);
  if (virtualSourceP == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }

  // Collect total dataset size
  const std::string datasetPath = this->GetGroupName(group) + "/" + name;
  hsize_t totalSize = 0;
  int count = 0;
  for (auto& fileRoot : this->Subfiles)
  {
    vtkDebugWithObjectMacro(
      this->Writer, "Opening dataset " << datasetPath << " in " << this->SubfileNames[count++]);
    vtkHDF::ScopedH5DHandle sourceDataset = H5Dopen(fileRoot, datasetPath.c_str(), H5P_DEFAULT);
    // if (sourceDataset == H5I_INVALID_HID) {
    //   return H5I_INVALID_HID;
    // }
    vtkHDF::ScopedH5SHandle sourceDataSpace = H5Dget_space(sourceDataset);
    std::vector<hsize_t> sourceDims(3);
    H5Sget_simple_extent_dims(sourceDataSpace, sourceDims.data(), nullptr);
    totalSize += sourceDims[0];
  }

  // Create destination dataspace with the final size
  std::vector<hsize_t> dspaceDims{ totalSize };
  const int numDim = numComp == 1 ? 1 : 2;
  if (numDim == 2)
  {
    dspaceDims.emplace_back(numComp);
  }
  vtkHDF::ScopedH5SHandle destSpace = H5Screate_simple(numDim, dspaceDims.data(), nullptr);

  vtkDebugWithObjectMacro(this->Writer, "Total VDS size: " << totalSize << "x" << numComp);

  // Add virtual mapping for each timestep file
  hsize_t mappingOffset = 0;
  std::vector<hsize_t> sourceOffsets(this->Subfiles.size(), 0);

  bool indexedOnPoints = false;
  if (this->GetGroupName(group) == std::string("/VTKHDF/PointData") ||
    name == std::string("Points"))
  {
    indexedOnPoints = true;
    vtkDebugWithObjectMacro(
      this->Writer, << name << " is indexed on points in group " << this->GetGroupName(group));
  }
  bool indexedOnCells = false;
  if (this->GetGroupName(group) == std::string("/VTKHDF/CellData") ||
    name == std::string("Offsets"))
  {
    indexedOnCells = true;
    vtkDebugWithObjectMacro(
      this->Writer, << name << " is indexed on cells in group " << this->GetGroupName(group));
  }
  bool indexedOnConnectivity = false;
  if (name == std::string("Connectivity"))
  {
    indexedOnConnectivity = true;
    vtkDebugWithObjectMacro(this->Writer,
      << name << " is indexed on connectivity in group " << this->GetGroupName(group));
  }

  int totalSteps = 1;
  if (this->Writer->IsTemporal &&
    (single || indexedOnPoints || indexedOnCells || indexedOnConnectivity)) // FIXME
  {
    totalSteps = this->Writer->NumberOfTimeSteps;
  }

  std::vector<hsize_t> prevPointOffsets(this->Subfiles.size(), -1);
  std::vector<hsize_t> prevCellOffsets(this->Subfiles.size(), -1);
  std::vector<hsize_t> prevConnectivityOffsets(this->Subfiles.size(), -1);

  for (int step = 0; step < totalSteps; step++)
  {
    vtkDebugWithObjectMacro(this->Writer, << "Writing step " << step);

    for (std::size_t part = 0; part < this->Subfiles.size(); part++)
    {
      vtkDebugWithObjectMacro(this->Writer, << "Writing part " << part);

      // Open source dataset/dataspace
      vtkHDF::ScopedH5DHandle sourceDataset =
        H5Dopen(this->Subfiles[part], datasetPath.c_str(), H5P_DEFAULT);
      vtkHDF::ScopedH5SHandle sourceDataSpace = H5Dget_space(sourceDataset);
      std::vector<hsize_t> sourceDims(3);
      H5Sget_simple_extent_dims(sourceDataSpace, sourceDims.data(), nullptr);

      std::vector<hsize_t> sourceOffset{ sourceOffsets[part] };
      std::vector<hsize_t> mappingSize{
        sourceDims[0]
      }; // By default, select the whole source dataset

      if (this->Writer->IsTemporal &&
        (single || indexedOnPoints || indexedOnCells || indexedOnConnectivity))
      {

        if (single)
        {
          // Select only one value in the source dataspace
          mappingSize[0] = 1;
          vtkDebugWithObjectMacro(this->Writer, << "Is Single");
        }
        else if (indexedOnPoints)
        {
          vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on points");

          // Find NumberOfPoints in source file
          hsize_t partNbPoints = this->GetSubfileNumberOf("/VTKHDF/NumberOfPoints", part, step);
          hsize_t partPointsOffset =
            this->GetSubfileNumberOf("/VTKHDF/Steps/PointOffsets", part, step);

          if (name == std::string("Points") && prevPointOffsets[part] == partPointsOffset)
          {
            vtkDebugWithObjectMacro(
              nullptr, << "Static mesh, not writing points virtual dataset again");
            continue;
          }
          prevPointOffsets[part] = partPointsOffset;

          // sourceOffset[0] += partNbPoints;
          mappingSize[0] = partNbPoints;
        }
        else if (indexedOnCells)
        {
          vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on cells");

          // Find NumberOfCells in source file
          hsize_t partNbCells = this->GetSubfileNumberOf("/VTKHDF/NumberOfCells", part, step);
          hsize_t partCellOffset =
            this->GetSubfileNumberOf("/VTKHDF/Steps/CellOffsets", part, step);

          if (name == std::string("Offsets") && prevCellOffsets[part] == partCellOffset)
          {
            vtkDebugWithObjectMacro(
              this->Writer, << "Static mesh, not writing virtual offsets again");
            continue;
          }

          // sourceOffset[0] += partNbCells;
          mappingSize[0] = partNbCells;

          // For N cells, we have N+1 offsets
          if (isOffset)
          {
            mappingSize[0]++;
            // partCellOffset++;
          }
          prevCellOffsets[part] = partCellOffset;
        }
        else if (indexedOnConnectivity)
        {
          vtkDebugWithObjectMacro(this->Writer, << "Is Indexed on conn");

          // Find NumberOfCells in source file
          hsize_t partNbCells =
            this->GetSubfileNumberOf("/VTKHDF/NumberOfConnectivityIds", part, step);
          hsize_t partConnOffset =
            this->GetSubfileNumberOf("/VTKHDF/Steps/ConnectivityIdOffsets", part, step);

          if (name == std::string("Connectivity") &&
            prevConnectivityOffsets[part] == partConnOffset)
          {
            vtkDebugWithObjectMacro(
              nullptr, << "Static mesh, not writing virtual connectivity Ids again");
            continue;
          }
          prevConnectivityOffsets[part] = partConnOffset;

          // sourceOffset[0] += part;
          mappingSize[0] = partNbCells;
        }
      }

      // Select hyperslab in source space of size 1
      if (numDim == 2)
      {
        sourceOffset.emplace_back(0);
        mappingSize.emplace_back(sourceDims[1]); // All components
      }

      // Select hyperslab in destination space
      std::vector<hsize_t> destinationOffset{ mappingOffset };
      if (numDim == 2)
      {
        destinationOffset.emplace_back(0);
      }
      if (H5Sselect_hyperslab(destSpace, H5S_SELECT_SET, destinationOffset.data(), nullptr,
            mappingSize.data(), nullptr) < 0)
      {
        return H5I_INVALID_HID;
      }

      vtkDebugWithObjectMacro(this->Writer,
        << "Build mapping of " << name << " from [" << sourceOffset[0] << "+" << mappingSize[0]
        << "] to [" << destinationOffset[0] << "+" << mappingSize[0] << "]");

      // Create mapping H5S
      vtkHDF::ScopedH5SHandle mappedDataSpace =
        H5Screate_simple(numDim, mappingSize.data(), nullptr);

      // Select Hyperslab on mapping
      H5Sselect_hyperslab(
        mappedDataSpace, H5S_SELECT_SET, sourceOffset.data(), nullptr, mappingSize.data(), nullptr);

      // Build the mapping
      if (H5Pset_virtual(virtualSourceP, destSpace, this->SubfileNames[part].c_str(),
            datasetPath.c_str(), mappedDataSpace) < 0)
      {
        return H5I_INVALID_HID;
      }

      mappingOffset += mappingSize[0];
      sourceOffsets[part] += mappingSize[0];
    }
  }

  // Create the virtual dataset
  vtkHDF::ScopedH5DHandle vdset =
    H5Dcreate(group, name, type, destSpace, H5P_DEFAULT, virtualSourceP, H5P_DEFAULT);
  return vdset;
}

vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::WriteSumSteps(
  hid_t group, const char* name, hid_t type)
{
  vtkDebugWithObjectMacro(
    this->Writer, "Creating steps sum " << name << " in " << this->GetGroupName(group));

  int size = this->Writer->NumberOfTimeSteps * this->Writer->NbProcs;

  const std::string datasetPath = this->GetGroupName(group) + "/" + name;
  vtkHDF::ScopedH5DHandle dataset = this->OpenDataset(group, name);

  // FIXME: bad
  // int startStep = 1;
  // std::string nameString{ name };
  // if (nameString.find("CellDataOffsets") != std::string::npos ||
  //   nameString.find("PointDataOffsets") != std::string::npos)
  // {
  //   startStep = 0;
  // }

  // Start at 1 because timestep 0 is already 0-initialized

  for (int step = 0; step < this->Writer->NumberOfTimeSteps; step++)
  {
    vtkDebugWithObjectMacro(this->Writer, "timestep " << step);
    int totalForTimeStep = 0;
    // TODO: foreach subfiles
    // Collect size for the current time step in each subfile
    for (std::size_t part = 0; part < this->Subfiles.size(); part++)
    {
      totalForTimeStep += this->GetSubfileNumberOf(datasetPath, part, step);
    }

    // Append the total value
    // if (step == 0)
    // {
    //   if (!this->CreateSingleValueDataset(group, name, totalForTimeStep)) {
    //     return H5I_INVALID_HID;
    //   }
    // }
    // else
    // {
    if (!this->AddSingleValueToDataset(dataset, totalForTimeStep, false, false))
    {
      return H5I_INVALID_HID;
    }
    // }
  }

  return dataset;
}

hsize_t vtkHDFWriter::Implementation::GetSubfileNumberOf(
  const std::string& qualifier, std::size_t subfileId, int part)
{
  vtkDebugWithObjectMacro(
    nullptr, << "Fetching " << qualifier << " for subfile " << subfileId << " for part " << part);
  // TODO: Error handling
  vtkHDF::ScopedH5DHandle sourceNumPoints =
    H5Dopen(this->Subfiles[subfileId], qualifier.c_str(), H5P_DEFAULT);
  std::array<hsize_t, 1> start{ static_cast<unsigned long>(part) }, count{ 1 }, numPoints{ 0 };
  vtkHDF::ScopedH5SHandle dataspace = H5Dget_space(sourceNumPoints);
  vtkHDF::ScopedH5SHandle destSpace = H5Screate_simple(1, count.data(), nullptr);
  H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr);
  H5Dread(sourceNumPoints, H5T_NATIVE_INT, destSpace, dataspace, H5P_DEFAULT, numPoints.data());
  vtkDebugWithObjectMacro(this->Writer, << "Fetched " << qualifier << ": " << numPoints[0]);
  return numPoints[0];
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::InitDynamicDataset(hid_t group, const char* name, hid_t type,
  hsize_t cols, hsize_t chunkSize[], int compressionLevel)
{
  // When writing data externally, don't create a dynamic dataset,
  // But create a virtual one based on the subfiles on the last step or partition.
  if (!this->Subfiles.empty() && group != this->StepsGroup)
  {
    return true;
  }

  vtkHDF::ScopedH5SHandle emptyDataspace = this->CreateUnlimitedSimpleDataspace(cols);
  if (emptyDataspace == H5I_INVALID_HID)
  {
    return false;
  }
  vtkHDF::ScopedH5DHandle dataset = this->CreateChunkedHdfDataset(
    group, name, type, emptyDataspace, cols, chunkSize, compressionLevel);
  return dataset != H5I_INVALID_HID;
}

//------------------------------------------------------------------------------
vtkHDFWriter::Implementation::Implementation(vtkHDFWriter* writer)
  : Writer(writer)
  , File(H5I_INVALID_HID)
  , Root(H5I_INVALID_HID)
{
}

//------------------------------------------------------------------------------
vtkHDFWriter::Implementation::~Implementation() = default;

VTK_ABI_NAMESPACE_END
