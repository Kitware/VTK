// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAppendPolyData.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAppendFilter.h"
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
struct AppendCellArray : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdTypeArray* outputOffsets,
    vtkIdTypeArray* outputConnectivity, vtkIdType cellOffset, vtkIdType cellConnectivityOffset,
    vtkIdType pointOffset)
  {
    using ValueType = GetAPIType<OffsetsT>;
    auto inputOffsets = GetRange(offsets);
    auto inputConnectivity = GetRange(conn);
    auto numberOfCells = inputOffsets.size() - 1;

    // Copy the offsets and transform them using the cellConnectivityOffset
    std::transform(inputOffsets.begin(), inputOffsets.begin() + numberOfCells,
      outputOffsets->GetPointer(cellOffset),
      [&](ValueType offset) { return static_cast<ValueType>(offset + cellConnectivityOffset); });
    // Copy the connectivity and transform them using the pointOffset
    std::transform(inputConnectivity.begin(), inputConnectivity.end(),
      outputConnectivity->GetPointer(cellConnectivityOffset),
      [&](ValueType ptId) { return static_cast<vtkIdType>(ptId + pointOffset); });
  }
};

void FillImplicitIndexArray(vtkIdList* indexArray, vtkIdType elementCount, vtkIdType outOffset,
  vtkIdType inOffset, vtkIdType tuplesOffset)
{
  if (elementCount)
  {
    for (int i = 0; i < elementCount; i++)
    {
      indexArray->SetId(outOffset + i, tuplesOffset + inOffset + i);
    }
  }
}
}

//------------------------------------------------------------------------------
vtkAppendPolyData::PolyDataOffsets::PolyDataOffsets(const std::vector<vtkPolyData*>& datasets)
{
  this->PointOffsets.resize(datasets.size());
  this->VertOffsets.resize(datasets.size());
  this->VertConnectivityOffsets.resize(datasets.size());
  this->LineOffsets.resize(datasets.size());
  this->LineConnectivityOffsets.resize(datasets.size());
  this->PolyOffsets.resize(datasets.size());
  this->PolyConnectivityOffsets.resize(datasets.size());
  this->StripOffsets.resize(datasets.size());
  this->StripConnectivityOffsets.resize(datasets.size());

  for (size_t idx = 0; idx < datasets.size(); ++idx)
  {
    auto& dataset = datasets[idx];
    this->PointOffsets[idx] = this->TotalNumberOfPoints;
    this->VertOffsets[idx] = this->TotalNumberOfVerts;
    this->VertConnectivityOffsets[idx] = this->TotalNumberOfVertsConnectivity;
    this->LineOffsets[idx] = this->TotalNumberOfLines;
    this->LineConnectivityOffsets[idx] = this->TotalNumberOfLinesConnectivity;
    this->PolyOffsets[idx] = this->TotalNumberOfPolys;
    this->PolyConnectivityOffsets[idx] = this->TotalNumberOfPolysConnectivity;
    this->StripOffsets[idx] = this->TotalNumberOfStrips;
    this->StripConnectivityOffsets[idx] = this->TotalNumberOfStripsConnectivity;

    this->TotalNumberOfPoints += dataset->GetNumberOfPoints();
    this->TotalNumberOfCells += dataset->GetNumberOfCells();
    this->TotalNumberOfVerts += dataset->GetNumberOfVerts();
    this->TotalNumberOfVertsConnectivity += dataset->GetVerts()->GetNumberOfConnectivityIds();
    this->TotalNumberOfLines += dataset->GetNumberOfLines();
    this->TotalNumberOfLinesConnectivity += dataset->GetLines()->GetNumberOfConnectivityIds();
    this->TotalNumberOfPolys += dataset->GetNumberOfPolys();
    this->TotalNumberOfPolysConnectivity += dataset->GetPolys()->GetNumberOfConnectivityIds();
    this->TotalNumberOfStrips += dataset->GetNumberOfStrips();
    this->TotalNumberOfStripsConnectivity += dataset->GetStrips()->GetNumberOfConnectivityIds();
  }
}

//------------------------------------------------------------------------------
int vtkAppendPolyData::ExecuteAppend(vtkPolyData* output, vtkPolyData* inputs[], int numInputs)
{
  std::vector<vtkPolyData*> polyDataInputs;
  std::vector<vtkDataSet*> datasetInputs;
  for (int i = 0; i < numInputs; ++i)
  {
    if (inputs[i] && inputs[i]->GetNumberOfPoints() > 0)
    {
      polyDataInputs.push_back(inputs[i]);
      datasetInputs.push_back(inputs[i]);
    }
  }

  PolyDataOffsets stats(polyDataInputs);

  if (stats.TotalNumberOfPoints < 1)
  {
    vtkDebugMacro(<< "No data to append!");
    return 1;
  }

  this->AppendPoints(stats, polyDataInputs, output);
  this->UpdateProgress(0.25);
  if (this->CheckAbort())
  {
    return 1;
  }

  // Since points and cells are not merged,
  // this filter can easily pass all field arrays, including global ids.
  auto outputPD = output->GetPointData();
  outputPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  vtkAppendFilter::AppendArrays(
    datasetInputs, vtkDataObject::POINT, output, stats.TotalNumberOfPoints, this->UseImplicitArray);

  this->UpdateProgress(0.50);
  if (this->CheckAbort())
  {
    return 1;
  }

  this->AppendCells(stats, polyDataInputs, output);

  this->UpdateProgress(0.75);
  if (this->CheckAbort())
  {
    return 1;
  }

  this->AppendCellData(stats, polyDataInputs, output, this->UseImplicitArray);

  this->UpdateProgress(1.0);
  return 1;
}

//------------------------------------------------------------------------------
void vtkAppendPolyData::AppendCellData(const PolyDataOffsets& stats,
  const std::vector<vtkPolyData*>& datasets, vtkPolyData* output, bool useImplicit)
{
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

  if (useImplicit)
  {
    std::vector<vtkFieldData*> cellDataList;
    cellDataList.resize(datasets.size());
    for (std::size_t i = 0; i < datasets.size(); ++i)
    {
      cellDataList[i] = datasets[i]->GetCellData();
    }
    vtkNew<vtkIdList> indexArray;
    indexArray->SetNumberOfIds(output->GetNumberOfCells());
    std::vector<vtkIdType> tuplesOffsets(datasets.size(), 0);
    for (std::size_t i = 1; i < tuplesOffsets.size(); i++)
    {
      tuplesOffsets[i] = tuplesOffsets[i - 1] + datasets[i - 1]->GetCellData()->GetNumberOfTuples();
    }
    vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType idx = begin; idx < end; ++idx)
        {
          auto& dataset = datasets[idx];
          const auto outVertOffset = stats.VertOffsets[idx];
          const auto outLineOffset = stats.LineOffsets[idx] + stats.TotalNumberOfVerts;
          const auto outPolyOffset =
            stats.PolyOffsets[idx] + stats.TotalNumberOfLines + stats.TotalNumberOfVerts;
          const auto outStripOffset = stats.StripOffsets[idx] + stats.TotalNumberOfPolys +
            stats.TotalNumberOfLines + stats.TotalNumberOfVerts;

          FillImplicitIndexArray(
            indexArray, dataset->GetNumberOfVerts(), outVertOffset, 0, tuplesOffsets[idx]);

          const vtkIdType inLineOffset = dataset->GetNumberOfVerts();
          FillImplicitIndexArray(indexArray, dataset->GetNumberOfLines(), outLineOffset,
            inLineOffset, tuplesOffsets[idx]);

          const vtkIdType inPolyOffset = inLineOffset + dataset->GetNumberOfLines();
          FillImplicitIndexArray(indexArray, dataset->GetNumberOfPolys(), outPolyOffset,
            inPolyOffset, tuplesOffsets[idx]);

          const vtkIdType inStripOffset = inPolyOffset + dataset->GetNumberOfPolys();
          FillImplicitIndexArray(indexArray, dataset->GetNumberOfStrips(), outStripOffset,
            inStripOffset, tuplesOffsets[idx]);
        }
      });
    cellFieldList.GenerateCompositeArray(cellDataList, indexArray, outputCD);
  }
  else
  {
    outputCD->CopyAllocate(cellFieldList, stats.TotalNumberOfCells);
    outputCD->SetNumberOfTuples(stats.TotalNumberOfCells);
    // copy arrays.
    vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType idx = begin; idx < end; ++idx)
        {
          auto& dataset = datasets[idx];
          const auto outVertOffset = stats.VertOffsets[idx];
          const auto outLineOffset = stats.LineOffsets[idx] + stats.TotalNumberOfVerts;
          const auto outPolyOffset =
            stats.PolyOffsets[idx] + stats.TotalNumberOfLines + stats.TotalNumberOfVerts;
          const auto outStripOffset = stats.StripOffsets[idx] + stats.TotalNumberOfPolys +
            stats.TotalNumberOfLines + stats.TotalNumberOfVerts;
          auto inputCD = dataset->GetCellData();
          if (auto numVerts = dataset->GetNumberOfVerts())
          {
            constexpr vtkIdType inVertOffset = 0;
            cellFieldList.CopyData(idx, inputCD, inVertOffset, numVerts, outputCD, outVertOffset);
          }
          if (auto numLines = dataset->GetNumberOfLines())
          {
            const vtkIdType inLineOffset = dataset->GetNumberOfVerts();
            cellFieldList.CopyData(idx, inputCD, inLineOffset, numLines, outputCD, outLineOffset);
          }
          if (auto numPolys = dataset->GetNumberOfPolys())
          {
            const vtkIdType inPolyOffset =
              dataset->GetNumberOfLines() + dataset->GetNumberOfVerts();
            cellFieldList.CopyData(idx, inputCD, inPolyOffset, numPolys, outputCD, outPolyOffset);
          }
          if (auto numStrips = dataset->GetNumberOfStrips())
          {
            const vtkIdType inStripOffset = dataset->GetNumberOfPolys() +
              dataset->GetNumberOfLines() + dataset->GetNumberOfVerts();
            cellFieldList.CopyData(
              idx, inputCD, inStripOffset, numStrips, outputCD, outStripOffset);
          }
        }
      });
  }
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
void vtkAppendPolyData::AppendPoints(const PolyDataOffsets& stats,
  const std::vector<vtkPolyData*>& polyDataInputs, vtkPolyData* output)
{
  vtkNew<vtkPoints> newPoints;
  // set precision for the points in the output
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    int dataType = 0;

    // Keep track of types for fast point append
    for (const auto& dataset : polyDataInputs)
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

  newPoints->SetNumberOfPoints(stats.TotalNumberOfPoints);
  vtkSMPTools::For(0, static_cast<vtkIdType>(polyDataInputs.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        const auto& dataset = polyDataInputs[idx];
        // copy points
        newPoints->GetData()->InsertTuples(stats.PointOffsets[idx], dataset->GetNumberOfPoints(), 0,
          dataset->GetPoints()->GetData());
      }
    });
  output->SetPoints(newPoints);
}

//------------------------------------------------------------------------------
void vtkAppendPolyData::AppendCells(
  const PolyDataOffsets& stats, const std::vector<vtkPolyData*>& inputs, vtkPolyData* output)
{
  vtkNew<vtkIdTypeArray> vertsOffsetsArray;
  vtkNew<vtkIdTypeArray> vertsConnectivityArray;
  if (stats.TotalNumberOfVerts > 0)
  {
    vertsOffsetsArray->SetNumberOfValues(stats.TotalNumberOfVerts + 1);
    vertsOffsetsArray->SetValue(stats.TotalNumberOfVerts, stats.TotalNumberOfVertsConnectivity);
    vertsConnectivityArray->SetNumberOfValues(stats.TotalNumberOfVertsConnectivity);
  }
  vtkNew<vtkIdTypeArray> linesOffsetsArray;
  vtkNew<vtkIdTypeArray> linesConnectivityArray;
  if (stats.TotalNumberOfLines > 0)
  {
    linesOffsetsArray->SetNumberOfValues(stats.TotalNumberOfLines + 1);
    linesOffsetsArray->SetValue(stats.TotalNumberOfLines, stats.TotalNumberOfLinesConnectivity);
    linesConnectivityArray->SetNumberOfValues(stats.TotalNumberOfLinesConnectivity);
  }
  vtkNew<vtkIdTypeArray> polysOffsetsArray;
  vtkNew<vtkIdTypeArray> polysConnectivityArray;
  if (stats.TotalNumberOfPolys > 0)
  {
    polysOffsetsArray->SetNumberOfValues(stats.TotalNumberOfPolys + 1);
    polysOffsetsArray->SetValue(stats.TotalNumberOfPolys, stats.TotalNumberOfPolysConnectivity);
    polysConnectivityArray->SetNumberOfValues(stats.TotalNumberOfPolysConnectivity);
  }
  vtkNew<vtkIdTypeArray> stripsOffsetsArray;
  vtkNew<vtkIdTypeArray> stripsConnectivityArray;
  if (stats.TotalNumberOfStrips > 0)
  {
    stripsOffsetsArray->SetNumberOfValues(stats.TotalNumberOfStrips + 1);
    stripsOffsetsArray->SetValue(stats.TotalNumberOfStrips, stats.TotalNumberOfStripsConnectivity);
    stripsConnectivityArray->SetNumberOfValues(stats.TotalNumberOfStripsConnectivity);
  }

  vtkSMPTools::For(0, static_cast<vtkIdType>(inputs.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        auto& dataset = inputs[idx];
        auto pointOffset = stats.PointOffsets[idx];
        auto vertOffset = stats.VertOffsets[idx];
        auto vertConnectivityOffset = stats.VertConnectivityOffsets[idx];
        auto lineOffset = stats.LineOffsets[idx];
        auto lineConnectivityOffset = stats.LineConnectivityOffsets[idx];
        auto polyOffset = stats.PolyOffsets[idx];
        auto polyConnectivityOffset = stats.PolyConnectivityOffsets[idx];
        auto stripOffset = stats.StripOffsets[idx];
        auto stripConnectivityOffset = stats.StripConnectivityOffsets[idx];
        if (dataset->GetNumberOfVerts() > 0)
        {
          dataset->GetVerts()->Dispatch(AppendCellArray{}, vertsOffsetsArray,
            vertsConnectivityArray, vertOffset, vertConnectivityOffset, pointOffset);
        }
        if (dataset->GetNumberOfLines() > 0)
        {
          dataset->GetLines()->Dispatch(AppendCellArray{}, linesOffsetsArray,
            linesConnectivityArray, lineOffset, lineConnectivityOffset, pointOffset);
        }
        if (dataset->GetNumberOfPolys() > 0)
        {
          dataset->GetPolys()->Dispatch(AppendCellArray{}, polysOffsetsArray,
            polysConnectivityArray, polyOffset, polyConnectivityOffset, pointOffset);
        }
        if (dataset->GetNumberOfStrips() > 0)
        {
          dataset->GetStrips()->Dispatch(AppendCellArray{}, stripsOffsetsArray,
            stripsConnectivityArray, stripOffset, stripConnectivityOffset, pointOffset);
        }
      }
    });
  if (stats.TotalNumberOfVerts > 0)
  {
    vtkNew<vtkCellArray> verts;
    verts->SetData(vertsOffsetsArray, vertsConnectivityArray);
    output->SetVerts(verts);
  }
  if (stats.TotalNumberOfLines > 0)
  {
    vtkNew<vtkCellArray> lines;
    lines->SetData(linesOffsetsArray, linesConnectivityArray);
    output->SetLines(lines);
  }
  if (stats.TotalNumberOfPolys > 0)
  {
    vtkNew<vtkCellArray> polys;
    polys->SetData(polysOffsetsArray, polysConnectivityArray);
    output->SetPolys(polys);
  }
  if (stats.TotalNumberOfStrips > 0)
  {
    vtkNew<vtkCellArray> strips;
    strips->SetData(stripsOffsetsArray, stripsConnectivityArray);
    output->SetStrips(strips);
  }
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
