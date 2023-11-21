// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFWriter.h"

#include "vtkAbstractArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkErrorCode.h"
#include "vtkHDFWriterImplementation.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHDFWriter);

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(vtkPolyData* input)
{
  bool writeSuccess = true;

  writeSuccess &= this->Impl->OpenRoot(this->Overwrite);
  hid_t rootGroup = this->Impl->GetRoot();

  writeSuccess &= this->Impl->WriteHeader("PolyData");
  writeSuccess &= this->AppendNumberOfPoints(rootGroup, input);
  writeSuccess &= this->AppendPoints(rootGroup, input);
  writeSuccess &= this->AppendPrimitiveCells(rootGroup, input);
  writeSuccess &= this->AppendDataArrays(rootGroup, input);
  return writeSuccess;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::WriteDatasetToFile(vtkUnstructuredGrid* input)
{
  vtkCellArray* cells = input->GetCells();
  bool writeSuccess = true;

  writeSuccess &= this->Impl->OpenRoot(this->Overwrite);
  hid_t rootGroup = this->Impl->GetRoot();

  writeSuccess &= this->Impl->WriteHeader("UnstructuredGrid");
  writeSuccess &= this->AppendNumberOfPoints(rootGroup, input);
  writeSuccess &= this->AppendPoints(rootGroup, input);
  writeSuccess &= this->AppendNumberOfCells(rootGroup, cells);
  writeSuccess &= this->AppendCellTypes(rootGroup, input);
  writeSuccess &= this->AppendNumberOfConnectivityIds(rootGroup, cells);
  writeSuccess &= this->AppendConnectivity(rootGroup, cells);
  writeSuccess &= this->AppendOffsets(rootGroup, cells);
  writeSuccess &= this->AppendDataArrays(rootGroup, input);
  return writeSuccess;
}
//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfPoints(hid_t group, vtkPointSet* input)
{
  if (this->Impl->createSingleValueDataset(group, "NumberOfPoints", input->GetNumberOfPoints()) ==
    H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create NumberOfPoints dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfCells(hid_t group, vtkCellArray* input)
{
  if (this->Impl->createSingleValueDataset(group, "NumberOfCells", input->GetNumberOfCells()) ==
    H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create NumberOfCells dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendNumberOfConnectivityIds(hid_t group, vtkCellArray* input)
{
  if (this->Impl->createSingleValueDataset(
        group, "NumberOfConnectivityIds", input->GetNumberOfConnectivityIds()) == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create NumberOfConnectivityIds dataset when creating: "
                  << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendCellTypes(hid_t group, vtkUnstructuredGrid* input)
{
  if (this->Impl->createDatasetFromDataArray(
        group, "Types", H5T_STD_U8LE, input->GetCellTypesArray()) == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create Types dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}
//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendOffsets(hid_t group, vtkCellArray* input)
{
  if (this->Impl->createDatasetFromDataArray(
        group, "Offsets", H5T_STD_I64LE, input->GetOffsetsArray()) == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create Offsets dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendConnectivity(hid_t group, vtkCellArray* input)
{
  if (this->Impl->createDatasetFromDataArray(
        group, "Connectivity", H5T_STD_I64LE, input->GetConnectivityArray()) == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create Connectivity dataset when creating: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendPoints(hid_t group, vtkPointSet* input)
{
  const int nPoints = input->GetNumberOfPoints();
  hid_t dataset{ H5I_INVALID_HID };
  if (input->GetPoints() != nullptr && input->GetPoints()->GetData() != nullptr)
  {
    dataset = this->Impl->createDatasetFromDataArray(
      group, "Points", H5T_IEEE_F32LE, input->GetPoints()->GetData());
  }
  else if (nPoints == 0)
  {
    hsize_t pointsDimensions[2] = { 0, 3 };
    dataset = this->Impl->createHdfDataset(group, "Points", H5T_IEEE_F32LE, 2, pointsDimensions);
  }

  if (dataset == H5I_INVALID_HID)
  {
    vtkErrorMacro(<< "Can not create points dataset when creating: " << this->FileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendPrimitiveCells(hid_t baseGroup, vtkPolyData* input)
{
  // On group per primitive: Polygons, Strips, Vertices, Lines
  auto cellArrayTopos = this->Impl->getCellArraysForTopos(input);
  for (const auto& cellArrayTopo : cellArrayTopos)
  {
    const char* groupName = cellArrayTopo.hdfGroupName;
    vtkCellArray* cells = cellArrayTopo.cellArray;

    // Create group
    vtkHDF::ScopedH5GHandle group{ H5Gcreate(
      baseGroup, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
    if (group == H5I_INVALID_HID)
    {
      vtkErrorMacro(<< "Can not create " << groupName
                    << " group when creating: " << this->FileName);
      return false;
    }

    if (!this->AppendNumberOfCells(group, cells))
    {
      vtkErrorMacro(<< "Can not create NumberOfCells dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }

    if (!this->AppendNumberOfConnectivityIds(group, cells))
    {
      vtkErrorMacro(<< "Can not create NumberOfConnectivityIds dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }

    if (!this->AppendOffsets(group, cells))
    {
      vtkErrorMacro(<< "Can not create Offsets dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }

    if (!this->AppendConnectivity(group, cells))
    {
      vtkErrorMacro(<< "Can not create Connectivity dataset in group " << groupName
                    << " when creating: " << this->FileName);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHDFWriter::AppendDataArrays(hid_t baseGroup, vtkDataObject* input)
{
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
      baseGroup, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
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

  // The writer can handle polydata and unstructured grids.
  vtkPolyData* polydata = vtkPolyData::SafeDownCast(input);
  if (polydata != nullptr)
  {
    if (!this->WriteDatasetToFile(polydata))
    {
      vtkErrorMacro(<< "Can't write polydata to file:" << this->FileName);
      return;
    }
    return;
  }

  vtkUnstructuredGrid* unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (unstructuredGrid != nullptr)
  {
    if (!this->WriteDatasetToFile(unstructuredGrid))
    {
      vtkErrorMacro(<< "Can't write unstructuredGrid to file:" << this->FileName);
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
