/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHDFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHDFReader.h"

#include "vtkAppendDataSets.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkHDFReaderImplementation.h"
#include "vtkHDFReaderVersion.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/Encoding.hxx"
#include "vtksys/FStream.hxx"
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <locale>
#include <numeric>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkHDFReader);

namespace
{
//----------------------------------------------------------------------------
int GetNDims(int* extent)
{
  int ndims = 3;
  if (extent[5] - extent[4] == 0)
  {
    --ndims;
  }
  if (extent[3] - extent[2] == 0)
  {
    --ndims;
  }
  return ndims;
}

//----------------------------------------------------------------------------
std::vector<hsize_t> ReduceDimension(int* updateExtent, int* wholeExtent)
{
  int dims = ::GetNDims(wholeExtent);
  std::vector<hsize_t> v(2 * dims);
  for (int i = 0; i < dims; ++i)
  {
    int j = 2 * i;
    v[j] = updateExtent[j];
    v[j + 1] = updateExtent[j + 1];
  }
  return v;
}
}

//----------------------------------------------------------------------------
vtkHDFReader::vtkHDFReader()
{
  this->FileName = nullptr;
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkHDFReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  for (int i = 0; i < vtkHDFReader::GetNumberOfAttributeTypes(); ++i)
  {
    this->DataArraySelection[i] = vtkDataArraySelection::New();
    this->DataArraySelection[i]->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  }
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  std::fill(this->WholeExtent, this->WholeExtent + 6, 0);
  std::fill(this->Origin, this->Origin + 3, 0.0);
  std::fill(this->Spacing, this->Spacing + 3, 0.0);
  this->Impl = new vtkHDFReader::Implementation(this);
}

//----------------------------------------------------------------------------
vtkHDFReader::~vtkHDFReader()
{
  delete this->Impl;
  this->SetFileName(nullptr);
  for (int i = 0; i < vtkHDFReader::GetNumberOfAttributeTypes(); ++i)
  {
    this->DataArraySelection[i]->RemoveObserver(this->SelectionObserver);
    this->DataArraySelection[i]->Delete();
  }
  this->SelectionObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkHDFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "CellDataArraySelection: " << this->DataArraySelection[vtkDataObject::CELL]
     << "\n";
  os << indent << "PointDataArraySelection: " << this->DataArraySelection[vtkDataObject::POINT]
     << "\n";
}

//----------------------------------------------------------------------------
vtkDataSet* vtkHDFReader::GetOutputAsDataSet()
{
  return this->GetOutputAsDataSet(0);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkHDFReader::GetOutputAsDataSet(int index)
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(index));
}

//----------------------------------------------------------------------------
// Major version should be incremented when older readers can no longer
// read files written for this reader. Minor versions are for added
// functionality that can be safely ignored by older readers.
int vtkHDFReader::CanReadFileVersion(int major, int vtkNotUsed(minor))
{
  return (major > vtkHDFReaderMajorVersion) ? 0 : 1;
}

//----------------------------------------------------------------------------
int vtkHDFReader::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(name, &fs) != 0)
  {
    vtkErrorMacro("File does not exist: " << name);
    return 0;
  }
  if (!this->Impl->Open(name))
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkHDFReader::SelectionModifiedCallback(vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkHDFReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int vtkHDFReader::GetNumberOfPointArrays()
{
  return this->DataArraySelection[vtkDataObject::POINT]->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkHDFReader::GetPointArrayName(int index)
{
  return this->DataArraySelection[vtkDataObject::POINT]->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkHDFReader::GetPointDataArraySelection()
{
  return this->DataArraySelection[vtkDataObject::POINT];
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkHDFReader::GetFieldDataArraySelection()
{
  return this->DataArraySelection[vtkDataObject::FIELD];
}

//----------------------------------------------------------------------------
int vtkHDFReader::GetNumberOfCellArrays()
{
  return this->DataArraySelection[vtkDataObject::CELL]->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkHDFReader::GetCellDataArraySelection()
{
  return this->DataArraySelection[vtkDataObject::CELL];
}

//----------------------------------------------------------------------------
const char* vtkHDFReader::GetCellArrayName(int index)
{
  return this->DataArraySelection[vtkDataObject::CELL]->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkHDFReader::RequestDataObject(vtkInformation*, vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  std::map<int, std::string> typeNameMap = { std::make_pair(VTK_IMAGE_DATA, "vtkImageData"),
    std::make_pair(VTK_UNSTRUCTURED_GRID, "vtkUnstructuredGrid") };
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }

  if (!this->Impl->Open(this->FileName))
  {
    return 0;
  }
  auto version = this->Impl->GetVersion();
  if (!CanReadFileVersion(version[0], version[1]))
  {
    vtkWarningMacro("File version: " << version[0] << "." << version[1]
                                     << " is higher than "
                                        "this reader supports "
                                     << vtkHDFReaderMajorVersion << "."
                                     << vtkHDFReaderMinorVersion);
  }
  int dataSetType = this->Impl->GetDataSetType();
  if (!output || !output->IsA(typeNameMap[dataSetType].c_str()))
  {
    vtkDataSet* newOutput = nullptr;
    if (dataSetType == VTK_IMAGE_DATA)
    {
      newOutput = vtkImageData::New();
    }
    else if (dataSetType == VTK_UNSTRUCTURED_GRID)
    {
      newOutput = vtkUnstructuredGrid::New();
    }
    else
    {
      vtkErrorMacro("HDF dataset type unknown: " << dataSetType);
      return 0;
    }
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
    for (int i = 0; i < vtkHDFReader::GetNumberOfAttributeTypes(); ++i)
    {
      this->DataArraySelection[i]->RemoveAllArrays();
      std::vector<std::string> arrayNames = this->Impl->GetArrayNames(i);
      for (const std::string& arrayName : arrayNames)
      {
        this->DataArraySelection[i]->AddArray(arrayName.c_str());
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }
  // Insures a new file is open. This happen for vtkFileSeriesReader
  // which does not call RequestDataObject for every time step.
  if (!this->Impl->Open(this->FileName))
  {
    return 0;
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Invalid output information object");
    return 0;
  }
  int dataSetType = this->Impl->GetDataSetType();
  if (dataSetType == VTK_IMAGE_DATA)
  {
    if (!this->Impl->GetAttribute("WholeExtent", 6, this->WholeExtent))
    {
      return 0;
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);
    if (!this->Impl->GetAttribute("Origin", 3, this->Origin))
    {
      return 0;
    }
    outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
    if (!this->Impl->GetAttribute("Spacing", 3, this->Spacing))
    {
      return 0;
    }
    outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
    outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
  else
  {
    vtkErrorMacro("Invalid dataset type: " << dataSetType);
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkHDFReader::PrintPieceInformation(vtkInformation* outInfo)
{
  std::array<int, 6> updateExtent;
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), &updateExtent[0]);
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numGhosts = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  std::cout << "Piece:" << piece << " " << numPieces << " " << numGhosts;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    std::cout << " Extent: " << updateExtent[0] << " " << updateExtent[1] << " " << updateExtent[2]
              << " " << updateExtent[3] << " " << updateExtent[4] << " " << updateExtent[5];
  }
  std::cout << std::endl;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkImageData* data)
{
  std::array<int, 6> updateExtent;
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), &updateExtent[0]);
  // this->PrintPieceInformation(outInfo);
  data->SetOrigin(this->Origin);
  data->SetSpacing(this->Spacing);
  data->SetExtent(&updateExtent[0]);
  if (!this->Impl->GetAttribute("Direction", 9, data->GetDirectionMatrix()->GetData()))
  {
    return 0;
  }

  // in the same order as vtkDataObject::AttributeTypes: POINT, CELL, FIELD
  for (int attributeType = 0; attributeType < vtkHDFReader::GetNumberOfAttributeTypes();
       ++attributeType)
  {
    std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
    for (const std::string& name : names)
    {
      if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
      {
        vtkSmartPointer<vtkDataArray> array;
        std::vector<hsize_t> fileExtent = ::ReduceDimension(&updateExtent[0], this->WholeExtent);
        std::copy(updateExtent.begin(), updateExtent.end(), fileExtent.begin());
        if ((array = vtk::TakeSmartPointer(
               this->Impl->NewArray(attributeType, name.c_str(), fileExtent))) == nullptr)
        {
          vtkErrorMacro("Error reading array " << name);
          return 0;
        }
        array->SetName(name.c_str());
        data->GetAttributesAsFieldData(attributeType)->AddArray(array);
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::AddFieldArrays(vtkDataSet* data)
{
  std::vector<std::string> names = this->Impl->GetArrayNames(vtkDataObject::FIELD);
  for (const std::string& name : names)
  {
    vtkSmartPointer<vtkAbstractArray> array;
    if ((array = vtk::TakeSmartPointer(this->Impl->NewFieldArray(name.c_str()))) == nullptr)
    {
      vtkErrorMacro("Error reading array " << name);
      return 0;
    }
    array->SetName(name.c_str());
    data->GetAttributesAsFieldData(vtkDataObject::FIELD)->AddArray(array);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(const std::vector<vtkIdType>& numberOfPoints,
  const std::vector<vtkIdType>& numberOfCells,
  const std::vector<vtkIdType>& numberOfConnectivityIds, int filePiece,
  vtkUnstructuredGrid* pieceData)
{
  // read the piece and add it to data
  vtkNew<vtkPoints> points;
  vtkSmartPointer<vtkDataArray> pointArray;
  vtkIdType pointOffset = std::accumulate(&numberOfPoints[0], &numberOfPoints[filePiece], 0);
  if ((pointArray = vtk::TakeSmartPointer(this->Impl->NewMetadataArray(
         "Points", pointOffset, numberOfPoints[filePiece]))) == nullptr)
  {
    vtkErrorMacro("Cannot read the Points array");
    return 0;
  }
  points->SetData(pointArray);
  pieceData->SetPoints(points);
  vtkNew<vtkCellArray> cellArray;
  vtkSmartPointer<vtkDataArray> offsetsArray;
  vtkSmartPointer<vtkDataArray> connectivityArray;
  vtkSmartPointer<vtkDataArray> p;
  vtkUnsignedCharArray* typesArray;
  // the offsets array has (numberOfCells[i] + 1) elements.
  vtkIdType offset = std::accumulate(&numberOfCells[0], &numberOfCells[filePiece], filePiece);
  if ((offsetsArray = vtk::TakeSmartPointer(
         this->Impl->NewMetadataArray("Offsets", offset, numberOfCells[filePiece] + 1))) == nullptr)
  {
    vtkErrorMacro("Cannot read the Offsets array");
    return 0;
  }
  offset = std::accumulate(&numberOfConnectivityIds[0], &numberOfConnectivityIds[filePiece], 0);
  if ((connectivityArray = vtk::TakeSmartPointer(this->Impl->NewMetadataArray(
         "Connectivity", offset, numberOfConnectivityIds[filePiece]))) == nullptr)
  {
    vtkErrorMacro("Cannot read the Connectivity array");
    return 0;
  }
  cellArray->SetData(offsetsArray, connectivityArray);

  vtkIdType cellOffset = std::accumulate(&numberOfCells[0], &numberOfCells[filePiece], 0);
  if ((p = vtk::TakeSmartPointer(
         this->Impl->NewMetadataArray("Types", cellOffset, numberOfCells[filePiece]))) == nullptr)
  {
    vtkErrorMacro("Cannot read the Types array");
    return 0;
  }
  if ((typesArray = vtkUnsignedCharArray::SafeDownCast(p)) == nullptr)
  {
    vtkErrorMacro("Error: The Types array element is not unsigned char.");
    return 0;
  }
  pieceData->SetCells(typesArray, cellArray);

  std::vector<vtkIdType> offsets = { pointOffset, cellOffset };
  std::vector<const std::vector<vtkIdType>*> numberOf = { &numberOfPoints, &numberOfCells };
  // in the same order as vtkDataObject::AttributeTypes: POINT, CELL, FIELD
  // field arrays are only read on node 0
  for (int attributeType = 0; attributeType < vtkDataObject::FIELD; ++attributeType)
  {
    std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
    for (const std::string& name : names)
    {
      if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
      {
        vtkSmartPointer<vtkDataArray> array;
        if ((array = vtk::TakeSmartPointer(this->Impl->NewArray(attributeType, name.c_str(),
               offsets[attributeType], (*numberOf[attributeType])[filePiece]))) == nullptr)
        {
          vtkErrorMacro("Error reading array " << name);
          return 0;
        }
        array->SetName(name.c_str());
        pieceData->GetAttributesAsFieldData(attributeType)->AddArray(array);
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkUnstructuredGrid* data)
{
  // this->PrintPieceInformation(outInfo);
  int filePieceCount = this->Impl->GetNumberOfPieces();
  std::vector<vtkIdType> numberOfPoints = this->Impl->GetMetadata("NumberOfPoints", filePieceCount);
  if (numberOfPoints.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfCells = this->Impl->GetMetadata("NumberOfCells", filePieceCount);
  if (numberOfCells.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfConnectivityIds =
    this->Impl->GetMetadata("NumberOfConnectivityIds", filePieceCount);
  if (numberOfConnectivityIds.empty())
  {
    return 0;
  }
  int memoryPieceCount = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  vtkNew<vtkUnstructuredGrid> pieceData;
  vtkNew<vtkAppendDataSets> append;
  append->AddInputData(data);
  append->AddInputData(pieceData);
  for (int filePiece = piece; filePiece < filePieceCount; filePiece += memoryPieceCount)
  {
    pieceData->Initialize();
    if (!this->Read(numberOfPoints, numberOfCells, numberOfConnectivityIds, filePiece, pieceData))
    {
      return 0;
    }
    append->Update();
    data->ShallowCopy(append->GetOutput());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int ok = 1;
  if (!outInfo)
  {
    return 0;
  }
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }
  int dataSetType = this->Impl->GetDataSetType();
  if (dataSetType == VTK_IMAGE_DATA)
  {
    vtkImageData* data = vtkImageData::SafeDownCast(output);
    ok = this->Read(outInfo, data);
  }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID)
  {
    vtkUnstructuredGrid* data = vtkUnstructuredGrid::SafeDownCast(output);
    ok = this->Read(outInfo, data);
  }
  else
  {
    vtkErrorMacro("HDF dataset type unknown: " << dataSetType);
    return 0;
  }
  return ok && this->AddFieldArrays(output);
}
