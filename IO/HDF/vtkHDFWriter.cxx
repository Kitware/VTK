// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFWriter.h"

#include "vtkAbstractArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkErrorCode.h"
#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFWriterImplementation.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include "vtk_hdf5.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHDFWriter);

//------------------------------------------------------------------------------
bool vtkHDFWriter::WritePolyDataToRoot(vtkPolyData* input)
{
  // Create file, root and write header
  if (!this->Impl->OpenRoot(this->Overwrite))
  {
    return false;
  }

  if (!this->Impl->WriteHeader("PolyData"))
  {
    return false;
  }

  if (!this->AppendNumberOfPoints(input))
  {
    return false;
  }

  if (!this->AppendPoints(input))
  {
    return false;
  }

  if (!this->AppendCells(input))
  {
    return false;
  }

  if (!this->AppendDataArrays(input))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfPoints(vtkPointSet* input)
{
  const hid_t root = this->Impl->GetRoot();
  const int nPoints = input->GetNumberOfPoints();
  hsize_t nPointsDimensions[1] = { 1 };
  const vtkHDF::ScopedH5DHandle result = this->Impl->createAndWriteHdfDataset(
    root, H5T_STD_I64LE, H5T_NATIVE_INT, "NumberOfPoints", 1, nPointsDimensions, &nPoints);
  if (result == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create NumberOfPoints dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendPoints(vtkPointSet* input)
{
  const hid_t root = this->Impl->GetRoot();
  const int nPoints = input->GetNumberOfPoints();
  hid_t dataset{ H5I_INVALID_HID };
  if (input->GetPoints() != nullptr && input->GetPoints()->GetData() != nullptr)
  {
    dataset = this->Impl->createDatasetFromDataArray(
      root, "Points", H5T_IEEE_F32LE, input->GetPoints()->GetData());
  }
  else if (nPoints == 0)
  {
    hsize_t pointsDimensions[2] = { 0, 3 };
    dataset = this->Impl->createHdfDataset(root, "Points", H5T_IEEE_F32LE, 2, pointsDimensions);
  }

  if (dataset == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create points dataset when creating: " << this->FileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendCells(vtkPolyData* input)
{
  const hid_t root = this->Impl->GetRoot();
  // On group per primitive: Polygons, Strips, Vertices, Lines
  auto cellArrayTopos = this->Impl->getCellArraysForTopos(input);
  for (const auto& cellArrayTopo : cellArrayTopos)
  {
    const char* groupName = cellArrayTopo.hdfGroupName;
    vtkCellArray* cells = cellArrayTopo.cellArray;

    // Create group
    vtkHDF::ScopedH5GHandle group{ H5Gcreate(
      root, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
    if (group == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Can not create " << groupName
                    << " group when creating: " << this->FileName);
      return false;
    }

    // Number of cells
    {
      int nCells = cells->GetNumberOfCells();
      hsize_t dimensions[1] = { 1 };
      const vtkHDF::ScopedH5DHandle nCellsDataset = this->Impl->createAndWriteHdfDataset(
        group, H5T_STD_I64LE, H5T_NATIVE_INT, "NumberOfCells", 1, dimensions, &nCells);
      if (nCellsDataset == H5I_INVALID_HID)
      {
        vtkErrorMacro(<< "Can not create NumberOfCells dataset in group " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }
    }

    // Number of connectivityIds
    {
      int nConnectivityIds = cells->GetNumberOfConnectivityIds();
      hsize_t dimensions[1] = { 1 };
      const vtkHDF::ScopedH5DHandle nConnIdsDataset = this->Impl->createAndWriteHdfDataset(group,
        H5T_STD_I64LE, H5T_NATIVE_INT, "NumberOfConnectivityIds", 1, dimensions, &nConnectivityIds);
      if (nConnIdsDataset == H5I_INVALID_HID)
      {
        vtkErrorMacro(<< "Can not create NumberOfConnectivityIds dataset in group " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }
    }

    // Offsets
    if (this->Impl->createDatasetFromDataArray(
          group, "Offsets", H5T_STD_I64LE, cells->GetOffsetsArray()) == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Can not create Offsets dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }

    // Connectivity
    if (this->Impl->createDatasetFromDataArray(
          group, "Connectivity", H5T_STD_I64LE, cells->GetConnectivityArray()) == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Can not create Connectivity dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataArrays(vtkDataObject* input)
{
  const hid_t root = this->Impl->GetRoot();
  constexpr std::array<const char*, 3> groupNames = { "PointData", "CellData", "FieldData" };
  for (int iAttribute = 0; iAttribute < vtkHDFUtilities::GetNumberOfAttributeTypes(); ++iAttribute)
  {
    vtkDataSetAttributes* attributes = input->GetAttributes(iAttribute);
    if (attributes == nullptr)
    {
      continue;
    }

    int nArrays = attributes->GetNumberOfArrays();
    if (nArrays <= 0)
    {
      continue;
    }

    // Create the group
    const char* groupName = groupNames[iAttribute];
    vtkHDF::ScopedH5GHandle group{ H5Gcreate(
      root, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
    if (group == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Can not create " << groupName
                    << " group when creating: " << this->FileName);
      return false;
    }

    // Add the arrays in the group
    for (int iArray = 0; iArray < nArrays; ++iArray)
    {
      vtkAbstractArray* array = attributes->GetAbstractArray(iArray);
      const char* arrayName = array->GetName();
      hid_t arrayType = vtkHDFUtilities::getH5TypeFromVtkType(array->GetDataType());
      if (arrayType == H5I_INVALID_HID)
      {
        vtkWarningMacro(<< "Can not find HDF type for VTK type: " << array->GetDataType()
                        << " when creating: " << this->FileName);
        continue;
      }
      auto dataset = this->Impl->createDatasetFromDataArray(group, arrayName, arrayType, array);
      if (dataset == H5I_INVALID_HID)
      {
        vtkErrorMacro(<< "Can not create array " << arrayName << " of attribute " << groupName
                      << " when creating: " << this->FileName);
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkHDFWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Overwrite: " << (this->Overwrite ? "yes" : "no") << "\n";
}

//------------------------------------------------------------------------------
void vtkHDFWriter::WriteData()
{
  vtkDataSet* input = vtkDataSet::SafeDownCast(this->GetInput());
  if (!input)
  {
    vtkErrorMacro(<< "A dataset input is required.");
    return;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use.");
    return;
  }

  vtkPolyData* polydata = vtkPolyData::SafeDownCast(input);
  if (polydata != nullptr)
  {
    if (!this->WritePolyDataToRoot(polydata))
    {
      vtkErrorMacro(<< "Can't write polydata to file:" << this->FileName);
      return;
    }
    return;
  }

  vtkErrorMacro(<< "Dataset type not supported: " << input->GetClassName());
}

//------------------------------------------------------------------------------
int vtkHDFWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkHDFWriter::vtkHDFWriter()
  : Impl(new Implementation(this))
{
}

//------------------------------------------------------------------------------
vtkHDFWriter::~vtkHDFWriter()
{
  this->SetFileName(nullptr);
}

VTK_ABI_NAMESPACE_END
