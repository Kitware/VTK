// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDFReader.h"

#include "vtkAppendDataSets.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkHDFReaderImplementation.h"
#include "vtkHDFReaderVersion.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPolyData.h"
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

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHDFReader);

namespace
{
//----------------------------------------------------------------------------
constexpr std::size_t NUM_POLY_DATA_TOPOS = 4;
const std::vector<std::string> POLY_DATA_TOPOS{ "Vertices", "Lines", "Polygons", "Strips" };

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

//----------------------------------------------------------------------------
struct TransientGeometryOffsets
{
public:
  bool Success = true;
  vtkIdType PartOffset = 0;
  vtkIdType PointOffset = 0;
  std::vector<vtkIdType> CellOffsets;
  std::vector<vtkIdType> ConnectivityOffsets;

  template <class T>
  TransientGeometryOffsets(T* impl, vtkIdType step)
  {
    auto recupMultiOffset = [&](std::string path, std::vector<vtkIdType>& val) {
      val = impl->GetMetadata(path.c_str(), 1, step);
      if (val.empty())
      {
        vtkErrorWithObjectMacro(
          nullptr, << path.c_str() << " array cannot be empty when there is transient data");
        return false;
      }
      return true;
    };
    auto recupSingleOffset = [&](std::string path, vtkIdType& val) {
      std::vector<vtkIdType> buffer;
      if (!recupMultiOffset(path, buffer))
      {
        return false;
      }
      val = buffer[0];
      return true;
    };
    if (!recupSingleOffset("Steps/PartOffsets", this->PartOffset))
    {
      this->Success = false;
      return;
    }
    if (!recupSingleOffset("Steps/PointOffsets", this->PointOffset))
    {
      this->Success = false;
      return;
    }
    if (!recupMultiOffset("Steps/CellOffsets", this->CellOffsets))
    {
      this->Success = false;
      return;
    }
    if (!recupMultiOffset("Steps/ConnectivityIdOffsets", this->ConnectivityOffsets))
    {
      this->Success = false;
      return;
    }
  }
};

template <class T>
bool ReadPolyDataPiece(T* impl, vtkIdType pointOffset, vtkIdType numberOfPoints,
  std::vector<vtkIdType>& cellOffsets, std::vector<vtkIdType>& numberOfCells,
  std::vector<vtkIdType>& connectivityOffsets, std::vector<vtkIdType>& numberOfConnectivityIds,
  vtkPolyData* pieceData)
{
  vtkNew<vtkPoints> points;
  vtkSmartPointer<vtkDataArray> pointArray;
  if ((pointArray = vtk::TakeSmartPointer(
         impl->NewMetadataArray("Points", pointOffset, numberOfPoints))) == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, "Cannot read the Points array");
    return false;
  }
  points->SetData(pointArray);
  pieceData->SetPoints(points);

  std::vector<vtkSmartPointer<vtkCellArray>> cArrays;
  for (std::size_t iTopo = 0; iTopo < ::NUM_POLY_DATA_TOPOS; ++iTopo)
  {
    const auto& name = ::POLY_DATA_TOPOS[iTopo];
    vtkSmartPointer<vtkDataArray> offsetsArray;
    if ((offsetsArray = vtk::TakeSmartPointer(impl->NewMetadataArray(
           (name + "/Offsets").c_str(), cellOffsets[iTopo], numberOfCells[iTopo] + 1))) == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Cannot read the Offsets array for " + name);
      return false;
    }
    vtkSmartPointer<vtkDataArray> connectivityArray;
    if ((connectivityArray =
            vtk::TakeSmartPointer(impl->NewMetadataArray((name + "/Connectivity").c_str(),
              connectivityOffsets[iTopo], numberOfConnectivityIds[iTopo]))) == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Cannot read the Connectivity array for " + name);
      return false;
    }
    vtkNew<vtkCellArray> cellArray;
    cellArray->SetData(offsetsArray, connectivityArray);
    cArrays.emplace_back(cellArray);
  }
  pieceData->SetVerts(cArrays[0]);
  pieceData->SetLines(cArrays[1]);
  pieceData->SetPolys(cArrays[2]);
  pieceData->SetStrips(cArrays[3]);
  return true;
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
  this->TimeRange[0] = this->TimeRange[1] = 0.0;
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
  os << indent << "HasTransientData: " << (this->HasTransientData ? "true" : "false") << "\n";
  os << indent << "NumberOfSteps: " << this->NumberOfSteps << "\n";
  os << indent << "Step: " << this->Step << "\n";
  os << indent << "TimeValue: " << this->TimeValue << "\n";
  os << indent << "TimeRange: " << this->TimeRange[0] << " - " << this->TimeRange[1] << "\n";
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
  std::map<int, std::string> typeNameMap = {
    { VTK_IMAGE_DATA, "vtkImageData" }, { VTK_UNSTRUCTURED_GRID, "vtkUnstructuredGrid" },
    { VTK_POLY_DATA, "vtkPolyData" }, { VTK_OVERLAPPING_AMR, "vtkOverlappingAMR" }

  };
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

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
  this->NumberOfSteps = this->Impl->GetNumberOfSteps();
  this->HasTransientData = (this->NumberOfSteps > 1);
  int dataSetType = this->Impl->GetDataSetType();
  if (!output || !output->IsA(typeNameMap[dataSetType].c_str()))
  {
    vtkSmartPointer<vtkDataObject> newOutput = nullptr;
    if (dataSetType == VTK_IMAGE_DATA)
    {
      newOutput = vtkSmartPointer<vtkImageData>::New();
    }
    else if (dataSetType == VTK_UNSTRUCTURED_GRID)
    {
      newOutput = vtkSmartPointer<vtkUnstructuredGrid>::New();
    }
    else if (dataSetType == VTK_OVERLAPPING_AMR)
    {
      newOutput = vtkSmartPointer<vtkOverlappingAMR>::New();
    }
    else if (dataSetType == VTK_POLY_DATA)
    {
      newOutput = vtkSmartPointer<vtkPolyData>::New();
    }
    else
    {
      vtkErrorMacro("HDF dataset type unknown: " << dataSetType);
      return 0;
    }
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
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
  // Ensures a new file is open. This happen for vtkFileSeriesReader
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
  else if (dataSetType == VTK_UNSTRUCTURED_GRID || dataSetType == VTK_POLY_DATA)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
  else if (dataSetType == VTK_OVERLAPPING_AMR)
  {
    if (!this->Impl->GetAttribute("Origin", 3, this->Origin))
    {
      return 0;
    }
    outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 0);
  }
  else
  {
    vtkErrorMacro("Invalid dataset type: " << dataSetType);
    return 0;
  }
  // Recover transient data information
  this->HasTransientData = (this->NumberOfSteps > 1);
  if (this->HasTransientData)
  {
    std::vector<double> values(this->NumberOfSteps, 0.0);
    {
      vtkSmartPointer<vtkDataArray> stepValues = vtk::TakeSmartPointer(this->Impl->GetStepValues());
      auto container = vtk::DataArrayValueRange<1>(stepValues);
      std::copy(container.begin(), container.end(), values.begin());
    }
    this->TimeRange[0] = *std::min_element(values.begin(), values.end());
    this->TimeRange[1] = *std::max_element(values.begin(), values.end());
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), values.data(),
      static_cast<int>(values.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), this->TimeRange.data(), 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkHDFReader::PrintPieceInformation(vtkInformation* outInfo)
{
  std::array<int, 6> updateExtent;
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent.data());
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
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent.data());
  data->SetOrigin(this->Origin);
  data->SetSpacing(this->Spacing);
  data->SetExtent(updateExtent.data());
  if (!this->Impl->GetAttribute("Direction", 9, data->GetDirectionMatrix()->GetData()))
  {
    return 0;
  }

  // in the same order as vtkDataObject::AttributeTypes: POINT, CELL
  for (int attributeType = 0; attributeType < vtkDataObject::FIELD; ++attributeType)
  {
    const hsize_t pointModifier = (attributeType == vtkDataObject::POINT) ? 1 : 0;
    std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
    for (const std::string& name : names)
    {
      if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
      {
        vtkSmartPointer<vtkDataArray> array;
        std::vector<hsize_t> fileExtent = ::ReduceDimension(updateExtent.data(), this->WholeExtent);
        std::vector<int> extentBuffer(fileExtent.size(), 0);
        std::copy(
          updateExtent.begin(), updateExtent.begin() + extentBuffer.size(), extentBuffer.begin());
        if (this->HasTransientData)
        {
          vtkIdType offset = this->Impl->GetArrayOffset(this->Step, attributeType, name);
          if (offset >= 0)
          {
            extentBuffer.emplace_back(offset);
            extentBuffer.emplace_back(offset);
          }
          else
          {
            extentBuffer.emplace_back(this->Step);
            extentBuffer.emplace_back(this->Step);
          }
          fileExtent.resize(extentBuffer.size(), 0);
        }
        // Create the memory space, reverse axis order for VTK fortran order,
        // because VTK stores 2D/3D arrays in memory along columns (fortran order) rather
        // than along rows (C order)
        for (std::size_t iDim = 0; iDim < fileExtent.size() / 2; ++iDim)
        {
          std::size_t rIDim = (fileExtent.size() / 2) - 1 - iDim;
          // if an extent value is negative it won't go into an hsize_t
          if (extentBuffer[rIDim * 2] < 0)
          {
            extentBuffer[rIDim * 2 + 1] -= extentBuffer[rIDim * 2];
            extentBuffer[rIDim * 2] = 0;
          }
          fileExtent[iDim * 2] = extentBuffer[rIDim * 2];
          fileExtent[iDim * 2 + 1] = extentBuffer[rIDim * 2 + 1] + pointModifier;
        }
        if (this->HasTransientData && !pointModifier)
        {
          // Add one to the extent for the time dimension if needed
          fileExtent[1] += 1;
        }
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
int vtkHDFReader::AddFieldArrays(vtkDataObject* data)
{
  std::vector<std::string> names = this->Impl->GetArrayNames(vtkDataObject::FIELD);
  for (const std::string& name : names)
  {
    vtkSmartPointer<vtkAbstractArray> array;
    vtkIdType offset = -1;
    vtkIdType size = -1;
    if (this->HasTransientData)
    {
      // If the field data is transient we expect it to have NumberSteps number of tuples
      // and as many components as necessary
      size = 1;
      offset = this->Impl->GetArrayOffset(this->Step, vtkDataObject::FIELD, name);
    }
    if ((array = vtk::TakeSmartPointer(this->Impl->NewFieldArray(name.c_str(), offset, size))) ==
      nullptr)
    {
      vtkErrorMacro("Error reading array " << name);
      return 0;
    }
    array->SetName(name.c_str());
    if (this->HasTransientData)
    {
      vtkIdType len = array->GetNumberOfComponents();
      array->SetNumberOfComponents(1);
      array->SetNumberOfTuples(len);
    }
    data->GetAttributesAsFieldData(vtkDataObject::FIELD)->AddArray(array);
  }
  if (this->HasTransientData)
  {
    vtkNew<vtkDoubleArray> time;
    time->SetName("Time");
    time->SetNumberOfComponents(1);
    time->SetNumberOfTuples(1);
    time->SetValue(0, this->TimeValue);
    data->GetAttributesAsFieldData(vtkDataObject::FIELD)->AddArray(time);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(const std::vector<vtkIdType>& numberOfPoints,
  const std::vector<vtkIdType>& numberOfCells,
  const std::vector<vtkIdType>& numberOfConnectivityIds, vtkIdType partOffset,
  vtkIdType startingPointOffset, vtkIdType startingCellOffset,
  vtkIdType startingConnectivityIdOffset, int filePiece, vtkUnstructuredGrid* pieceData)
{
  // read the piece and add it to data
  vtkNew<vtkPoints> points;
  vtkSmartPointer<vtkDataArray> pointArray;
  vtkIdType pointOffset =
    std::accumulate(numberOfPoints.data(), &numberOfPoints[filePiece], startingPointOffset);
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
  // the offsets array has (numberOfCells[part] + 1) elements per part.
  vtkIdType offset = std::accumulate(
    numberOfCells.data(), &numberOfCells[filePiece], startingCellOffset + partOffset + filePiece);
  if ((offsetsArray = vtk::TakeSmartPointer(
         this->Impl->NewMetadataArray("Offsets", offset, numberOfCells[filePiece] + 1))) == nullptr)
  {
    vtkErrorMacro("Cannot read the Offsets array");
    return 0;
  }
  offset = std::accumulate(numberOfConnectivityIds.data(), &numberOfConnectivityIds[filePiece],
    startingConnectivityIdOffset);
  if ((connectivityArray = vtk::TakeSmartPointer(this->Impl->NewMetadataArray(
         "Connectivity", offset, numberOfConnectivityIds[filePiece]))) == nullptr)
  {
    vtkErrorMacro("Cannot read the Connectivity array");
    return 0;
  }
  cellArray->SetData(offsetsArray, connectivityArray);

  vtkIdType cellOffset =
    std::accumulate(numberOfCells.data(), &numberOfCells[filePiece], startingCellOffset);
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
  std::vector<vtkIdType> startingOffsets = { startingPointOffset, startingCellOffset };
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
        vtkIdType arrayOffset = offsets[attributeType];
        if (this->HasTransientData)
        {
          vtkIdType buff = this->Impl->GetArrayOffset(this->Step, attributeType, name);
          if (buff >= 0)
          {
            arrayOffset += buff - startingOffsets[attributeType];
          }
        }
        vtkSmartPointer<vtkDataArray> array;
        if ((array = vtk::TakeSmartPointer(this->Impl->NewArray(attributeType, name.c_str(),
               arrayOffset, (*numberOf[attributeType])[filePiece]))) == nullptr)
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
  int filePieceCount = this->Impl->GetNumberOfPieces(this->Step);
  vtkIdType partOffset = 0;
  vtkIdType startingPointOffset = 0;
  vtkIdType startingCellOffset = 0;
  vtkIdType startingConnectivityIdOffset = 0;
  if (this->HasTransientData)
  {
    ::TransientGeometryOffsets geoOffs(this->Impl, this->Step);
    if (!geoOffs.Success)
    {
      vtkErrorMacro("Error in reading transient geometry offsets");
      return 0;
    }
    partOffset = geoOffs.PartOffset;
    startingPointOffset = geoOffs.PointOffset;
    startingCellOffset = geoOffs.CellOffsets[0];
    startingConnectivityIdOffset = geoOffs.ConnectivityOffsets[0];
  }
  std::vector<vtkIdType> numberOfPoints =
    this->Impl->GetMetadata("NumberOfPoints", filePieceCount, partOffset);
  if (numberOfPoints.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfCells =
    this->Impl->GetMetadata("NumberOfCells", filePieceCount, partOffset);
  if (numberOfCells.empty())
  {
    return 0;
  }
  std::vector<vtkIdType> numberOfConnectivityIds =
    this->Impl->GetMetadata("NumberOfConnectivityIds", filePieceCount, partOffset);
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
    if (!this->Read(numberOfPoints, numberOfCells, numberOfConnectivityIds, partOffset,
          startingPointOffset, startingCellOffset, startingConnectivityIdOffset, filePiece,
          pieceData))
    {
      return 0;
    }
    append->Update();
    data->ShallowCopy(append->GetOutput());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* outInfo, vtkPolyData* data)
{
  // The number of pieces in this step
  int filePieceCount = this->Impl->GetNumberOfPieces();
  if (this->HasTransientData)
  {
    filePieceCount = this->Impl->GetNumberOfPieces(this->Step);
  }

  // The initial offsetting with which to read the step in particular
  vtkIdType partOffset = 0;
  vtkIdType startingPointOffset = 0;
  std::vector<vtkIdType> startingCellOffsets(::NUM_POLY_DATA_TOPOS, 0);
  std::vector<vtkIdType> startingConnectivityIdOffsets(::NUM_POLY_DATA_TOPOS, 0);
  if (this->HasTransientData)
  {
    // Read the time offsets for this step
    ::TransientGeometryOffsets geoOffs(this->Impl, this->Step);
    if (!geoOffs.Success)
    {
      vtkErrorMacro("Error in reading transient geometry offsets");
      return 0;
    }
    // bring these offsets up in scope
    partOffset = geoOffs.PartOffset;
    startingPointOffset = geoOffs.PointOffset;
    std::swap(startingCellOffsets, geoOffs.CellOffsets);
    std::swap(startingConnectivityIdOffsets, geoOffs.ConnectivityOffsets);
  }

  // extract the array containing the number of points for this step
  std::vector<vtkIdType> numberOfPoints =
    this->Impl->GetMetadata("NumberOfPoints", filePieceCount, partOffset);
  if (numberOfPoints.empty())
  {
    vtkErrorMacro("Error in reading NumberOfPoints");
    return 0;
  }

  std::map<std::string, std::vector<vtkIdType>> numberOfCells;
  std::map<std::string, std::vector<vtkIdType>> numberOfConnectivityIds;
  for (const auto& name : ::POLY_DATA_TOPOS)
  {
    // extract the array containing the number of cells of this topology for this step
    numberOfCells[name] =
      this->Impl->GetMetadata((name + "/NumberOfCells").c_str(), filePieceCount, partOffset);
    if (numberOfCells[name].empty())
    {
      vtkErrorMacro("Error in reading NumberOfCells for " + name);
      return 0;
    }
    // extract the array containing the number of connectivity ids of this topology for this step
    numberOfConnectivityIds[name] = this->Impl->GetMetadata(
      (name + "/NumberOfConnectivityIds").c_str(), filePieceCount, partOffset);
    if (numberOfConnectivityIds[name].empty())
    {
      vtkErrorMacro("Error in reading NumberOfConnectivityIds for " + name);
      return 0;
    }
  }
  // determine the stride to use when updating pieces
  int memoryPieceCount = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  // determine the initial piece number to update
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  // initialize the appending filter mechanism
  vtkNew<vtkPolyData> pieceData;
  vtkNew<vtkAppendDataSets> append;
  append->SetOutputDataSetType(VTK_POLY_DATA);
  append->AddInputData(data);
  append->AddInputData(pieceData);
  vtkIdType startingCellOffset =
    std::accumulate(startingCellOffsets.begin(), startingCellOffsets.end(), 0);
  for (int filePiece = piece; filePiece < filePieceCount; filePiece += memoryPieceCount)
  {
    // determine the exact offsetting for the piece that needs to be read
    vtkIdType pointOffset =
      std::accumulate(numberOfPoints.data(), &numberOfPoints[filePiece], startingPointOffset);
    std::vector<vtkIdType> cellOffsets(::NUM_POLY_DATA_TOPOS, 0);
    std::vector<vtkIdType> pieceNumberOfCells(::NUM_POLY_DATA_TOPOS, 0);
    std::vector<vtkIdType> connectivityOffsets(::NUM_POLY_DATA_TOPOS, 0);
    std::vector<vtkIdType> pieceNumberOfConnectivityIds(::NUM_POLY_DATA_TOPOS, 0);
    for (std::size_t iTopo = 0; iTopo < ::NUM_POLY_DATA_TOPOS; ++iTopo)
    {
      const auto& nCells = numberOfCells[::POLY_DATA_TOPOS[iTopo]];
      cellOffsets[iTopo] = std::accumulate(nCells.begin(), nCells.begin() + filePiece,
        startingCellOffsets[iTopo] + partOffset + filePiece);
      pieceNumberOfCells[iTopo] = nCells[filePiece];
      const auto& nConnectivity = numberOfConnectivityIds[::POLY_DATA_TOPOS[iTopo]];
      connectivityOffsets[iTopo] = std::accumulate(nConnectivity.begin(),
        nConnectivity.begin() + filePiece, startingConnectivityIdOffsets[iTopo]);
      pieceNumberOfConnectivityIds[iTopo] = nConnectivity[filePiece];
    }

    // populate the poly data piece
    pieceData->Initialize();
    if (!::ReadPolyDataPiece(this->Impl, pointOffset, numberOfPoints[filePiece], cellOffsets,
          pieceNumberOfCells, connectivityOffsets, pieceNumberOfConnectivityIds, pieceData))
    {
      vtkErrorMacro(
        "There was an error in reading the " << filePiece << " piece of the poly data file.");
      return 0;
    }

    // sum over topologies to get total offsets for fields
    vtkIdType cellOffset = startingCellOffset;
    for (const auto& name : ::POLY_DATA_TOPOS)
    {
      const auto& nCells = numberOfCells[name];
      cellOffset = std::accumulate(nCells.begin(), nCells.begin() + filePiece, cellOffset);
    }
    vtkIdType accumulatedNumberOfCells =
      std::accumulate(pieceNumberOfCells.begin(), pieceNumberOfCells.end(), 0);

    std::vector<vtkIdType> offsets = { pointOffset, cellOffset };
    std::vector<vtkIdType> startingOffsets = { startingPointOffset, startingCellOffset };
    std::vector<vtkIdType> numberOf = { numberOfPoints[filePiece], accumulatedNumberOfCells };
    for (int attributeType = 0; attributeType < vtkDataObject::FIELD; ++attributeType)
    {
      std::vector<std::string> names = this->Impl->GetArrayNames(attributeType);
      for (const std::string& name : names)
      {
        if (this->DataArraySelection[attributeType]->ArrayIsEnabled(name.c_str()))
        {
          vtkIdType arrayOffset = offsets[attributeType];
          if (this->HasTransientData)
          {
            vtkIdType buff = this->Impl->GetArrayOffset(this->Step, attributeType, name);
            if (buff >= 0)
            {
              arrayOffset += buff - startingOffsets[attributeType];
            }
          }
          vtkSmartPointer<vtkDataArray> array;
          if ((array = vtk::TakeSmartPointer(this->Impl->NewArray(
                 attributeType, name.c_str(), arrayOffset, numberOf[attributeType]))) == nullptr)
          {
            vtkErrorMacro("Error reading array " << name);
            return 0;
          }
          array->SetName(name.c_str());
          pieceData->GetAttributesAsFieldData(attributeType)->AddArray(array);
        }
      }
    }
    append->Update();
    data->ShallowCopy(append->GetOutput());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHDFReader::Read(vtkInformation* vtkNotUsed(outInfo), vtkOverlappingAMR* data)
{
  data->SetOrigin(this->Origin);

  if (!this->Impl->FillAMR(
        data, this->MaximumLevelsToReadByDefaultForAMR, this->Origin, this->DataArraySelection))
  {
    return 0;
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
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!output)
  {
    return 0;
  }
  if (this->HasTransientData)
  {
    double* values = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      double requestedValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      this->Step = std::distance(values,
                     std::upper_bound(values, values + this->NumberOfSteps, requestedValue)) -
        1;
      this->Step = this->Step >= this->NumberOfSteps ? this->NumberOfSteps - 1
                                                     : (this->Step < 0 ? 0 : this->Step);
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->TimeValue);
    }
    this->TimeValue = values[this->Step];
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
  else if (dataSetType == VTK_POLY_DATA)
  {
    vtkPolyData* data = vtkPolyData::SafeDownCast(output);
    ok = this->Read(outInfo, data);
  }
  else if (dataSetType == VTK_OVERLAPPING_AMR)
  {
    vtkOverlappingAMR* data = vtkOverlappingAMR::SafeDownCast(output);
    ok = this->Read(outInfo, data);
  }
  else
  {
    vtkErrorMacro("HDF dataset type unknown: " << dataSetType);
    return 0;
  }
  return ok && this->AddFieldArrays(output);
}
VTK_ABI_NAMESPACE_END
