// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAppendPolyData.h"

#include "vtkAlgorithmOutput.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAppendPolyData);

//------------------------------------------------------------------------------
vtkAppendPolyData::vtkAppendPolyData()
{
  this->ParallelStreaming = 0;
  this->UserManagedInputs = 0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//------------------------------------------------------------------------------
vtkAppendPolyData::~vtkAppendPolyData() = default;

//------------------------------------------------------------------------------
// Add a dataset to the list of data to append.
void vtkAppendPolyData::AddInputData(vtkPolyData* ds)
{
  if (this->UserManagedInputs)
  {
    vtkErrorMacro(<< "AddInput is not supported if UserManagedInputs is true");
    return;
  }
  this->Superclass::AddInputData(ds);
}

//------------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendPolyData::RemoveInputData(vtkPolyData* ds)
{
  if (this->UserManagedInputs)
  {
    vtkErrorMacro(<< "RemoveInput is not supported if UserManagedInputs is true");
    return;
  }

  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0, this->GetInputConnection(0, i));
    }
  }
}

//------------------------------------------------------------------------------
// make ProcessObject function visible
// should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetNumberOfInputs(int num)
{
  if (!this->UserManagedInputs)
  {
    vtkErrorMacro(<< "SetNumberOfInputs is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to set the number of connections.
  this->SetNumberOfInputConnections(0, num);
}

//------------------------------------------------------------------------------
void vtkAppendPolyData::SetInputDataByNumber(int num, vtkPolyData* input)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(input);
  this->SetInputConnectionByNumber(num, tp->GetOutputPort());
  tp->Delete();
}

//------------------------------------------------------------------------------
// Set Nth input, should only be used when UserManagedInputs is true.
void vtkAppendPolyData::SetInputConnectionByNumber(int num, vtkAlgorithmOutput* input)
{
  if (!this->UserManagedInputs)
  {
    vtkErrorMacro(<< "SetInputConnectionByNumber is not supported if UserManagedInputs is false");
    return;
  }

  // Ask the superclass to connect the input.
  this->SetNthInputConnection(0, num, input);
}

namespace
{
struct AppendCellArray
{
  template <typename CellStateT>
  void operator()(CellStateT& state, vtkIdTypeArray* outputOffsets,
    vtkIdTypeArray* outputConnectivity, vtkIdType cellOffset, vtkIdType cellConnectivityOffset,
    vtkIdType pointOffset)
  {
    using ValueType = typename CellStateT::ValueType;
    auto inputOffsets = state.GetOffsets();
    auto inputConnectivity = state.GetConnectivity();
    auto numberOfCells = inputOffsets->GetNumberOfValues() - 1;
    auto numberOfConnectivityIds = inputConnectivity->GetNumberOfValues();

    // Copy the offsets and transform them using the cellConnectivityOffset
    std::transform(inputOffsets->GetPointer(0), inputOffsets->GetPointer(numberOfCells),
      outputOffsets->GetPointer(cellOffset),
      [&](ValueType offset) { return static_cast<ValueType>(offset + cellConnectivityOffset); });
    // Copy the connectivity and transform them using the pointOffset
    std::transform(inputConnectivity->GetPointer(0),
      inputConnectivity->GetPointer(numberOfConnectivityIds),
      outputConnectivity->GetPointer(cellConnectivityOffset),
      [&](ValueType ptId) { return static_cast<vtkIdType>(ptId + pointOffset); });
  }
};
}

//------------------------------------------------------------------------------
int vtkAppendPolyData::ExecuteAppend(vtkPolyData* output, vtkPolyData* inputs[], int numInputs)
{
  std::vector<vtkPolyData*> datasets;
  for (int i = 0; i < numInputs; ++i)
  {
    if (inputs[i] && inputs[i]->GetNumberOfPoints() > 0)
    {
      datasets.push_back(inputs[i]);
    }
  }
  vtkIdType totalNumberOfPoints = 0;
  vtkIdType totalNumberOfCells = 0;
  vtkIdType totalNumberOfVerts = 0;
  vtkIdType totalNumberOfVertsConnectivity = 0;
  vtkIdType totalNumberOfLines = 0;
  vtkIdType totalNumberOfLinesConnectivity = 0;
  vtkIdType totalNumberOfPolys = 0;
  vtkIdType totalNumberOfPolysConnectivity = 0;
  vtkIdType totalNumberOfStrips = 0;
  vtkIdType totalNumberOfStripsConnectivity = 0;
  std::vector<vtkIdType> pointOffsets(datasets.size());
  std::vector<vtkIdType> vertOffsets(datasets.size());
  std::vector<vtkIdType> vertConnectivityOffsets(datasets.size());
  std::vector<vtkIdType> lineOffsets(datasets.size());
  std::vector<vtkIdType> lineConnectivityOffsets(datasets.size());
  std::vector<vtkIdType> polyOffsets(datasets.size());
  std::vector<vtkIdType> polyConnectivityOffsets(datasets.size());
  std::vector<vtkIdType> stripOffsets(datasets.size());
  std::vector<vtkIdType> stripConnectivityOffsets(datasets.size());
  for (size_t idx = 0; idx < datasets.size(); ++idx)
  {
    auto& dataset = datasets[idx];
    pointOffsets[idx] = totalNumberOfPoints;
    vertOffsets[idx] = totalNumberOfVerts;
    vertConnectivityOffsets[idx] = totalNumberOfVertsConnectivity;
    lineOffsets[idx] = totalNumberOfLines;
    lineConnectivityOffsets[idx] = totalNumberOfLinesConnectivity;
    polyOffsets[idx] = totalNumberOfPolys;
    polyConnectivityOffsets[idx] = totalNumberOfPolysConnectivity;
    stripOffsets[idx] = totalNumberOfStrips;
    stripConnectivityOffsets[idx] = totalNumberOfStripsConnectivity;

    totalNumberOfPoints += dataset->GetNumberOfPoints();
    totalNumberOfCells += dataset->GetNumberOfCells();
    totalNumberOfVerts += dataset->GetNumberOfVerts();
    totalNumberOfVertsConnectivity += dataset->GetVerts()->GetNumberOfConnectivityIds();
    totalNumberOfLines += dataset->GetNumberOfLines();
    totalNumberOfLinesConnectivity += dataset->GetLines()->GetNumberOfConnectivityIds();
    totalNumberOfPolys += dataset->GetNumberOfPolys();
    totalNumberOfPolysConnectivity += dataset->GetPolys()->GetNumberOfConnectivityIds();
    totalNumberOfStrips += dataset->GetNumberOfStrips();
    totalNumberOfStripsConnectivity += dataset->GetStrips()->GetNumberOfConnectivityIds();
  }

  if (totalNumberOfPoints < 1)
  {
    vtkDebugMacro(<< "No data to append!");
    return 1;
  }

  vtkNew<vtkPoints> newPoints;
  // set precision for the points in the output
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    int dataType = 0;

    // Keep track of types for fast point append
    for (const auto& dataset : datasets)
    {
      if (dataset->GetNumberOfPoints() > 0)
      {
        auto type = dataset->GetPoints()->GetDataType();
        dataType = std::max(dataType, type);
      }
    }
    newPoints->SetDataType(dataType);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }
  vtkDebugMacro(<< "Appending polydata");

  newPoints->SetNumberOfPoints(totalNumberOfPoints);
  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        const auto& dataset = datasets[idx];
        // copy points
        newPoints->GetData()->InsertTuples(
          pointOffsets[idx], dataset->GetNumberOfPoints(), 0, dataset->GetPoints()->GetData());
      }
    });
  output->SetPoints(newPoints);
  this->UpdateProgress(0.25);
  if (this->CheckAbort())
  {
    return 1;
  }
  // Since points are cells are not merged,
  // this filter can easily pass all field arrays, including global ids.
  auto outputPD = output->GetPointData();
  outputPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);

  // copy arrays.
  vtkDataSetAttributes::FieldList pointFieldList;
  for (const auto& dataset : datasets)
  {
    pointFieldList.IntersectFieldList(dataset->GetPointData());
  }
  outputPD->CopyAllocate(pointFieldList, totalNumberOfPoints);
  outputPD->SetNumberOfTuples(totalNumberOfPoints);
  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        auto& dataset = datasets[idx];
        const auto& pointOffset = pointOffsets[idx];
        auto inputPD = dataset->GetPointData();
        const auto numberOfInputTuples = inputPD->GetNumberOfTuples();
        pointFieldList.CopyData(idx, inputPD, 0, numberOfInputTuples, outputPD, pointOffset);
      }
    });
  this->UpdateProgress(0.50);
  if (this->CheckAbort())
  {
    return 1;
  }

  vtkNew<vtkIdTypeArray> vertsOffsetsArray;
  vtkNew<vtkIdTypeArray> vertsConnectivityArray;
  if (totalNumberOfVerts > 0)
  {
    vertsOffsetsArray->SetNumberOfValues(totalNumberOfVerts + 1);
    vertsOffsetsArray->SetValue(totalNumberOfVerts, totalNumberOfVertsConnectivity);
    vertsConnectivityArray->SetNumberOfValues(totalNumberOfVertsConnectivity);
  }
  vtkNew<vtkIdTypeArray> linesOffsetsArray;
  vtkNew<vtkIdTypeArray> linesConnectivityArray;
  if (totalNumberOfLines > 0)
  {
    linesOffsetsArray->SetNumberOfValues(totalNumberOfLines + 1);
    linesOffsetsArray->SetValue(totalNumberOfLines, totalNumberOfLinesConnectivity);
    linesConnectivityArray->SetNumberOfValues(totalNumberOfLinesConnectivity);
  }
  vtkNew<vtkIdTypeArray> polysOffsetsArray;
  vtkNew<vtkIdTypeArray> polysConnectivityArray;
  if (totalNumberOfPolys > 0)
  {
    polysOffsetsArray->SetNumberOfValues(totalNumberOfPolys + 1);
    polysOffsetsArray->SetValue(totalNumberOfPolys, totalNumberOfPolysConnectivity);
    polysConnectivityArray->SetNumberOfValues(totalNumberOfPolysConnectivity);
  }
  vtkNew<vtkIdTypeArray> stripsOffsetsArray;
  vtkNew<vtkIdTypeArray> stripsConnectivityArray;
  if (totalNumberOfStrips > 0)
  {
    stripsOffsetsArray->SetNumberOfValues(totalNumberOfStrips + 1);
    stripsOffsetsArray->SetValue(totalNumberOfStrips, totalNumberOfStripsConnectivity);
    stripsConnectivityArray->SetNumberOfValues(totalNumberOfStripsConnectivity);
  }

  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        auto& dataset = datasets[idx];
        auto pointOffset = pointOffsets[idx];
        auto vertOffset = vertOffsets[idx];
        auto vertConnectivityOffset = vertConnectivityOffsets[idx];
        auto lineOffset = lineOffsets[idx];
        auto lineConnectivityOffset = lineConnectivityOffsets[idx];
        auto polyOffset = polyOffsets[idx];
        auto polyConnectivityOffset = polyConnectivityOffsets[idx];
        auto stripOffset = stripOffsets[idx];
        auto stripConnectivityOffset = stripConnectivityOffsets[idx];
        if (dataset->GetNumberOfVerts() > 0)
        {
          dataset->GetVerts()->Visit(AppendCellArray{}, vertsOffsetsArray, vertsConnectivityArray,
            vertOffset, vertConnectivityOffset, pointOffset);
        }
        if (dataset->GetNumberOfLines() > 0)
        {
          dataset->GetLines()->Visit(AppendCellArray{}, linesOffsetsArray, linesConnectivityArray,
            lineOffset, lineConnectivityOffset, pointOffset);
        }
        if (dataset->GetNumberOfPolys() > 0)
        {
          dataset->GetPolys()->Visit(AppendCellArray{}, polysOffsetsArray, polysConnectivityArray,
            polyOffset, polyConnectivityOffset, pointOffset);
        }
        if (dataset->GetNumberOfStrips() > 0)
        {
          dataset->GetStrips()->Visit(AppendCellArray{}, stripsOffsetsArray,
            stripsConnectivityArray, stripOffset, stripConnectivityOffset, pointOffset);
        }
      }
    });
  if (totalNumberOfVerts > 0)
  {
    vtkNew<vtkCellArray> verts;
    verts->SetData(vertsOffsetsArray, vertsConnectivityArray);
    output->SetVerts(verts);
  }
  if (totalNumberOfLines > 0)
  {
    vtkNew<vtkCellArray> lines;
    lines->SetData(linesOffsetsArray, linesConnectivityArray);
    output->SetLines(lines);
  }
  if (totalNumberOfPolys > 0)
  {
    vtkNew<vtkCellArray> polys;
    polys->SetData(polysOffsetsArray, polysConnectivityArray);
    output->SetPolys(polys);
  }
  if (totalNumberOfStrips > 0)
  {
    vtkNew<vtkCellArray> strips;
    strips->SetData(stripsOffsetsArray, stripsConnectivityArray);
    output->SetStrips(strips);
  }
  this->UpdateProgress(0.75);
  if (this->CheckAbort())
  {
    return 1;
  }

  // Since points are cells are not merged,
  // this filter can easily pass all field arrays, including global ids.
  auto outputCD = output->GetCellData();
  outputCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);

  // copy arrays.
  vtkDataSetAttributes::FieldList cellFieldList;
  for (const auto& dataset : datasets)
  {
    cellFieldList.IntersectFieldList(dataset->GetCellData());
  }
  outputCD->CopyAllocate(cellFieldList, totalNumberOfCells);
  outputCD->SetNumberOfTuples(totalNumberOfCells);
  // copy arrays.
  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        auto& dataset = datasets[idx];
        const auto outVertOffset = vertOffsets[idx];
        const auto outLineOffset = lineOffsets[idx] + totalNumberOfVerts;
        const auto outPolyOffset = polyOffsets[idx] + totalNumberOfLines + totalNumberOfVerts;
        const auto outStripOffset =
          stripOffsets[idx] + totalNumberOfPolys + totalNumberOfLines + totalNumberOfVerts;
        auto inputCD = dataset->GetCellData();
        if (auto numVerts = dataset->GetNumberOfVerts())
        {
          const vtkIdType inVertOffset = 0;
          cellFieldList.CopyData(idx, inputCD, inVertOffset, numVerts, outputCD, outVertOffset);
        }
        if (auto numLines = dataset->GetNumberOfLines())
        {
          const vtkIdType inLineOffset = dataset->GetNumberOfVerts();
          cellFieldList.CopyData(idx, inputCD, inLineOffset, numLines, outputCD, outLineOffset);
        }
        if (auto numPolys = dataset->GetNumberOfPolys())
        {
          const vtkIdType inPolyOffset = dataset->GetNumberOfLines() + dataset->GetNumberOfVerts();
          cellFieldList.CopyData(idx, inputCD, inPolyOffset, numPolys, outputCD, outPolyOffset);
        }
        if (auto numStrips = dataset->GetNumberOfStrips())
        {
          const vtkIdType inStripOffset =
            dataset->GetNumberOfPolys() + dataset->GetNumberOfLines() + dataset->GetNumberOfVerts();
          cellFieldList.CopyData(idx, inputCD, inStripOffset, numStrips, outputCD, outStripOffset);
        }
      }
    });
  this->UpdateProgress(1.0);

  return 1;
}

//------------------------------------------------------------------------------
// This method is much too long, and has to be broken up!
// Append data sets into single polygonal data set.
int vtkAppendPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info object
  // get the output
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs == 1)
  {
    output->ShallowCopy(vtkPolyData::GetData(inputVector[0], 0));
    return 1;
  }

  vtkPolyData** inputs = new vtkPolyData*[numInputs];
  for (int idx = 0; idx < numInputs; ++idx)
  {
    inputs[idx] = vtkPolyData::GetData(inputVector[0], idx);
  }
  int retVal = this->ExecuteAppend(output, inputs, numInputs);
  delete[] inputs;
  return retVal;
}

//------------------------------------------------------------------------------
int vtkAppendPolyData::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;
  int idx;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  int numInputs = this->GetNumberOfInputConnections(0);
  if (this->ParallelStreaming)
  {
    piece = piece * numInputs;
    numPieces = numPieces * numInputs;
  }

  vtkInformation* inInfo;
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < numInputs; ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
    {
      if (this->ParallelStreaming)
      {
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece + idx);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
      }
      else
      {
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
        inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
      }
    }
  }
  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (idx = 1; idx < numInputs; ++idx)
  {
    vtkInformation* inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkPolyData* vtkAppendPolyData::GetInput(int idx)
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//------------------------------------------------------------------------------
void vtkAppendPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "ParallelStreaming:" << (this->ParallelStreaming ? "On" : "Off") << endl;
  os << "UserManagedInputs:" << (this->UserManagedInputs ? "On" : "Off") << endl;
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << endl;
}

//------------------------------------------------------------------------------
namespace
{
struct AppendDataWorker
{
  vtkIdType Offset;

  AppendDataWorker(vtkIdType offset)
    : Offset(offset)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* dest, Array2T* src)
  {
    VTK_ASSUME(src->GetNumberOfComponents() == dest->GetNumberOfComponents());
    const auto srcTuples = vtk::DataArrayTupleRange(src);

    // Offset the dstTuple range to begin at this->Offset
    auto dstTuples = vtk::DataArrayTupleRange(dest, this->Offset);

    std::copy(srcTuples.cbegin(), srcTuples.cend(), dstTuples.begin());
  }
};
} // end anon namespace

//------------------------------------------------------------------------------
void vtkAppendPolyData::AppendData(vtkDataArray* dest, vtkDataArray* src, vtkIdType offset)
{
  assert("Arrays have same number of components." &&
    src->GetNumberOfComponents() == dest->GetNumberOfComponents());
  assert("Destination array has enough tuples." &&
    src->GetNumberOfTuples() + offset <= dest->GetNumberOfTuples());

  AppendDataWorker worker(offset);
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(dest, src, worker))
  {
    // Use vtkDataArray API when fast-path dispatch fails.
    worker(dest, src);
  }
}

//------------------------------------------------------------------------------
void vtkAppendPolyData::AppendCells(vtkCellArray* dst, vtkCellArray* src, vtkIdType offset)
{
  dst->Append(src, offset);
}

//------------------------------------------------------------------------------
int vtkAppendPolyData::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}
VTK_ABI_NAMESPACE_END
