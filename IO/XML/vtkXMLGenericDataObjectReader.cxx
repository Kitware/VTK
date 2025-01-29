// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_5_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkXMLGenericDataObjectReader.h"

#include "vtkCommand.h"
#include "vtkDataObjectTypes.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLFileReadTester.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPImageDataReader.h"
#include "vtkXMLPPolyDataReader.h"
#include "vtkXMLPRectilinearGridReader.h"
#include "vtkXMLPStructuredGridReader.h"
#include "vtkXMLPUnstructuredGridReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLGenericDataObjectReader);

//------------------------------------------------------------------------------
vtkXMLGenericDataObjectReader::vtkXMLGenericDataObjectReader()
{
  this->Reader = nullptr;
}

//------------------------------------------------------------------------------
vtkXMLGenericDataObjectReader::~vtkXMLGenericDataObjectReader()
{
  if (this->Reader != nullptr)
  {
    if (auto observer = this->GetReaderErrorObserver())
    {
      this->Reader->RemoveObserver(observer);
    }
    if (auto observer = this->GetParserErrorObserver())
    {
      this->Reader->RemoveObserver(observer);
    }
    this->Reader->Delete();
  }
}

//------------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::ReadOutputType(const char* name, bool& parallel)
{
  parallel = false;

  // Test if the file with the given name is a VTKFile with the given
  // type.
  vtkSmartPointer<vtkXMLFileReadTester> tester = vtkSmartPointer<vtkXMLFileReadTester>::New();

  tester->SetFileName(name);
  if (tester->TestReadFile())
  {
    const char* cfileDataType = tester->GetFileDataType();
    if (cfileDataType != nullptr)
    {
      std::string fileDataType(cfileDataType);
      if (fileDataType == "HierarchicalBoxDataSet" || fileDataType == "vtkHierarchicalBoxDataSet")
      {
        return VTK_HIERARCHICAL_BOX_DATA_SET;
      }
      if (fileDataType == "vtkOverlappingAMR")
      {
        return VTK_OVERLAPPING_AMR;
      }
      if (fileDataType == "vtkNonOverlappingAMR")
      {
        return VTK_NON_OVERLAPPING_AMR;
      }
      if (fileDataType == "ImageData")
      {
        return VTK_IMAGE_DATA;
      }
      if (fileDataType == "PImageData")
      {
        parallel = true;
        return VTK_IMAGE_DATA;
      }
      if (fileDataType == "vtkMultiBlockDataSet")
      {
        return VTK_MULTIBLOCK_DATA_SET;
      }
      if (fileDataType == "PolyData")
      {
        return VTK_POLY_DATA;
      }
      if (fileDataType == "PPolyData")
      {
        parallel = true;
        return VTK_POLY_DATA;
      }
      if (fileDataType == "RectilinearGrid")
      {
        return VTK_RECTILINEAR_GRID;
      }
      if (fileDataType == "PRectilinearGrid")
      {
        parallel = true;
        return VTK_RECTILINEAR_GRID;
      }
      if (fileDataType == "StructuredGrid")
      {
        return VTK_STRUCTURED_GRID;
      }
      if (fileDataType == "PStructuredGrid")
      {
        parallel = true;
        return VTK_STRUCTURED_GRID;
      }
      if (fileDataType == "UnstructuredGrid" || fileDataType == "UnstructuredGridBase")
      {
        return VTK_UNSTRUCTURED_GRID;
      }
      if (fileDataType == "PUnstructuredGrid" || fileDataType == "PUnstructuredGridBase")
      {
        parallel = true;
        return VTK_UNSTRUCTURED_GRID;
      }
    }
  }

  vtkErrorMacro(<< "could not load " << name);
  return -1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkXMLReader> vtkXMLGenericDataObjectReader::CreateReader(
  int data_object_type, bool parallel)
{
  switch (data_object_type)
  {
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      return vtkSmartPointer<vtkXMLUniformGridAMRReader>::New();
    case VTK_OVERLAPPING_AMR:
      return vtkSmartPointer<vtkXMLUniformGridAMRReader>::New();
    case VTK_NON_OVERLAPPING_AMR:
      return vtkSmartPointer<vtkXMLUniformGridAMRReader>::New();
    case VTK_IMAGE_DATA:
      if (parallel)
      {
        return vtkSmartPointer<vtkXMLPImageDataReader>::New();
      }
      else
      {
        return vtkSmartPointer<vtkXMLImageDataReader>::New();
      }
    case VTK_MULTIBLOCK_DATA_SET:
      return vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
    case VTK_POLY_DATA:
      if (parallel)
      {
        return vtkSmartPointer<vtkXMLPPolyDataReader>::New();
      }
      else
      {
        return vtkSmartPointer<vtkXMLPolyDataReader>::New();
      }
    case VTK_RECTILINEAR_GRID:
      if (parallel)
      {
        return vtkSmartPointer<vtkXMLPRectilinearGridReader>::New();
      }
      else
      {
        return vtkSmartPointer<vtkXMLRectilinearGridReader>::New();
      }
    case VTK_STRUCTURED_GRID:
      if (parallel)
      {
        return vtkSmartPointer<vtkXMLPStructuredGridReader>::New();
      }
      else
      {
        return vtkSmartPointer<vtkXMLStructuredGridReader>::New();
      }
    case VTK_UNSTRUCTURED_GRID:
      if (parallel)
      {
        return vtkSmartPointer<vtkXMLPUnstructuredGridReader>::New();
      }
      else
      {
        return vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
      }
    default:
      return nullptr;
  }
}

//------------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Stream && !this->FileName)
  {
    vtkErrorMacro("File name not specified");
    return 0;
  }

  if (this->Reader != nullptr)
  {
    if (auto observer = this->GetReaderErrorObserver())
    {
      this->Reader->RemoveObserver(observer);
    }
    if (auto observer = this->GetParserErrorObserver())
    {
      this->Reader->RemoveObserver(observer);
    }
    this->Reader->Delete();
    this->Reader = nullptr;
  }

  vtkDataObject* output = nullptr;

  // Create reader.
  bool parallel = false;
  auto data_type = this->ReadOutputType(this->FileName, parallel);
  if (auto reader = vtkXMLGenericDataObjectReader::CreateReader(data_type, parallel))
  {
    output = vtkDataObjectTypes::NewDataObject(data_type);
    this->Reader = reader;
    this->Reader->Register(this);
  }
  else
  {
    this->Reader = nullptr;
  }

  if (this->Reader != nullptr)
  {
    this->Reader->SetFileName(this->GetFileName());
    //    this->Reader->SetStream(this->GetStream());
    // Delegate the error observers
    if (this->GetReaderErrorObserver())
    {
      this->Reader->AddObserver(vtkCommand::ErrorEvent, this->GetReaderErrorObserver());
    }
    if (this->GetParserErrorObserver())
    {
      this->Reader->SetParserErrorObserver(this->GetParserErrorObserver());
    }
    // Delegate call. RequestDataObject() would be more appropriate but it is
    // protected.
    int result = this->Reader->ProcessRequest(request, inputVector, outputVector);
    if (result)
    {
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);

      //      outInfo->Set(vtkDataObject::DATA_OBJECT(),output);
      if (output != nullptr)
      {
        output->Delete();
      }
    }
    return result;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // reader is created in RequestDataObject.
  if (this->Reader != nullptr)
  {
    // RequestInformation() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // reader is created in RequestDataObject.
  if (this->Reader != nullptr)
  {
    // RequestUpdateExtent() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // reader is created in RequestDataObject.
  if (this->Reader != nullptr)
  {
    // RequestData() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
void vtkXMLGenericDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkXMLGenericDataObjectReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkXMLGenericDataObjectReader::GetOutput(int idx)
{
  return this->GetOutputDataObject(idx);
}

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkXMLGenericDataObjectReader::GetHierarchicalBoxDataSetOutput()
{
  vtkLogF(
    WARNING, "GetHierarchicalBoxDataSetOutput is deprecated, use GetOverlappingAMROutput instead");
  return vtkHierarchicalBoxDataSet::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* vtkXMLGenericDataObjectReader::GetOverlappingAMROutput()
{
  return vtkOverlappingAMR::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkImageData* vtkXMLGenericDataObjectReader::GetImageDataOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkXMLGenericDataObjectReader::GetMultiBlockDataSetOutput()
{
  return vtkMultiBlockDataSet::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkPolyData* vtkXMLGenericDataObjectReader::GetPolyDataOutput()
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLGenericDataObjectReader::GetRectilinearGridOutput()
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLGenericDataObjectReader::GetStructuredGridOutput()
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLGenericDataObjectReader::GetUnstructuredGridOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

//------------------------------------------------------------------------------
const char* vtkXMLGenericDataObjectReader::GetDataSetName()
{
  assert("check: not_used" && 0); // should not be used.
  return "DataObject";            // not used.
}

//------------------------------------------------------------------------------
void vtkXMLGenericDataObjectReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//------------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLGenericDataObjectReader::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
  {
    numPts = output->GetNumberOfPoints();
  }
  return numPts;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLGenericDataObjectReader::GetNumberOfCells()
{
  vtkIdType numCells = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
  {
    numCells = output->GetNumberOfCells();
  }
  return numCells;
}
VTK_ABI_NAMESPACE_END
