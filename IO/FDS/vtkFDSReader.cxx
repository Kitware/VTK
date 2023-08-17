// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFDSReader.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkFileResourceStream.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkResourceParser.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

#include <fstream>
#include <limits>

#include <sstream>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
enum BaseNodes
{
  GRIDS = 0,
  DEVICES = 1,
  HRR = 2,
  SLICES = 3,
  BOUNDARIES = 4
};

//------------------------------------------------------------------------------
// Constants
const std::vector<std::string> BASE_NODES = { "Grids", "Devices", "HRR", "Slices", "Boundaries" };
const std::vector<std::string> DIM_KEYWORDS = { "TRNX", "TRNY", "TRNZ" };

//------------------------------------------------------------------------------
struct FDSParser
{
  void Init(vtkResourceStream* stream)
  {
    this->Parser->Reset();
    this->Parser->SetStream(stream);
    this->Parser->StopOnNewLineOn();
    this->LineNumber = 0;
    this->Result = vtkParseResult::EndOfLine;
  }

  template <typename T>
  bool Parse(T& output)
  {
    this->Result = this->Parser->Parse(output);
    if (this->Result != vtkParseResult::Ok)
    {
      this->LineNumber += (this->Result == vtkParseResult::EndOfLine ? 1 : 0);
      return false;
    }
    return true;
  }

  bool DiscardLine()
  {
    this->Result = this->Parser->DiscardLine();
    if (this->Result != vtkParseResult::EndOfLine)
    {
      return false;
    }
    this->LineNumber++;
    return true;
  }

  vtkNew<vtkResourceParser> Parser;
  vtkParseResult Result = vtkParseResult::EndOfLine;
  int LineNumber = 0; // current line
};

//------------------------------------------------------------------------------
struct GridData
{
  vtkSmartPointer<vtkRectilinearGrid> Geometry;
  unsigned int GridNb;
};

//------------------------------------------------------------------------------
struct ObstacleData
{
  vtkSmartPointer<vtkRectilinearGrid> Geometry;
  vtkIdType BlockageNumber;
  GridData* AssociatedGrid;
};

//------------------------------------------------------------------------------
struct BoundaryFieldData
{
  vtkIdType GridID;
  std::string FieldName;
  std::string FileName;
  std::vector<float> TimeValues;
};

//------------------------------------------------------------------------------
struct SliceData
{
  std::array<vtkIdType, 6> GridSubExtent;
  unsigned int AssociatedGridNumber;
  std::string FileName;
  bool CellCentered = false;
  std::vector<float> TimeValues;
};

//------------------------------------------------------------------------------
struct DeviceData
{
  std::string Name;
  std::array<double, 3> Position = { 0.0, 0.0, 0.0 };
  std::array<double, 3> Direction = { 0.0, 0.0, 0.0 };
};

//------------------------------------------------------------------------------
vtkSmartPointer<vtkRectilinearGrid> GenerateSubGrid(
  vtkRectilinearGrid* original, std::array<vtkIdType, 6>& subExtent)
{
  vtkNew<vtkRectilinearGrid> subGrid;

  std::array<int, 3> dimensions = { 0 };
  for (std::size_t iDim = 0; iDim < 3; ++iDim)
  {
    dimensions[iDim] = static_cast<int>(subExtent[2 * iDim + 1] - subExtent[2 * iDim]) + 1;
  }

  subGrid->SetDimensions(dimensions.data());

  std::array<vtkDataArray*, 3> gridCoords = { original->GetXCoordinates(),
    original->GetYCoordinates(), original->GetZCoordinates() };
  for (std::size_t iDim = 0; iDim < 3; ++iDim)
  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfComponents(1);
    coords->SetNumberOfTuples(dimensions[iDim]);
    for (vtkIdType iCoord = 0; iCoord < dimensions[iDim]; ++iCoord)
    {
      coords->SetValue(iCoord, gridCoords[iDim]->GetComponent(subExtent[2 * iDim] + iCoord, 0));
    }
    switch (iDim)
    {
      case 0:
        subGrid->SetXCoordinates(coords);
        break;
      case 1:
        subGrid->SetYCoordinates(coords);
        break;
      case 2:
        subGrid->SetZCoordinates(coords);
        break;
      default:
        break;
    }
  }

  return subGrid;
}

//------------------------------------------------------------------------------
void ReadAndExtractRowFromCSV(vtkDelimitedTextReader* reader, const std::string& fileName,
  vtkIdType requestedTimeStep, std::map<std::string, vtkSmartPointer<vtkDataArray>>& map)
{
  reader->SetFileName(fileName.c_str());
  reader->Update();
  auto table = vtkTable::SafeDownCast(reader->GetOutput());
  const auto rowData = table->GetRowData();
  for (int iCol = 0; iCol < rowData->GetNumberOfArrays(); ++iCol)
  {
    auto arr = vtkStringArray::SafeDownCast(rowData->GetAbstractArray(iCol));
    vtkNew<vtkDoubleArray>
      Value; // potentially change later with new instance once CSV reader can skip first line
    Value->SetNumberOfComponents(1);
    Value->SetNumberOfTuples(1);
    Value->SetValue(
      0, std::stod(arr->GetValue(requestedTimeStep + 2))); // csv file has two headers technically
    map.emplace(arr->GetValue(1), Value);
  }
}

//------------------------------------------------------------------------------
std::vector<float> ParseTimeStepsInSliceFile(const std::string& fileName)
{
  vtkNew<vtkFileResourceStream> fileStream;
  if (fileName.empty() || !fileStream->Open(fileName.c_str()))
  {
    vtkErrorWithObjectMacro(nullptr,
      << "Failed to open file: " << (fileName.empty() ? fileName : "No file name for slice given"));
    return std::vector<float>();
  }

  vtkNew<vtkResourceParser> parser;
  parser->Reset();
  parser->SetStream(fileStream);
  parser->StopOnNewLineOff();

  unsigned int size = 0;
  // skip header lines
  for (vtkIdType iL = 0; iL < 4; ++iL)
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    parser->ReadUntil(
      vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
  }

  std::vector<float> timeValues;
  vtkParseResult result = vtkParseResult::Ok;
  do
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    if (size != sizeof(float))
    {
      break;
    }
    float time = 0.0;
    parser->Read(reinterpret_cast<char*>(&time), 4);
    timeValues.emplace_back(time);
    parser->Read(reinterpret_cast<char*>(&size), 4);
    parser->Read(reinterpret_cast<char*>(&size), 4);
    result = parser->ReadUntil(
      vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
  } while (result != vtkParseResult::EndOfStream);
  return timeValues;
}

//------------------------------------------------------------------------------
// Since the file format does not describe the endianness, we won't be dealing with that
vtkSmartPointer<vtkDataArray> ReadSliceFile(const std::string& fileName,
  vtkIdType requestedTimeStep, vtkIdType nTuples, vtkIdType nComponents)
{
  // two lines per time step + one time value line + 4 header lines
  vtkIdType targetLineNumber = requestedTimeStep * 2 + 1 + 4;

  vtkNew<vtkFileResourceStream> fileStream;
  if (fileName.empty() || !fileStream->Open(fileName.c_str()))
  {
    vtkErrorWithObjectMacro(nullptr,
      << "Failed to open file: " << (fileName.empty() ? fileName : "No file name for slice given"));
    return nullptr;
  }

  vtkNew<vtkResourceParser> parser;
  parser->Reset();
  parser->SetStream(fileStream);
  parser->StopOnNewLineOff();

  unsigned int size = 0;
  for (vtkIdType iL = 0; iL < targetLineNumber; ++iL)
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    parser->ReadUntil(
      vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
  }

  parser->Read(reinterpret_cast<char*>(&size), 4);
  std::size_t nBytesFloat = nComponents * nTuples * sizeof(float);
  std::size_t nBytesDouble = nComponents * nTuples * sizeof(double);

  if (size != nBytesFloat && size != nBytesDouble)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Line length seems to be " << size << " bytes when expected " << nBytesFloat
                                 << " for floats and " << nBytesDouble << " for doubles");
    return nullptr;
  }

  vtkSmartPointer<vtkDataArray> result;
  if (size == nBytesFloat)
  {
    result = vtkSmartPointer<vtkFloatArray>::New();
  }
  else
  {
    result = vtkSmartPointer<vtkDoubleArray>::New();
  }
  result->SetNumberOfComponents(nComponents);
  result->SetNumberOfTuples(nTuples);
  std::size_t readBytes = parser->Read(reinterpret_cast<char*>(result->GetVoidPointer(0)), size);
  if (readBytes != size)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Did not read correct number of bytes from file, expected to read " << size << " but read "
                                                                          << readBytes);
    return nullptr;
  }

  return result;
}

//------------------------------------------------------------------------------
std::vector<float> ParseTimeStepsInBoundaryFile(const std::string& fileName)
{
  vtkNew<vtkFileResourceStream> fileStream;
  if (fileName.empty() || !fileStream->Open(fileName.c_str()))
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Failed to open file: "
               << (fileName.empty() ? fileName : "No file name for boundary given"));
    return std::vector<float>();
  }

  vtkNew<vtkResourceParser> parser;
  parser->Reset();
  parser->SetStream(fileStream);
  parser->StopOnNewLineOff();

  unsigned int size = 0;
  // skip header lines
  for (vtkIdType iL = 0; iL < 3; ++iL)
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    parser->ReadUntil(
      vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
  }

  // read number of patches
  parser->Read(reinterpret_cast<char*>(&size), 4);
  unsigned int nBlockages = 0;
  parser->Read(reinterpret_cast<char*>(&nBlockages), size);
  parser->Read(reinterpret_cast<char*>(&size), 4);

  std::vector<float> timeValues;
  vtkParseResult result = vtkParseResult::Ok;
  do
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    if (size != sizeof(float))
    {
      break;
    }
    float time = 0.0;
    parser->Read(reinterpret_cast<char*>(&time), 4);
    timeValues.emplace_back(time);
    parser->Read(reinterpret_cast<char*>(&size), 4);
    for (unsigned int iBlock = 0; iBlock < nBlockages; ++iBlock)
    {
      parser->Read(reinterpret_cast<char*>(&size), 4);
      result = parser->ReadUntil(
        vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
    }
  } while (result != vtkParseResult::EndOfStream);
  return timeValues;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> ReadBoundaryFile(const std::string& fileName,
  vtkIdType requestedTimeStep, vtkIdType blockageNumber, vtkIdType nTuples, vtkIdType nComponents)
{
  vtkNew<vtkFileResourceStream> fileStream;
  if (fileName.empty() || !fileStream->Open(fileName.c_str()))
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Failed to open file: "
               << (fileName.empty() ? fileName : "No file name for boundary given"));
    return nullptr;
  }

  vtkNew<vtkResourceParser> parser;
  parser->Reset();
  parser->SetStream(fileStream);
  parser->StopOnNewLineOff();

  unsigned int size = 0;
  // skip header lines
  for (vtkIdType iL = 0; iL < 3; ++iL)
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    parser->ReadUntil(
      vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
  }

  // read number of patches
  parser->Read(reinterpret_cast<char*>(&size), 4);
  unsigned int nBlockages = 0;
  parser->Read(reinterpret_cast<char*>(&nBlockages), size);
  parser->Read(reinterpret_cast<char*>(&size), 4);

  bool foundBlock = false;
  vtkIdType blockPos = 0;
  for (unsigned int iBlock = 0; iBlock < nBlockages; ++iBlock)
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    std::vector<int> line(size / 4);
    if (line.size() != 9)
    {
      vtkErrorWithObjectMacro(nullptr, "Error in reading blockage dimensions");
      return nullptr;
    }
    parser->Read(reinterpret_cast<char*>(line.data()), size);
    parser->Read(reinterpret_cast<char*>(&size), 4);
    if (static_cast<vtkIdType>(line[7]) == blockageNumber)
    {
      blockPos = static_cast<vtkIdType>(iBlock);
      foundBlock = true;
    }
  }

  if (!foundBlock)
  {
    vtkWarningWithObjectMacro(nullptr,
      "Could not find blockage " << blockageNumber << " in file " << fileName
                                 << ". Returning zeroed array.");
    vtkNew<vtkFloatArray> result;
    result->SetNumberOfComponents(nComponents);
    result->SetNumberOfTuples(nTuples);
    return result;
  }

  vtkIdType nLinesToSkip =
    (static_cast<vtkIdType>(nBlockages) + 1) * requestedTimeStep + blockPos + 1;
  for (vtkIdType iL = 0; iL < nLinesToSkip; ++iL)
  {
    parser->Read(reinterpret_cast<char*>(&size), 4);
    parser->ReadUntil(
      vtkResourceParser::DiscardNone, [](const char*, std::size_t) {}, size + 4);
  }

  parser->Read(reinterpret_cast<char*>(&size), 4);
  std::size_t nBytesFloat = nComponents * nTuples * sizeof(float);
  std::size_t nBytesDouble = nComponents * nTuples * sizeof(double);

  if (size != nBytesFloat && size != nBytesDouble)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Line length seems to be " << size << " bytes when expected " << nBytesFloat
                                 << " for floats and " << nBytesDouble << " for doubles");
    return nullptr;
  }

  vtkSmartPointer<vtkDataArray> result;
  if (size == nBytesFloat)
  {
    result = vtkSmartPointer<vtkFloatArray>::New();
  }
  else
  {
    result = vtkSmartPointer<vtkDoubleArray>::New();
  }
  result->SetNumberOfComponents(nComponents);
  result->SetNumberOfTuples(nTuples);
  std::size_t readBytes = parser->Read(reinterpret_cast<char*>(result->GetVoidPointer(0)), size);
  if (readBytes != size)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Did not read correct number of bytes from file, expected to read " << size << " but read "
                                                                          << readBytes);
    return nullptr;
  }

  return result;
}

//------------------------------------------------------------------------------
template <typename InternalsT>
class vtkFDSGridVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSGridVisitor* New() { VTK_STANDARD_NEW_BODY(vtkFDSGridVisitor<InternalsT>); }
  vtkTypeMacro(vtkFDSGridVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (!this->Internals || !this->OutputPDSC)
    {
      vtkErrorMacro("Must set internals and output pointers before using grid visitor");
      return;
    }
    auto itGrid = this->Internals->Grids.find(nodeId);
    if (itGrid == this->Internals->Grids.end())
    {
      return;
    }

    unsigned int lastIndex = this->OutputPDSC->GetNumberOfPartitionedDataSets();
    this->OutputPDSC->SetNumberOfPartitionedDataSets(lastIndex + 1);
    this->OutputPDSC->GetDataAssembly()->AddDataSetIndex(nodeId, lastIndex);
    this->OutputPDSC->SetPartition(lastIndex, 0, itGrid->second.Geometry);
    this->OutputPDSC->GetMetaData(lastIndex)->Set(
      vtkCompositeDataSet::NAME(), this->OutputPDSC->GetDataAssembly()->GetNodeName(nodeId));
  }

  std::shared_ptr<InternalsT> Internals = nullptr;
  vtkPartitionedDataSetCollection* OutputPDSC = nullptr;

protected:
  vtkFDSGridVisitor() = default;
  ~vtkFDSGridVisitor() override = default;

private:
  vtkFDSGridVisitor(const vtkFDSGridVisitor&) = delete;
  void operator=(const vtkFDSGridVisitor&) = delete;
};

//------------------------------------------------------------------------------
template <typename InternalsT>
class vtkFDSDeviceVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSDeviceVisitor* New() { VTK_STANDARD_NEW_BODY(vtkFDSDeviceVisitor<InternalsT>); }
  vtkTypeMacro(vtkFDSDeviceVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (!this->Internals || !this->OutputPDSC)
    {
      vtkErrorMacro("Must set internals and output before using device visitor");
      return;
    }
    auto itDevice = this->Internals->Devices.find(nodeId);
    if (itDevice == this->Internals->Devices.end())
    {
      return;
    }

    // Is a valid leaf
    auto& dData = itDevice->second;
    auto itMap = this->DeviceValueMap.find(dData.Name);
    if (itMap == this->DeviceValueMap.end())
    {
      vtkErrorMacro("Did not read value for device with name " << dData.Name);
      return;
    }

    vtkNew<vtkPolyData> device;

    auto setSingleVector = [](vtkDoubleArray* arr, const std::array<double, 3>& vec) {
      arr->SetNumberOfComponents(3);
      arr->SetNumberOfTuples(1);
      for (std::size_t iDim = 0; iDim < 3; ++iDim)
      {
        arr->SetValue(iDim, vec[iDim]);
      }
    };

    // generate point
    vtkNew<vtkPoints> pContainer;
    vtkNew<vtkDoubleArray> pArray;
    setSingleVector(pArray, dData.Position);
    pContainer->SetData(pArray);
    device->SetPoints(pContainer);

    // generate vertex
    vtkNew<vtkCellArray> verts;
    vtkNew<vtkIdTypeArray> offset;
    vtkNew<vtkIdTypeArray> connectivity;
    offset->SetNumberOfComponents(1);
    offset->SetNumberOfTuples(2);
    offset->SetValue(0, 0);
    offset->SetValue(1, 1);
    connectivity->SetNumberOfComponents(1);
    connectivity->SetNumberOfTuples(1);
    connectivity->SetValue(0, 0);
    verts->SetData(offset, connectivity);
    device->SetVerts(verts);

    // set point data
    vtkNew<vtkDoubleArray> direction;
    direction->SetName("Direction");
    setSingleVector(direction, dData.Direction);
    device->GetPointData()->AddArray(direction);
    itMap->second->SetName("Value");
    device->GetPointData()->AddArray(itMap->second);

    unsigned int lastIndex = this->OutputPDSC->GetNumberOfPartitionedDataSets();
    this->OutputPDSC->SetNumberOfPartitionedDataSets(lastIndex + 1);
    this->OutputPDSC->GetDataAssembly()->AddDataSetIndex(nodeId, lastIndex);
    this->OutputPDSC->SetPartition(lastIndex, 0, device);
    this->OutputPDSC->GetMetaData(lastIndex)->Set(
      vtkCompositeDataSet::NAME(), this->OutputPDSC->GetDataAssembly()->GetNodeName(nodeId));
  }

  std::map<std::string, vtkSmartPointer<vtkDataArray>> DeviceValueMap;

  std::shared_ptr<InternalsT> Internals = nullptr;
  vtkPartitionedDataSetCollection* OutputPDSC = nullptr;

protected:
  vtkFDSDeviceVisitor() = default;
  ~vtkFDSDeviceVisitor() override = default;

private:
  vtkFDSDeviceVisitor(const vtkFDSDeviceVisitor&) = delete;
  void operator=(const vtkFDSDeviceVisitor&) = delete;
};

//------------------------------------------------------------------------------
template <typename InternalsT>
class vtkFDSHRRVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSHRRVisitor* New() { VTK_STANDARD_NEW_BODY(vtkFDSHRRVisitor<InternalsT>); }
  vtkTypeMacro(vtkFDSHRRVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (!this->Internals || !this->OutputPDSC)
    {
      vtkErrorMacro("Must set internals and output pointers before using grid visitor");
      return;
    }
    auto itHRR = this->Internals->HRRFiles.find(nodeId);
    if (itHRR == this->Internals->HRRFiles.end())
    {
      return;
    }

    std::map<std::string, vtkSmartPointer<vtkDataArray>> hrrMap;
    vtkNew<vtkDelimitedTextReader> reader;
    ReadAndExtractRowFromCSV(reader, itHRR->second, this->RequestedTimeStep, hrrMap);

    // transfer back to table
    vtkNew<vtkTable> result;
    for (const auto& pair : hrrMap)
    {
      pair.second->SetName(pair.first.c_str());
      result->GetRowData()->AddArray(pair.second);
    }

    unsigned int lastIndex = this->OutputPDSC->GetNumberOfPartitionedDataSets();
    this->OutputPDSC->SetNumberOfPartitionedDataSets(lastIndex + 1);
    this->OutputPDSC->GetDataAssembly()->AddDataSetIndex(nodeId, lastIndex);
    this->OutputPDSC->SetPartition(lastIndex, 0, result);
    this->OutputPDSC->GetMetaData(lastIndex)->Set(
      vtkCompositeDataSet::NAME(), this->OutputPDSC->GetDataAssembly()->GetNodeName(nodeId));
  }

  std::shared_ptr<InternalsT> Internals = nullptr;
  vtkPartitionedDataSetCollection* OutputPDSC = nullptr;
  vtkIdType RequestedTimeStep = 0;

protected:
  vtkFDSHRRVisitor() = default;
  ~vtkFDSHRRVisitor() override = default;

private:
  vtkFDSHRRVisitor(const vtkFDSHRRVisitor&) = delete;
  void operator=(const vtkFDSHRRVisitor&) = delete;
};

//------------------------------------------------------------------------------
template <typename InternalsT>
class vtkFDSSliceVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSSliceVisitor* New() { VTK_STANDARD_NEW_BODY(vtkFDSSliceVisitor<InternalsT>); }
  vtkTypeMacro(vtkFDSSliceVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (!this->Internals || !this->OutputPDSC)
    {
      vtkErrorMacro("Must set internals and output pointers before using grid visitor");
      return;
    }

    auto itSlice = this->Internals->Slices.find(nodeId);
    if (itSlice == this->Internals->Slices.end())
    {
      return;
    }

    auto& sData = itSlice->second;

    // make grid
    const GridData* gData = nullptr;
    for (const auto& pair : this->Internals->Grids)
    {
      if (pair.second.GridNb == sData.AssociatedGridNumber)
      {
        gData = &(pair.second);
        break;
      }
    }

    if (!gData)
    {
      vtkErrorMacro(
        "Could not find grid number " << sData.AssociatedGridNumber << " associated to slice.");
      return;
    }

    vtkIdType requestedTimeStep = std::distance(sData.TimeValues.begin(),
                                    std::upper_bound(sData.TimeValues.begin(),
                                      sData.TimeValues.end(), this->RequestedTimeValue)) -
      1;
    requestedTimeStep = requestedTimeStep >= static_cast<vtkIdType>(sData.TimeValues.size())
      ? sData.TimeValues.size() - 1
      : (requestedTimeStep < 0 ? 0 : requestedTimeStep);

    vtkSmartPointer<vtkRectilinearGrid> slice =
      ::GenerateSubGrid(gData->Geometry, sData.GridSubExtent);
    vtkIdType nEntities =
      sData.CellCentered ? slice->GetNumberOfCells() : slice->GetNumberOfPoints();
    vtkSmartPointer<vtkDataArray> field =
      ::ReadSliceFile(sData.FileName, requestedTimeStep, nEntities, 1);
    if (!field)
    {
      vtkErrorMacro(
        "Could not read slice " << this->OutputPDSC->GetDataAssembly()->GetNodeName(nodeId));
      return;
    }
    field->SetName("Values");
    vtkFieldData* fData = sData.CellCentered ? static_cast<vtkFieldData*>(slice->GetCellData())
                                             : static_cast<vtkFieldData*>(slice->GetPointData());
    fData->AddArray(field);

    unsigned int lastIndex = this->OutputPDSC->GetNumberOfPartitionedDataSets();
    this->OutputPDSC->SetNumberOfPartitionedDataSets(lastIndex + 1);
    this->OutputPDSC->GetDataAssembly()->AddDataSetIndex(nodeId, lastIndex);
    this->OutputPDSC->SetPartition(lastIndex, 0, slice);
    this->OutputPDSC->GetMetaData(lastIndex)->Set(
      vtkCompositeDataSet::NAME(), this->OutputPDSC->GetDataAssembly()->GetNodeName(nodeId));
  }

  std::shared_ptr<InternalsT> Internals = nullptr;
  vtkPartitionedDataSetCollection* OutputPDSC = nullptr;
  double RequestedTimeValue = 0.0;

protected:
  vtkFDSSliceVisitor() = default;
  ~vtkFDSSliceVisitor() override = default;

private:
  vtkFDSSliceVisitor(const vtkFDSSliceVisitor&) = delete;
  void operator=(const vtkFDSSliceVisitor&) = delete;
};

//------------------------------------------------------------------------------
template <typename InternalsT>
class vtkFDSBoundaryVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSBoundaryVisitor* New() { VTK_STANDARD_NEW_BODY(vtkFDSBoundaryVisitor<InternalsT>); }
  vtkTypeMacro(vtkFDSBoundaryVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (!this->Internals || !this->OutputPDSC)
    {
      vtkErrorMacro("Must set internals and output pointers before using grid visitor");
      return;
    }

    auto itBound = this->Internals->Boundaries.find(nodeId);
    if (itBound == this->Internals->Boundaries.end())
    {
      return;
    }

    auto& oData = itBound->second;

    vtkNew<vtkRectilinearGrid> copy;
    copy->ShallowCopy(oData.Geometry);

    for (const auto& bfieldData : this->Internals->BoundaryFields)
    {
      if (oData.AssociatedGrid->GridNb != bfieldData.GridID)
      {
        continue;
      }

      vtkIdType requestedTimeStep = std::distance(bfieldData.TimeValues.begin(),
                                      std::upper_bound(bfieldData.TimeValues.begin(),
                                        bfieldData.TimeValues.end(), this->RequestedTimeValue)) -
        1;
      requestedTimeStep = requestedTimeStep >= static_cast<vtkIdType>(bfieldData.TimeValues.size())
        ? bfieldData.TimeValues.size() - 1
        : (requestedTimeStep < 0 ? 0 : requestedTimeStep);

      vtkSmartPointer<vtkDataArray> field = ::ReadBoundaryFile(
        bfieldData.FileName, requestedTimeStep, oData.BlockageNumber, copy->GetNumberOfPoints(), 1);
      if (!field)
      {
        vtkWarningMacro("Could not correctly read " << bfieldData.FieldName << " for blockage "
                                                    << oData.BlockageNumber << " on grid "
                                                    << bfieldData.GridID + 1);
        continue;
      }
      field->SetName(bfieldData.FieldName.c_str());
      copy->GetPointData()->AddArray(field);
    }

    unsigned int lastIndex = this->OutputPDSC->GetNumberOfPartitionedDataSets();
    this->OutputPDSC->SetNumberOfPartitionedDataSets(lastIndex + 1);
    this->OutputPDSC->GetDataAssembly()->AddDataSetIndex(nodeId, lastIndex);
    this->OutputPDSC->SetPartition(lastIndex, 0, copy);
    this->OutputPDSC->GetMetaData(lastIndex)->Set(
      vtkCompositeDataSet::NAME(), this->OutputPDSC->GetDataAssembly()->GetNodeName(nodeId));
  }

  std::shared_ptr<InternalsT> Internals = nullptr;
  vtkPartitionedDataSetCollection* OutputPDSC = nullptr;
  double RequestedTimeValue = 0;

protected:
  vtkFDSBoundaryVisitor() = default;
  ~vtkFDSBoundaryVisitor() override = default;

private:
  vtkFDSBoundaryVisitor(const vtkFDSBoundaryVisitor&) = delete;
  void operator=(const vtkFDSBoundaryVisitor&) = delete;
};
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
struct vtkFDSReader::vtkInternals
{
  // Maps used to retrieve filename(s) relative to
  // a given "leaf" in the data assembly
  std::map<int, ::GridData> Grids;
  std::map<int, ::DeviceData> Devices;
  std::map<int, ::SliceData> Slices;
  std::map<int, ::ObstacleData> Boundaries;
  std::map<int, std::string> HRRFiles;
  std::set<std::string> DevcFiles;
  std::vector<::BoundaryFieldData> BoundaryFields;

  std::vector<double> ViewTimes;

  unsigned int MaxNbOfPartitions = 0;
  unsigned int GridCount = 0;

  std::array<double, 2> TimeRange;
  std::vector<double> TimeValues;
  vtkIdType NumberOfTimeSteps = 1;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFDSReader);

//------------------------------------------------------------------------------
vtkFDSReader::vtkFDSReader()
  : Internals(new vtkInternals())
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkFDSReader::~vtkFDSReader() = default;

//------------------------------------------------------------------------------
void vtkFDSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkFDSReader::AddSelector(const char* selector)
{
  if (selector && this->Selectors.insert(selector).second)
  {
    this->Modified();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkFDSReader::ClearSelectors()
{
  if (!this->Selectors.empty())
  {
    this->Selectors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkFDSReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkResourceStream> stream = this->Open();
  if (!stream)
  {
    vtkErrorMacro(<< "Request information : failed to open stream");
    return 0;
  }

  // Fill base structure
  this->Assembly->Initialize();
  const auto baseNodes = this->Assembly->AddNodes(BASE_NODES);

  if (this->FileName.empty())
  {
    vtkErrorMacro("RequestInformation called for FDSReader without file name");
    return 0;
  }

  std::string rootNodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
  this->Assembly->SetNodeName(vtkDataAssembly::GetRootNode(), rootNodeName.c_str());
  std::string FDSRootDir = vtksys::SystemTools::GetFilenamePath(this->FileName);

  this->Internals->MaxNbOfPartitions = 0;
  this->Internals->GridCount = 0;
  this->Internals->DevcFiles.clear();
  this->Internals->BoundaryFields.clear();

  FDSParser parser;
  parser.Init(stream);

  // Main parsing loop
  do
  {
    std::string keyWord;
    if (!parser.Parse(keyWord))
    {
      // Let the loop handle the parsing result
      continue;
    }

    if (keyWord == "VIEWTIMES")
    {
      // throw away rest of line
      if (!parser.DiscardLine())
      {
        continue;
      }

      double beginTime = 0.0;
      if (!parser.Parse(beginTime))
      {
        vtkErrorMacro("Could not parse begin time at line " << parser.LineNumber);
        return 0;
      }

      double endTime = 0.0;
      if (!parser.Parse(endTime))
      {
        vtkErrorMacro("Could not parse end time at line " << parser.LineNumber);
        return 0;
      }

      vtkIdType nTimeSteps = 0;
      if (!parser.Parse(nTimeSteps))
      {
        vtkErrorMacro("Could not parse number of time steps at line " << parser.LineNumber);
        return 0;
      }

      // throw away rest of line
      if (!parser.DiscardLine())
      {
        continue;
      }

      this->Internals->ViewTimes.resize(nTimeSteps, 0.0);
      double timeStep = (endTime - beginTime) / (nTimeSteps > 1 ? nTimeSteps - 1 : 1);
      for (vtkIdType iStep = 0; iStep < nTimeSteps; ++iStep)
      {
        this->Internals->ViewTimes[iStep] = beginTime + iStep * timeStep;
      }
    }
    else if (keyWord == "GRID")
    {
      // Retrieve grid name
      std::string gridName;
      if (!parser.Parse(gridName))
      {
        continue;
      }

      // Skip the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      vtkNew<vtkRectilinearGrid> grid;

      // Parse grid dimensions
      int dimensions[3] = { 0, 0, 0 };
      for (std::size_t i = 0; i < 3; ++i)
      {
        if (!parser.Parse(dimensions[i]))
        {
          vtkErrorMacro(<< "Failed to parse the grid dimension " << i << " at line "
                        << parser.LineNumber);
          return 0;
        }
        dimensions[i] += 1;
      }

      grid->SetDimensions(dimensions);

      // Discard the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Discard empty line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Expected : PDIM keyword
      if (!parser.Parse(keyWord))
      {
        continue;
      }

      if (keyWord != "PDIM")
      {
        vtkErrorMacro(<< "Expected a PDIM keyword at line " << parser.LineNumber
                      << ", but none was found.");
        return 0;
      }

      // Discard the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // We throw away the grid extent since we will read the rectilinear subdivisions later
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Discard empty line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse X/Y/Z coordinates
      for (int dim = 0; dim < 3; dim++)
      {
        if (!parser.Parse(keyWord))
        {
          vtkErrorMacro(
            "Could not read line " << parser.LineNumber << " where expected TRN{X/Y/Z}");
          return 0;
        }

        // We should have TRNX, TRNY or TRNZ
        if (keyWord != ::DIM_KEYWORDS[dim])
        {
          vtkErrorMacro(<< "Expected a " << ::DIM_KEYWORDS[dim] << " keyword at line "
                        << parser.LineNumber << ", but none was found.");
          return 0;
        }

        // Discard the rest of the line
        if (!parser.DiscardLine())
        {
          continue;
        }

        // Discard line (contains unused info)
        if (!parser.DiscardLine())
        {
          continue;
        }

        vtkNew<vtkDoubleArray> coordArray;
        coordArray->SetNumberOfComponents(1);
        coordArray->SetNumberOfTuples(dimensions[dim]);

        // Iterate over all coordinates along current axis
        for (vtkIdType id = 0; id < dimensions[dim]; id++)
        {
          // Parse index
          vtkIdType i = 0;
          if (!parser.Parse(i))
          {
            return 0;
          }

          if (i != id)
          {
            vtkErrorMacro(<< "Wrong dimention found. Expected " << id << ", got " << i << ".");
            return 0;
          }

          // Parse X/Y/Z coordinates value
          double d = 0.;
          if (!parser.Parse(d))
          {
            vtkErrorMacro("Failed to parse coord value at line " << parser.LineNumber);
            return 0;
          }

          coordArray->SetValue(id, d);

          // Discard the rest of the line
          if (!parser.DiscardLine())
          {
            continue;
          }
        }

        switch (dim)
        {
          case 0:
            grid->SetXCoordinates(coordArray);
            break;
          case 1:
            grid->SetYCoordinates(coordArray);
            break;
          case 2:
            grid->SetZCoordinates(coordArray);
            break;
          default:
            break;
        }

        // Discard empty line
        if (!parser.DiscardLine())
        {
          continue;
        }
      }

      // Register grid and fill assembly
      const int idx = this->Assembly->AddNode(gridName.c_str(), baseNodes[GRIDS]);
      ::GridData gridData;
      gridData.GridNb = this->Internals->GridCount;
      this->Internals->GridCount++;
      gridData.Geometry = grid;
      this->Internals->Grids.emplace(idx, gridData);
      this->Internals->MaxNbOfPartitions++;

      // Parse obstacles
      std::string obst;
      if (!parser.Parse(obst))
      {
        vtkErrorMacro(
          "Could not parse line " << parser.LineNumber << " where OBST keyword was expected.");
        return 0;
      }

      if (obst != "OBST")
      {
        vtkErrorMacro("Found " << obst << " at line " << parser.LineNumber
                               << " where OBST keyword was expected.");
        return 0;
      }

      // discard the rest of the OBST line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // read the number of blockages composing the obstacle
      vtkIdType nBlockages;
      if (!parser.Parse(nBlockages))
      {
        vtkErrorMacro("Could not read line " << parser.LineNumber
                                             << " where expected number of blocakges of obstacle.");
        return 0;
      }

      // discard the rest of the nBlockages line
      if (!parser.DiscardLine())
      {
        continue;
      }

      std::vector<::ObstacleData> gridBoundaries;
      for (vtkIdType iBlock = 0; iBlock < nBlockages; ++iBlock)
      {
        ::ObstacleData oData;
        for (vtkIdType iExtent = 0; iExtent < 6; ++iExtent)
        {
          // throw out the extents which are duplicate info with the grid indexes that come after
          double trash = 0.0;
          if (!parser.Parse(trash))
          {
            vtkErrorMacro("Could not parse " << iExtent << " obstacle extent value at line "
                                             << parser.LineNumber);
            return 0;
          }
        }

        // Get blockage ID
        if (!parser.Parse(oData.BlockageNumber))
        {
          vtkErrorMacro("Could not parse obstacle blockage ID at line " << parser.LineNumber);
          return 0;
        }

        // Discard the rest of the line
        if (!parser.DiscardLine())
        {
          continue;
        }
        gridBoundaries.emplace_back(oData);
      }

      for (vtkIdType iBlock = 0; iBlock < nBlockages; ++iBlock)
      {
        auto& oData = gridBoundaries[iBlock];
        oData.AssociatedGrid = &(this->Internals->Grids[idx]);
        std::array<vtkIdType, 6> subExtent;
        for (vtkIdType iExtent = 0; iExtent < 6; ++iExtent)
        {
          // throw out the extents which are duplicate info with the grid indexes that come after
          if (!parser.Parse(subExtent[iExtent]))
          {
            vtkErrorMacro("Could not parse " << iExtent << " obstacle sub extent value at line "
                                             << parser.LineNumber);
            return 0;
          }
        }

        oData.Geometry = ::GenerateSubGrid(this->Internals->Grids[idx].Geometry, subExtent);

        // Discard the rest of the line
        if (!parser.DiscardLine())
        {
          continue;
        }
      }

      for (vtkIdType iBlock = 0; iBlock < nBlockages; ++iBlock)
      {
        auto& oData = gridBoundaries[iBlock];
        const int bIdx = this->Assembly->AddNode(
          (gridName + "_Blockage_" + std::to_string(oData.BlockageNumber)).c_str(),
          baseNodes[::BOUNDARIES]);
        this->Internals->Boundaries.emplace(bIdx, oData);
        this->Internals->MaxNbOfPartitions++;
      }
    }
    else if (keyWord == "CSVF")
    {
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse CSV file type
      // Possible values are : hrr, devc
      std::string fileType;
      if (!parser.Parse(fileType))
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse CSV file type.");
        continue;
      }

      if (fileType == "devc")
      {
        if (!parser.DiscardLine())
        {
          continue;
        }

        // Parse devc file path
        std::string fileName;
        if (!parser.Parse(fileName))
        {
          vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse devc file path.");
          continue;
        }

        std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

        // add FDS root to file name to get full path
        fileName = FDSRootDir + "/" + fileName;

        if (!vtksys::SystemTools::TestFileAccess(
              fileName, vtksys::TEST_FILE_OK | vtksys::TEST_FILE_READ))
        {
          vtkErrorMacro("Device file " << fileName
                                       << " is not accessible either because it does not exist or "
                                          "it does not have the right permissions.");
          return 0;
        }

        // Register file path and fill assembly
        this->Internals->DevcFiles.insert(fileName);
      }
      else if (fileType == "hrr")
      {
        if (!parser.DiscardLine())
        {
          continue;
        }

        // Parse hrr file path
        std::string fileName;
        if (!parser.Parse(fileName))
        {
          vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse hrr file path.");
          continue;
        }

        std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

        // add FDS root to file name to get full path
        fileName = FDSRootDir + "/" + fileName;

        if (!vtksys::SystemTools::TestFileAccess(
              fileName, vtksys::TEST_FILE_OK | vtksys::TEST_FILE_READ))
        {
          vtkErrorMacro("HRR file " << fileName
                                    << " is not accessible either because it does not exist or "
                                       "it does not have the right permissions.");
          return 0;
        }

        // Register file path and fill assembly
        const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[HRR]);
        this->Internals->HRRFiles.emplace(idx, fileName);
        this->Internals->MaxNbOfPartitions++;
      }
      else
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unknown CSV file type.");
      }
    }
    else if (keyWord == "DEVICE")
    {
      // discard rest of DEVICE line
      if (!parser.DiscardLine())
      {
        continue;
      }

      ::DeviceData dData;

      // read name
      if (!parser.Parse(dData.Name))
      {
        vtkErrorMacro(
          "Could not parse line " << parser.LineNumber << " where name of device was expected");
        return 0;
      }

      // discard the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // read position
      for (std::size_t iDim = 0; iDim < 3; ++iDim)
      {
        if (!parser.Parse(dData.Position[iDim]))
        {
          vtkErrorMacro(
            "Could not parse " << iDim << " position coordinate at line " << parser.LineNumber);
          return 0;
        }
      }

      // read direction
      for (std::size_t iDim = 0; iDim < 3; ++iDim)
      {
        if (!parser.Parse(dData.Direction[iDim]))
        {
          vtkErrorMacro(
            "Could not parse " << iDim << " direction value at line " << parser.LineNumber);
          return 0;
        }
      }

      // discard the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Register file path and fill assembly
      const int idx = this->Assembly->AddNode(dData.Name.c_str(), baseNodes[DEVICES]);
      this->Internals->Devices.emplace(idx, dData);
      this->Internals->MaxNbOfPartitions++;
    }
    else if (keyWord == "SLCF" || keyWord == "SLCC")
    {
      ::SliceData sData;

      // Contrary to the documentation, it would seem this is false
      // if (keyWord == "SLCC")
      //{
      // sData.CellCentered = true;
      //}

      // Parse grid ID
      if (!parser.Parse(sData.AssociatedGridNumber))
      {
        continue;
      }
      // FDS counting starts at 1
      sData.AssociatedGridNumber -= 1;

      // Search for dimensions
      // We can have a specified slice ID before that but it's not mandatory
      std::string name;
      if (!parser.Parse(name))
      {
        vtkErrorMacro("Could not parse name of slice at line " << parser.LineNumber);
        return 0;
      }

      // remove % from beginning of name
      name.erase(0, 1);

      std::string ampersand;
      if (!parser.Parse(ampersand))
      {
        vtkErrorMacro(
          "Error parsing ampersand at end of " << name << " at line " << parser.LineNumber);
        return 0;
      }

      if (ampersand != "&")
      {
        vtkErrorMacro("Expected & at end of " << name << " at line " << parser.LineNumber);
        return 0;
      }

      for (std::size_t iExtent = 0; iExtent < 6; ++iExtent)
      {
        if (!parser.Parse(sData.GridSubExtent[iExtent]))
        {
          vtkErrorMacro(<< "Unable to parse slice sub-extent index " << iExtent << " at line "
                        << parser.LineNumber);
          return 0;
        }
      }

      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse .sf file path
      if (!parser.Parse(sData.FileName))
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse sf file path.");
        continue;
      }

      // add FDS root to file name to get full path
      sData.FileName = FDSRootDir + "/" + sData.FileName;

      if (!vtksys::SystemTools::TestFileAccess(
            sData.FileName, vtksys::TEST_FILE_OK | vtksys::TEST_FILE_READ))
      {
        vtkErrorMacro("Slice file " << sData.FileName
                                    << " is not accessible either because it does not exist or "
                                       "it does not have the right permissions.");
        return 0;
      }

      sData.TimeValues = ::ParseTimeStepsInSliceFile(sData.FileName);

      const int idx = this->Assembly->AddNode(name.c_str(), baseNodes[SLICES]);
      this->Internals->Slices.emplace(idx, sData);
      this->Internals->MaxNbOfPartitions++;
    }
    else if (keyWord == "BNDF")
    {
      ::BoundaryFieldData bfData;

      if (!parser.Parse(bfData.GridID))
      {
        vtkErrorMacro(
          "Could not parse line " << parser.LineNumber << " where was expecting a grid ID");
        return 0;
      }
      bfData.GridID -= 1;

      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse bf file path
      if (!parser.Parse(bfData.FileName))
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse bf file path.");
        continue;
      }

      // add FDS root to file name to get full path
      bfData.FileName = FDSRootDir + "/" + bfData.FileName;

      if (!vtksys::SystemTools::TestFileAccess(
            bfData.FileName, vtksys::TEST_FILE_OK | vtksys::TEST_FILE_READ))
      {
        vtkErrorMacro("Boundary file " << bfData.FileName
                                       << " is not accessible either because it does not exist or "
                                          "it does not have the right permissions.");
        return 0;
      }

      bfData.TimeValues = ::ParseTimeStepsInBoundaryFile(bfData.FileName);

      // discard rest of line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // discard caps definition
      if (!parser.DiscardLine())
      {
        continue;
      }

      // get name of field
      if (!parser.Parse(bfData.FieldName))
      {
        vtkErrorMacro("Could not parse line " << parser.LineNumber
                                              << " where was expecting boundary field name");
        return 0;
      }

      this->Internals->BoundaryFields.emplace_back(bfData);
    }

    if (!parser.DiscardLine())
    {
      continue;
    }
  } while (parser.Result == vtkParseResult::EndOfLine);

  // The last result that ended the loop
  if (parser.Result != vtkParseResult::EndOfStream)
  {
    vtkErrorMacro(<< "Error during parsing of SMV file at line " << parser.LineNumber);
    return 0;
  }

  // Update Assembly widget
  this->AssemblyTag += 1;

  // Assemble all time values and then remove all those that are considered equal with
  // tolerance of this->TimeTolerance
  std::set<double> uniqueSortedTimeValues;

  auto insertIntoSet = [&uniqueSortedTimeValues](
                         double val) { uniqueSortedTimeValues.insert(val); };

  std::for_each(
    this->Internals->ViewTimes.begin(), this->Internals->ViewTimes.end(), insertIntoSet);
  for (auto& pair : this->Internals->Slices)
  {
    std::for_each(pair.second.TimeValues.begin(), pair.second.TimeValues.end(), insertIntoSet);
  }
  for (auto& bfData : this->Internals->BoundaryFields)
  {
    std::for_each(bfData.TimeValues.begin(), bfData.TimeValues.end(), insertIntoSet);
  }

  if (!uniqueSortedTimeValues.empty())
  {
    this->Internals->TimeValues.clear();
    this->Internals->TimeValues.emplace_back(*(uniqueSortedTimeValues.begin()));
    for (auto& val : uniqueSortedTimeValues)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            this->Internals->TimeValues.back(), val, this->TimeTolerance))
      {
        this->Internals->TimeValues.emplace_back(val);
      }
    }
    this->Internals->TimeRange[0] = this->Internals->TimeValues.front();
    this->Internals->TimeRange[1] = this->Internals->TimeValues.back();
  }
  this->Internals->NumberOfTimeSteps = this->Internals->TimeValues.size();

  // Update time information
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->Internals->TimeValues.data(),
    static_cast<int>(this->Internals->TimeValues.size()));
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::TIME_RANGE(), this->Internals->TimeRange.data(), 2);

  return 1;
}

// ----------------------------------------------------------------------------
int vtkFDSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outInfo);

  if (!output)
  {
    vtkErrorMacro("Unable to retrieve the output !");
    return 0;
  }

  vtkSmartPointer<vtkResourceStream> stream = this->Open();
  if (!stream)
  {
    vtkErrorMacro(<< "Request data: failed to open stream");
    return 0;
  }

  const std::vector<std::string> selectors(this->Selectors.begin(), this->Selectors.end());
  const auto selectedNodes = this->Assembly->SelectNodes(selectors);

  double requestedTimeValue = 0;
  vtkIdType requestedViewTimeStep = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double* timeValues = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    vtkIdType requestedTimeStep =
      std::distance(timeValues,
        std::upper_bound(
          timeValues, timeValues + this->Internals->NumberOfTimeSteps, requestedTimeValue)) -
      1;
    requestedTimeStep = requestedTimeStep >= this->Internals->NumberOfTimeSteps
      ? this->Internals->NumberOfTimeSteps - 1
      : (requestedTimeStep < 0 ? 0 : requestedTimeStep);
    output->GetInformation()->Set(
      vtkDataObject::DATA_TIME_STEP(), this->Internals->TimeValues[requestedTimeStep]);

    auto& viewTimes = this->Internals->ViewTimes;
    if (!viewTimes.empty())
    {
      requestedViewTimeStep =
        std::distance(viewTimes.begin(),
          std::upper_bound(viewTimes.begin(), viewTimes.end(), requestedTimeValue)) -
        1;
      requestedViewTimeStep = static_cast<std::size_t>(requestedTimeStep) >= viewTimes.size()
        ? viewTimes.size() - 1
        : (requestedViewTimeStep < 0 ? 0 : requestedViewTimeStep);
    }
  }

  vtkNew<vtkDataAssembly> outAssembly;
  outAssembly->SubsetCopy(this->Assembly, selectedNodes);
  output->SetDataAssembly(outAssembly);
  output->SetNumberOfPartitionedDataSets(0);

  int gridIdx =
    outAssembly->FindFirstNodeWithName("Grids", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (gridIdx != -1)
  {
    using GridVisitor = ::vtkFDSGridVisitor<vtkInternals>;
    vtkNew<GridVisitor> gridVisitor;
    gridVisitor->Internals = this->Internals;
    gridVisitor->OutputPDSC = output;
    outAssembly->Visit(gridIdx, gridVisitor);
  }

  int devicesIdx =
    outAssembly->FindFirstNodeWithName("Devices", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (devicesIdx != -1)
  {
    using DeviceVisitor = ::vtkFDSDeviceVisitor<vtkInternals>;
    vtkNew<DeviceVisitor> deviceVisitor;
    deviceVisitor->Internals = this->Internals;
    deviceVisitor->OutputPDSC = output;
    vtkNew<vtkDelimitedTextReader> csvReader;
    for (const auto& fileName : this->Internals->DevcFiles)
    {
      ::ReadAndExtractRowFromCSV(
        csvReader, fileName, requestedViewTimeStep, deviceVisitor->DeviceValueMap);
    }
    outAssembly->Visit(devicesIdx, deviceVisitor);
  }

  int hrrsIdx =
    outAssembly->FindFirstNodeWithName("HRR", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (hrrsIdx != -1)
  {
    using HRRVisitor = ::vtkFDSHRRVisitor<vtkInternals>;
    vtkNew<HRRVisitor> hrrVisitor;
    hrrVisitor->Internals = this->Internals;
    hrrVisitor->OutputPDSC = output;
    hrrVisitor->RequestedTimeStep = requestedViewTimeStep;
    outAssembly->Visit(hrrsIdx, hrrVisitor);
  }

  int sliceIdx =
    outAssembly->FindFirstNodeWithName("Slices", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (sliceIdx != -1)
  {
    using SliceVisitor = ::vtkFDSSliceVisitor<vtkInternals>;
    vtkNew<SliceVisitor> sliceVisitor;
    sliceVisitor->Internals = this->Internals;
    sliceVisitor->OutputPDSC = output;
    sliceVisitor->RequestedTimeValue = requestedTimeValue;
    outAssembly->Visit(sliceIdx, sliceVisitor);
  }

  int boundaryIdx =
    outAssembly->FindFirstNodeWithName("Boundaries", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (boundaryIdx != -1)
  {
    using BoundaryVisitor = ::vtkFDSBoundaryVisitor<vtkInternals>;
    vtkNew<BoundaryVisitor> boundaryVisitor;
    boundaryVisitor->Internals = this->Internals;
    boundaryVisitor->OutputPDSC = output;
    boundaryVisitor->RequestedTimeValue = requestedTimeValue;
    outAssembly->Visit(boundaryIdx, boundaryVisitor);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkFDSReader::Open()
{
  if (this->Stream)
  {
    if (this->Stream->SupportSeek())
    {
      this->Stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
    }

    return this->Stream;
  }

  auto fileStream = vtkSmartPointer<vtkFileResourceStream>::New();
  if (this->FileName.empty() || !fileStream->Open(this->FileName.c_str()))
  {
    vtkErrorMacro(<< "Failed to open file: "
                  << (this->FileName.empty() ? this->FileName : "No file name set"));
    return nullptr;
  }

  return fileStream;
}

VTK_ABI_NAMESPACE_END
