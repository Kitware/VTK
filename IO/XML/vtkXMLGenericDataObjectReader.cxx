/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLGenericDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLGenericDataObjectReader.h"

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHyperOctree.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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
#include "vtkXMLHyperOctreeReader.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPImageDataReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPPolyDataReader.h"
#include "vtkXMLPRectilinearGridReader.h"
#include "vtkXMLPStructuredGridReader.h"
#include "vtkXMLPUnstructuredGridReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkCommand.h"

#include <cassert>

vtkStandardNewMacro(vtkXMLGenericDataObjectReader);

// ---------------------------------------------------------------------------
vtkXMLGenericDataObjectReader::vtkXMLGenericDataObjectReader()
{
  this->Reader=0;
}

// ---------------------------------------------------------------------------
vtkXMLGenericDataObjectReader::~vtkXMLGenericDataObjectReader()
{
  if(this->Reader!=0)
  {
    this->Reader->Delete();
  }
}

// ---------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::ReadOutputType(const char *name,
  bool &parallel)
{
  parallel=false;

  // Test if the file with the given name is a VTKFile with the given
  // type.
  vtkSmartPointer<vtkXMLFileReadTester> tester=
    vtkSmartPointer<vtkXMLFileReadTester>::New();

  tester->SetFileName(name);
  if(tester->TestReadFile())
  {
    char *cfileDataType=tester->GetFileDataType();
    if(cfileDataType!=0)
    {
      std::string fileDataType(cfileDataType);
      if(fileDataType.compare("HierarchicalBoxDataSet")==0 ||
         fileDataType.compare("vtkHierarchicalBoxDataSet") == 0)
      {
        return VTK_HIERARCHICAL_BOX_DATA_SET;
      }
      if (fileDataType.compare("vtkOverlappingAMR") == 0)
      {
        return VTK_OVERLAPPING_AMR;
      }
      if (fileDataType.compare("vtkNonOverlappingAMR") == 0)
      {
        return VTK_NON_OVERLAPPING_AMR;
      }
      if(fileDataType.compare("HyperOctree")==0)
      {
        return VTK_HYPER_OCTREE;
      }
      if(fileDataType.compare("ImageData")==0)
      {
        return VTK_IMAGE_DATA;
      }
      if(fileDataType.compare("PImageData")==0)
      {
        parallel=true;
        return VTK_IMAGE_DATA;
      }
      if(fileDataType.compare("vtkMultiBlockDataSet")==0)
      {
        return VTK_MULTIBLOCK_DATA_SET;
      }
      if(fileDataType.compare("PolyData")==0)
      {
        return VTK_POLY_DATA;
      }
      if(fileDataType.compare("PPolyData")==0)
      {
        parallel=true;
        return VTK_POLY_DATA;
      }
      if(fileDataType.compare("RectilinearGrid")==0)
      {
        return VTK_RECTILINEAR_GRID;
      }
      if(fileDataType.compare("PRectilinearGrid")==0)
      {
        parallel=true;
        return VTK_RECTILINEAR_GRID;
      }
      if(fileDataType.compare("StructuredGrid")==0)
      {
        return VTK_STRUCTURED_GRID;
      }
      if(fileDataType.compare("PStructuredGrid")==0)
      {
        parallel=true;
        return VTK_STRUCTURED_GRID;
      }
      if(fileDataType.compare("UnstructuredGrid")==0 ||
         fileDataType.compare("UnstructuredGridBase")==0)
      {
        return VTK_UNSTRUCTURED_GRID;
      }
      if(fileDataType.compare("PUnstructuredGrid")==0 ||
         fileDataType.compare("PUnstructuredGridBase")==0)
      {
        parallel=true;
        return VTK_UNSTRUCTURED_GRID;
      }
    }
  }

  vtkErrorMacro(<<"could not load " << name);
  return -1;
}

// ---------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestDataObject(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if(!this->Stream && !this->FileName)
  {
    vtkErrorMacro("File name not specified");
    return 0;
  }

  if(this->Reader!=0)
  {
    this->Reader->Delete();
    this->Reader=0;
  }

  vtkDataObject *output=0;

  // Create reader.
  bool parallel=false;
  switch(this->ReadOutputType(this->FileName,parallel))
  {
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      this->Reader = vtkXMLUniformGridAMRReader::New();
      output=vtkHierarchicalBoxDataSet::New();
      break;
    case VTK_OVERLAPPING_AMR:
      this->Reader = vtkXMLUniformGridAMRReader::New();
      output = vtkOverlappingAMR::New();
      break;
    case VTK_NON_OVERLAPPING_AMR:
      this->Reader = vtkXMLUniformGridAMRReader::New();
      output = vtkNonOverlappingAMR::New();
      break;
    case VTK_HYPER_OCTREE:
      this->Reader=vtkXMLHyperOctreeReader::New();
      output=vtkHyperOctree::New();
      break;
    case VTK_IMAGE_DATA:
      if(parallel)
      {
        this->Reader=vtkXMLPImageDataReader::New();
      }
      else
      {
        this->Reader=vtkXMLImageDataReader::New();
      }
      output=vtkImageData::New();
      break;
    case VTK_MULTIBLOCK_DATA_SET:
      this->Reader=vtkXMLMultiBlockDataReader::New();
      output=vtkMultiBlockDataSet::New();
      break;
    case VTK_POLY_DATA:
      if(parallel)
      {
        this->Reader=vtkXMLPPolyDataReader::New();
      }
      else
      {
        this->Reader=vtkXMLPolyDataReader::New();
      }
      output=vtkPolyData::New();
      break;
    case VTK_RECTILINEAR_GRID:
      if(parallel)
      {
        this->Reader=vtkXMLPRectilinearGridReader::New();
      }
      else
      {
        this->Reader=vtkXMLRectilinearGridReader::New();
      }
      output=vtkRectilinearGrid::New();
      break;
    case VTK_STRUCTURED_GRID:
      if(parallel)
      {
        this->Reader=vtkXMLPStructuredGridReader::New();
      }
      else
      {
        this->Reader=vtkXMLStructuredGridReader::New();
      }
      output=vtkStructuredGrid::New();
      break;
    case VTK_UNSTRUCTURED_GRID:
      if(parallel)
      {
        this->Reader=vtkXMLPUnstructuredGridReader::New();
      }
      else
      {
        this->Reader=vtkXMLUnstructuredGridReader::New();
      }
      output=vtkUnstructuredGrid::New();
      break;
    default:
      this->Reader=0;
  }

  if(this->Reader!=0)
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
    int result=this->Reader->ProcessRequest(request,inputVector,outputVector);
    if(result)
    {
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);

//      outInfo->Set(vtkDataObject::DATA_OBJECT(),output);
      if(output!=0)
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

// ----------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // reader is created in RequestDataObject.
  if(this->Reader!=0)
  {
    // RequestInformation() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestUpdateExtent(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // reader is created in RequestDataObject.
  if(this->Reader!=0)
  {
    // RequestUpdateExtent() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // reader is created in RequestDataObject.
  if(this->Reader!=0)
  {
    // RequestData() would be more appropriate but it is protected.
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }
  else
  {
    return 0;
  }
}


// ----------------------------------------------------------------------------
void vtkXMLGenericDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
vtkDataObject* vtkXMLGenericDataObjectReader::GetOutput()
{
  return this->GetOutput(0);
}

// ----------------------------------------------------------------------------
vtkDataObject* vtkXMLGenericDataObjectReader::GetOutput(int idx)
{
  return this->GetOutputDataObject(idx);
}

// ---------------------------------------------------------------------------
vtkHierarchicalBoxDataSet *
vtkXMLGenericDataObjectReader::GetHierarchicalBoxDataSetOutput()
{
  return vtkHierarchicalBoxDataSet::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkHyperOctree *vtkXMLGenericDataObjectReader::GetHyperOctreeOutput()
{
  return vtkHyperOctree::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkImageData *vtkXMLGenericDataObjectReader::GetImageDataOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkMultiBlockDataSet *
vtkXMLGenericDataObjectReader::GetMultiBlockDataSetOutput()
{
  return vtkMultiBlockDataSet::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkPolyData *vtkXMLGenericDataObjectReader::GetPolyDataOutput()
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkRectilinearGrid *vtkXMLGenericDataObjectReader::GetRectilinearGridOutput()
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkStructuredGrid *vtkXMLGenericDataObjectReader::GetStructuredGridOutput()
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

// ---------------------------------------------------------------------------
vtkUnstructuredGrid *vtkXMLGenericDataObjectReader::GetUnstructuredGridOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

// ----------------------------------------------------------------------------
const char* vtkXMLGenericDataObjectReader::GetDataSetName()
{
  assert("check: not_used" && 0); // should not be used.
  return "DataObject"; // not used.
}

// ----------------------------------------------------------------------------
void vtkXMLGenericDataObjectReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

// ----------------------------------------------------------------------------
int vtkXMLGenericDataObjectReader::FillOutputPortInformation(
  int,
  vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
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
