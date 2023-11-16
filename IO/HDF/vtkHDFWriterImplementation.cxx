// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFWriterImplementation.h"

#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFVersion.h"

#include "vtk_hdf5.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
bool vtkHDFWriter::Implementation::WriteHeader(const char* hdfType)
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
    this->Root, "Type", typeOfTypeAttr, scalarSpaceAttribute, utf8PropertyList, H5P_DEFAULT) };
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
    H5Acreate(this->Root, "Version", H5T_STD_I64LE, versionDataspace, H5P_DEFAULT, H5P_DEFAULT);
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
bool vtkHDFWriter::Implementation::OpenRoot(bool overwrite)
{
  const char* filename = this->Writer->GetFileName();
  // Create file
  vtkHDF::ScopedH5FHandle file{ H5Fcreate(
    filename, overwrite ? H5F_ACC_TRUNC : H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT) };
  if (file == H5I_INVALID_HID)
  {
    this->LastError = "Can not create file";
    return false;
  }

  // Create the root group
  vtkHDF::ScopedH5GHandle root{ H5Gcreate(file, "VTKHDF", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
  if (root == H5I_INVALID_HID)
  {
    this->LastError = "Can not create root group";
    return false;
  }

  this->File = std::move(file);
  this->Root = std::move(root);
  return true;
}

//------------------------------------------------------------------------------
std::vector<vtkHDFWriter::Implementation::PolyDataTopos>
vtkHDFWriter::Implementation::getCellArraysForTopos(vtkPolyData* polydata)
{
  return { { "Polygons", polydata->GetPolys() }, { "Strips", polydata->GetStrips() },
    { "Lines", polydata->GetLines() }, { "Vertices", polydata->GetVerts() } };
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::createAndWriteHdfDataset(hid_t group,
  hid_t type, hid_t source_type, const char* name, int rank, const hsize_t dimensions[],
  const void* data)
{
  // Create the dataspace, use the whole extent
  vtkHDF::ScopedH5SHandle dataspace = this->createSimpleDataspace(rank, dimensions);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Create the dataset from the dataspace and other arguments
  vtkHDF::ScopedH5DHandle dataset = this->createHdfDataset(group, name, type, dataspace);
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
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::createSimpleDataspace(
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
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::createHdfDataset(
  hid_t group, const char* name, hid_t type, hid_t dataspace)
{
  return H5Dcreate(group, name, type, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::createHdfDataset(
  hid_t group, const char* name, hid_t type, int rank, const hsize_t dimensions[])
{
  vtkHDF::ScopedH5SHandle dataspace = this->createSimpleDataspace(rank, dimensions);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  return this->createHdfDataset(group, name, type, dataspace);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5SHandle vtkHDFWriter::Implementation::createDataspaceFromArray(
  vtkAbstractArray* dataArray)
{
  const int nComp = dataArray->GetNumberOfComponents();
  const int nTuples = dataArray->GetNumberOfTuples();
  const hsize_t dimensions[] = { (hsize_t)nTuples, (hsize_t)nComp };
  const int rank = nComp > 1 ? 2 : 1;
  return createSimpleDataspace(rank, dimensions);
}

//------------------------------------------------------------------------------
vtkHDF::ScopedH5DHandle vtkHDFWriter::Implementation::createDatasetFromDataArray(
  hid_t group, const char* name, hid_t type, vtkAbstractArray* dataArray)
{
  // Create dataspace from array
  vtkHDF::ScopedH5SHandle dataspace = createDataspaceFromArray(dataArray);
  if (dataspace == H5I_INVALID_HID)
  {
    return H5I_INVALID_HID;
  }
  // Create dataset from dataspace and other arguments
  vtkHDF::ScopedH5DHandle dataset = this->createHdfDataset(group, name, type, dataspace);
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
vtkHDFWriter::Implementation::Implementation(vtkHDFWriter* writer)
  : Writer(writer)
  , File(H5I_INVALID_HID)
  , Root(H5I_INVALID_HID)
{
}

//------------------------------------------------------------------------------
vtkHDFWriter::Implementation::~Implementation() = default;

VTK_ABI_NAMESPACE_END
