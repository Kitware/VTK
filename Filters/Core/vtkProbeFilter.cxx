// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProbeFilter.h"

#include "vtkAbstractCellLocator.h"
#include "vtkAbstractPointLocator.h"
#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFindCellStrategy.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkJumpAndWalkCellLocator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkProbeFilter);
vtkCxxSetObjectMacro(vtkProbeFilter, CellLocator, vtkAbstractCellLocator);
void vtkProbeFilter::SetFindCellStrategy(vtkFindCellStrategy* findCellStrategy)
{
  if (findCellStrategy)
  {
    auto cellLocator = findCellStrategy->ConvertToCellLocator();
    this->SetCellLocator(cellLocator);
  }
}

namespace
{
constexpr double CELL_TOLERANCE_FACTOR_SQR = 1e-6;

constexpr unsigned char CELL_GHOST_MASK =
  vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::DUPLICATECELL;

inline bool IsBlankedCell(vtkUnsignedCharArray* gcells, vtkIdType cellId)
{
  if (gcells)
  {
    const auto flag = gcells->GetTypedComponent(cellId, 0);
    return (flag & CELL_GHOST_MASK) != 0;
  }
  return false;
}
}

//------------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
  this->CategoricalData = 0;
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
  this->MaskPoints = nullptr;
  this->SetNumberOfInputPorts(2);
  this->ValidPointMaskArrayName = nullptr;
  this->SetValidPointMaskArrayName("vtkValidPointMask");

  this->CellLocator = nullptr;

  this->PointList = nullptr;
  this->CellList = nullptr;

  this->PassCellArrays = 0;
  this->PassPointArrays = 0;
  this->PassFieldArrays = 1;
  this->Tolerance = 1.0;
  this->ComputeTolerance = true;
  this->SnapToCellWithClosestPoint = false;
  this->SnappingRadius = std::numeric_limits<double>::infinity();
}

//------------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
  this->ValidPoints->Delete();

  this->SetValidPointMaskArrayName(nullptr);
  this->SetCellLocator(nullptr);

  delete this->PointList;
  delete this->CellList;
}

//------------------------------------------------------------------------------
void vtkProbeFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->CellLocator, "CellLocator");
}

//------------------------------------------------------------------------------
void vtkProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkProbeFilter::SetSourceData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//------------------------------------------------------------------------------
vtkIdTypeArray* vtkProbeFilter::GetValidPoints()
{
  if (this->MaskPoints && this->MaskPoints->GetMTime() > this->ValidPoints->GetMTime())
  {
    char* maskArray = this->MaskPoints->GetPointer(0);
    vtkIdType numPts = this->MaskPoints->GetNumberOfTuples();
    vtkIdType numValidPoints = std::count(maskArray, maskArray + numPts, static_cast<char>(1));
    this->ValidPoints->Initialize();
    this->ValidPoints->ReserveValues(numValidPoints);
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      if (maskArray[i])
      {
        this->ValidPoints->InsertNextValue(i);
      }
    }
    this->ValidPoints->Modified();
  }

  return this->ValidPoints;
}

//------------------------------------------------------------------------------
int vtkProbeFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* source = vtkDataSet::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->CategoricalData == 1)
  {
    // If the categorical data flag is enabled, then a) there must be scalars
    // to treat as categorical data, and b) the scalars must have one component.
    if (!source->GetPointData()->GetScalars())
    {
      vtkErrorMacro(<< "No input scalars!");
      return 1;
    }
    if (source->GetPointData()->GetScalars()->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro(<< "Source scalars have more than one component! Cannot categorize!");
      return 1;
    }

    // Set the scalar to interpolate via nearest neighbor. That way, we won't
    // get any false values (for example, a zone 4 cell appearing on the
    // boundary of zone 3 and zone 5).
    output->GetPointData()->SetCopyAttribute(
      vtkDataSetAttributes::SCALARS, 2, vtkDataSetAttributes::INTERPOLATE);
  }

  if (source)
  {
    this->Probe(input, source, output);
  }

  this->PassAttributeData(input, source, output);
  return 1;
}

//------------------------------------------------------------------------------
void vtkProbeFilter::PassAttributeData(
  vtkDataSet* input, vtkDataObject* vtkNotUsed(source), vtkDataSet* output)
{
  // Please see vtkHyperTreeGridProbeFilter implementation of this method for a condensed version
  // using vtkFieldData::PassData

  // copy point data arrays
  if (this->PassPointArrays)
  {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numPtArrays; ++i)
    {
      vtkDataArray* da = input->GetPointData()->GetArray(i);
      if (!output->GetPointData()->HasArray(da->GetName()))
      {
        output->GetPointData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetPointData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetPointData()->GetAttribute(i))
      {
        output->GetPointData()->SetAttribute(da, i);
      }
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i = 0; i < numCellArrays; ++i)
    {
      vtkDataArray* da = input->GetCellData()->GetArray(i);
      if (!output->GetCellData()->HasArray(da->GetName()))
      {
        output->GetCellData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetCellData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetCellData()->GetAttribute(i))
      {
        output->GetCellData()->SetAttribute(da, i);
      }
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
}

//------------------------------------------------------------------------------
void vtkProbeFilter::BuildFieldList(vtkDataSet* source)
{
  delete this->PointList;
  delete this->CellList;

  this->PointList = new vtkDataSetAttributes::FieldList(1);
  this->PointList->InitializeFieldList(source->GetPointData());

  this->CellList = new vtkDataSetAttributes::FieldList(1);
  this->CellList->InitializeFieldList(source->GetCellData());
}

//------------------------------------------------------------------------------
// * input -- dataset probed with
// * source -- dataset probed into
// * output - output.
void vtkProbeFilter::InitializeForProbing(vtkDataSet* input, vtkDataSet* output)
{
  if (!this->PointList || !this->CellList)
  {
    vtkErrorMacro("BuildFieldList() must be called before calling this method.");
    return;
  }

  vtkIdType numPts = input->GetNumberOfPoints();

  // if this is repeatedly called by the pipeline for a composite mesh,
  // you need a new array for each block
  // (that is you need to reinitialize the object)
  if (this->MaskPoints)
  {
    this->MaskPoints->Delete();
  }
  this->MaskPoints = vtkCharArray::New();
  this->MaskPoints->SetNumberOfComponents(1);
  this->MaskPoints->SetNumberOfTuples(numPts);
  this->MaskPoints->FillValue(0);
  this->MaskPoints->SetName(
    this->ValidPointMaskArrayName ? this->ValidPointMaskArrayName : "vtkValidPointMask");

  // Allocate storage for output PointData
  // All input PD is passed to output as PD. Those arrays in input CD that are
  // not present in output PD will be passed as output PD.
  vtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate(*this->PointList, numPts, numPts);

  vtkCellData* tempCellData = vtkCellData::New();
  // We're okay with copying global ids for cells. we just don't flag them as
  // such.
  tempCellData->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  tempCellData->CopyAllocate(*this->CellList, numPts, numPts);

  this->InputCellArrays.clear();
  int numCellArrays = tempCellData->GetNumberOfArrays();
  for (int cc = 0; cc < numCellArrays; cc++)
  {
    vtkDataArray* inArray = tempCellData->GetArray(cc);
    if (inArray && inArray->GetName() && !outPD->GetArray(inArray->GetName()))
    {
      outPD->AddArray(inArray);
      this->InputCellArrays.push_back(inArray);
    }
  }
  tempCellData->Delete();

  this->InitializeOutputArrays(outPD, numPts);
  outPD->AddArray(this->MaskPoints);
}

//------------------------------------------------------------------------------
void vtkProbeFilter::InitializeSourceArrays(vtkDataSet* source)
{
  if (!this->PointList || !this->CellList)
  {
    vtkErrorMacro("BuildFieldList() must be called before calling this method.");
    return;
  }

  this->SourceCellArrays.clear();
  auto cd = source->GetCellData();
  for (auto& cellArray : this->InputCellArrays)
  {
    vtkDataArray* inArray = cd->GetArray(cellArray->GetName());
    this->SourceCellArrays.push_back(inArray);
  }
}

//------------------------------------------------------------------------------
void vtkProbeFilter::InitializeOutputArrays(vtkPointData* outPD, vtkIdType numPts)
{
  for (int i = 0; i < outPD->GetNumberOfArrays(); ++i)
  {
    vtkDataArray* da = outPD->GetArray(i);
    if (da)
    {
      da->SetNumberOfTuples(numPts);
      da->Fill(0);
    }
  }
}

//------------------------------------------------------------------------------
void vtkProbeFilter::DoProbing(
  vtkDataSet* input, int srcIdx, vtkDataSet* source, vtkDataSet* output)
{
  vtkBoundingBox sbox(source->GetBounds());
  vtkBoundingBox ibox(input->GetBounds());
  if (!sbox.Intersects(ibox))
  {
    return;
  }

  if (auto sourceImage = vtkImageData::SafeDownCast(source))
  {
    this->ProbeImageDataPoints(input, srcIdx, sourceImage, output);
  }
  else if (auto inputImage = vtkImageData::SafeDownCast(input))
  {
    vtkImageData* outputImage = vtkImageData::SafeDownCast(output);
    this->ProbePointsImageData(inputImage, srcIdx, source, outputImage);
  }
  else
  {
    this->ProbeEmptyPoints(input, srcIdx, source, output);
  }
}

//------------------------------------------------------------------------------
void vtkProbeFilter::Probe(vtkDataSet* input, vtkDataSet* source, vtkDataSet* output)
{
  this->BuildFieldList(source);
  this->InitializeForProbing(input, output);
  this->InitializeSourceArrays(source);
  this->DoProbing(input, 0, source, output);
}

//------------------------------------------------------------------------------
class vtkProbeFilter::ProbeEmptyPointsWorklet
{
  vtkProbeFilter* ProbeFilter;
  int SourceIdx;
  vtkDataSet* Input;
  vtkDataSet* Source;
  vtkPointData* SourcePD;
  vtkCellData* SourceCD;
  vtkPointData* OutputPD;
  vtkAbstractCellLocator* CellLocator;
  vtkUnsignedCharArray* SourceGhostFlags;
  vtkCharArray* MaskArray;
  double Tol2;
  int MaxCellSize;

  struct LocalData
  {
    vtkSmartPointer<vtkGenericCell> CurrentCell;
    std::vector<double> Weights;
    double LastPCoords[3];
    int LastSubId;
    double LastClosestPoint[3];
    double LastLength2;
    vtkIdType LastCellId;
  };
  vtkSMPThreadLocal<LocalData> TLData;

public:
  ProbeEmptyPointsWorklet(vtkProbeFilter* probeFilter, int sourceIndex, vtkDataSet* input,
    vtkDataSet* source, vtkPointData* outputPD, vtkAbstractCellLocator* cellLocator,
    vtkUnsignedCharArray* sourceGhostFlags, vtkCharArray* maskArray, double tol2, int maxCellSize)
    : ProbeFilter(probeFilter)
    , SourceIdx(sourceIndex)
    , Input(input)
    , Source(source)
    , SourcePD(source->GetPointData())
    , SourceCD(source->GetCellData())
    , OutputPD(outputPD)
    , CellLocator(cellLocator)
    , SourceGhostFlags(sourceGhostFlags)
    , MaskArray(maskArray)
    , Tol2(tol2)
    , MaxCellSize(maxCellSize)
  {
    if (auto polyData = vtkPolyData::SafeDownCast(source))
    {
      // build cells if needed
      if (polyData->NeedToBuildCells())
      {
        polyData->BuildCells();
      }
    }
  }

  void Initialize()
  {
    auto& tlData = this->TLData.Local();
    tlData.CurrentCell = vtkSmartPointer<vtkGenericCell>::New();
    tlData.Weights.resize(static_cast<size_t>(this->MaxCellSize));
    tlData.LastCellId = -1;
  }

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    // global data
    auto maskArray = this->MaskArray->GetPointer(0);
    // thread local data
    auto& tlData = this->TLData.Local();
    auto& cellLocator = this->CellLocator;
    auto& currentCell = tlData.CurrentCell;
    auto weights = tlData.Weights.data();
    auto& lastPCoords = tlData.LastPCoords;
    auto& lastSubId = tlData.LastSubId;
    auto& lastClosestPoint = tlData.LastClosestPoint;
    auto& lastLength2 = tlData.LastLength2;
    auto& lastCellId = tlData.LastCellId;
    // local data
    double x[3], dist2 = 0;
    vtkIdType newCellId;
    int inside;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPointId - beginPointId) / 10 + 1, (vtkIdType)1000);

    for (vtkIdType pointId = beginPointId; pointId < endPointId; ++pointId)
    {
      if (pointId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->ProbeFilter->CheckAbort();
        }
        if (this->ProbeFilter->GetAbortOutput())
        {
          break;
        }
      }

      if (maskArray[pointId] == static_cast<char>(1))
      {
        // skip points which have already been probed with success.
        // This is helpful for multiblock dataset probing.
        continue;
      }

      // Get the xyz coordinate of the point in the input dataset
      this->Input->GetPoint(pointId, x);

      vtkCell* lastCell = lastCellId != -1 ? currentCell->GetRepresentativeCell() : nullptr;
      if (cellLocator)
      {
        // cellLocators are used for subclasses of vtkPointSet
        newCellId = cellLocator->FindCell(
          x, lastCell, currentCell, lastCellId, this->Tol2, lastSubId, lastPCoords, weights);
      }
      else
      {
        // the classes that do not use a cellLocator are vtkImageData, vtkRectilinearGrid
        newCellId = this->Source->FindCell(
          x, lastCell, currentCell, lastCellId, this->Tol2, lastSubId, lastPCoords, weights);
      }
      if (newCellId != -1)
      {
        // pcoords, weights and subId are all valid, so we can compute the closest point
        // using EvaluateLocation
        currentCell->EvaluateLocation(lastSubId, lastPCoords, lastClosestPoint, weights);
        // also compute distance
        dist2 = vtkMath::Distance2BetweenPoints(x, lastClosestPoint);
        if (newCellId != lastCellId)
        {
          // compute lastLength2
          lastLength2 = currentCell->GetLength2();
          lastCellId = newCellId;
        }
      }
      else
      {
        lastCellId = -1;
        if (this->ProbeFilter->SnapToCellWithClosestPoint && cellLocator)
        {
          // Find the closest point within the snapping radius and the cell that it belong to
          vtkIdType closestPointFound =
            cellLocator->FindClosestPointWithinRadius(x, this->ProbeFilter->SnappingRadius,
              lastClosestPoint, currentCell, lastCellId, lastSubId, dist2, inside);
          if (closestPointFound)
          {
            // Previously computed lastPCoords are not valid, so that we need to compute
            // them and the weights from the lastClosestPoint.
            currentCell->EvaluatePosition(
              lastClosestPoint, nullptr, lastSubId, lastPCoords, dist2, weights);
            // The use of the nullptr avoids the unnecessary recalculation of the closest point
            // and set dist2 to zero, making it to be always accepted for any tolerance.
            // compute lastLength2
            lastLength2 = currentCell->GetLength2();
          }
          else
          {
            lastCellId = -1;
          }
        }
      }

      if (lastCellId >= 0 && !::IsBlankedCell(this->SourceGhostFlags, lastCellId))
      {
        if (this->ProbeFilter->ComputeTolerance)
        {
          // If ComputeTolerance is set, compute a tolerance proportional to the
          // cell length.
          if (dist2 > (lastLength2 * CELL_TOLERANCE_FACTOR_SQR))
          {
            continue;
          }
        }

        // Interpolate the point data
        this->OutputPD->InterpolatePoint(*this->ProbeFilter->PointList, this->SourcePD,
          this->SourceIdx, pointId, currentCell->PointIds, weights);
        for (size_t i = 0, numArrays = this->ProbeFilter->InputCellArrays.size(); i < numArrays;
             ++i)
        {
          auto inputArray = this->ProbeFilter->InputCellArrays[i];
          auto sourceArray = this->ProbeFilter->SourceCellArrays[i];
          if (sourceArray)
          {
            inputArray->SetTuple(pointId, lastCellId, sourceArray);
          }
        }
        maskArray[pointId] = static_cast<char>(1);
      }
    }
  }

  void Reduce() {}
};

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbeEmptyPoints(
  vtkDataSet* input, int srcIdx, vtkDataSet* source, vtkDataSet* output)
{
  double tol2;
  vtkPointData* outPD;

  vtkDebugMacro(<< "Probing data");

  auto sourceGhostFlags = vtkUnsignedCharArray::SafeDownCast(
    source->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));

  // lets use a stack allocated array if possible for performance reasons
  int maxCellSize = source->GetMaxCellSize();

  outPD = output->GetPointData();

  if (this->ComputeTolerance)
  {
    // To compute a reasonable starting tolerance we use a fraction of the largest cell length
    // we come across after sampling 100 cells. Tolerance is meant to be an epsilon for cases,
    // such as probing 2D cells where the XYZ may be a tad off the surface but "close enough".
    double sLength2 = source->GetSampledMaxCellLength2(100);
    // use 0.1% of the diagonal (CELL_TOLERANCE_FACTOR_SQR = 1e-6, since 0.1% has to be squared)
    tol2 = sLength2 * CELL_TOLERANCE_FACTOR_SQR;
  }
  else
  {
    tol2 = (this->Tolerance * this->Tolerance);
  }

  // vtkPointSet based datasets do not have an implicit structure to their
  // points. A locator is needed to accelerate the search for cells, i.e.,
  // perform the FindCell() operation. A vtkAbstractCellLocator can be set.
  // If one is not specified, then vtkJumpAndWalkCellLocator is used to accelerate the search.
  vtkSmartPointer<vtkAbstractCellLocator> cellLocator;
  if (auto ps = vtkPointSet::SafeDownCast(source))
  {
    auto existingCellLocator = ps->GetCellLocator();
    if (this->CellLocator != nullptr)
    {
      // if the existing locator is the same type as the one provided
      if (existingCellLocator && existingCellLocator->IsA(this->CellLocator->GetClassName()))
      {
        // use the existing one because it will be most probably already built
        existingCellLocator->BuildLocator();
        cellLocator = existingCellLocator;
      }
      else
      {
        // create the same instance
        auto datasetCellLocator = vtk::TakeSmartPointer(this->CellLocator->NewInstance());
        datasetCellLocator->SetDataSet(ps);
        // set it as the dataset's locator to allow other filters to reuse the locator.
        ps->SetCellLocator(datasetCellLocator);
        // build it and use it
        datasetCellLocator->BuildLocator();
        cellLocator = datasetCellLocator;
      }
    }
    // if there is already a cell locator, just use it, to avoid building a new one
    else if (existingCellLocator)
    {
      // use the existing one because it will be most probably already built
      existingCellLocator->BuildLocator();
      cellLocator = existingCellLocator;
    }
    else // if no cell locator is specified or exists, use the default cell locator
    {
      vtkNew<vtkJumpAndWalkCellLocator> defaultCellLocator;
      defaultCellLocator->SetDataSet(ps);
      // set it as the dataset's locator to allow other filters to reuse the locator.
      ps->SetCellLocator(defaultCellLocator);
      // build it and use it
      defaultCellLocator->BuildLocator();
      cellLocator = defaultCellLocator;
    }
    if (vtkJumpAndWalkCellLocator::SafeDownCast(cellLocator))
    {
      if (auto polyData = vtkPolyData::SafeDownCast(ps))
      {
        polyData->BuildLinks();
      }
      else if (auto unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(ps))
      {
        unstructuredGrid->BuildLinks();
      }
    }
  }

  ProbeEmptyPointsWorklet worker(this, srcIdx, input, source, outPD, cellLocator, sourceGhostFlags,
    this->MaskPoints, tol2, maxCellSize);
  vtkSMPTools::For(0, input->GetNumberOfPoints(), worker);

  this->MaskPoints->Modified();
}

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbeImagePointsInCell(vtkGenericCell* cell, vtkIdType cellId,
  vtkDataSet* source, int srcBlockId, vtkImageData* input, vtkPointData* outPD, char* maskArray,
  double* wtsBuff)
{
  vtkPointData* pd = source->GetPointData();
  // 1. get coordinates of sampling grids

  // Recover cell bounds
  double cellBounds[6];
  source->GetCellBounds(cellId, cellBounds);
  vtkBoundingBox cellBB(cellBounds);

  // Recover input bounds, we already know they intersect, but reduce cellbounds
  // to input bounds to ensure ComputeStructuredCoordinates works as expected on
  // the edges of the input bounds
  double inBounds[6];
  input->GetBounds(inBounds);
  cellBB.IntersectBox(inBounds);

  // If ComputeTolerance is set, compute a tolerance proportional to the
  // cell length. Otherwise, use the user specified absolute tolerance.
  double tol2;
  if (this->ComputeTolerance)
  {
    const vtkBoundingBox bbox(cellBounds);
    tol2 = CELL_TOLERANCE_FACTOR_SQR * bbox.GetDiagonalLength2();
  }
  else
  {
    tol2 = this->Tolerance * this->Tolerance;
  }

  // Iterate on each point of the bounding box
  // to identify the covered grid
  int* inputExtent = input->GetExtent();
  int idxBounds[6] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
    std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
    std::numeric_limits<int>::max(), std::numeric_limits<int>::min() };
  double corner[3];
  int ijk[3];
  double pCoords[3];
  bool found = false;
  for (int i = 0; i < 8; i++)
  {
    cellBB.GetCorner(i, corner);
    if (input->ComputeStructuredCoordinates(corner, ijk, pCoords, tol2))
    {
      found = true;
      for (int j = 0; j < 3; j++)
      {
        idxBounds[2 * j] = std::min(ijk[j], idxBounds[2 * j]);

        // Force a +1 here to ensure we get even the edge of the input
        // In some case that can be outside of the input image
        int externalExtent = std::min(ijk[j] + 1, inputExtent[2 * j + 1]);
        idxBounds[2 * j + 1] = std::max(externalExtent, idxBounds[2 * j + 1]);
      }
    }
  }

  if (!found)
  {
    return;
  }

  // 2. Recover the cell to process
  source->GetCell(cellId, cell);

  double cpbuf[3];
  double dist2 = 0;
  double* closestPoint = cpbuf;
  const bool is3D = cell->GetCellDimension() == 3;
  if (is3D)
  {
    // we only care about closest point and its distance for 2D cells
    closestPoint = nullptr;
  }

  // 3. Check each indices from the grid
  for (ijk[2] = idxBounds[4]; ijk[2] <= idxBounds[5]; ijk[2]++)
  {
    for (ijk[1] = idxBounds[2]; ijk[1] <= idxBounds[3]; ijk[1]++)
    {
      for (ijk[0] = idxBounds[0]; ijk[0] <= idxBounds[1]; ijk[0]++)
      {
        // skip processed points
        const vtkIdType ptId = input->ComputePointId(ijk);
        if (maskArray[ptId] == 1)
        {
          continue;
        }

        // record point coordinates
        double p[3];
        input->GetPoint(ptId, p);

        double pcoords[3];
        int subId;
        const int inside = cell->EvaluatePosition(p, closestPoint, subId, pcoords, dist2, wtsBuff);

        if (inside == 1 && dist2 <= tol2)
        {
          // Interpolate the point data
          outPD->InterpolatePoint(*this->PointList, pd, srcBlockId, ptId, cell->PointIds, wtsBuff);

          // Assign cell data
          for (size_t i = 0, numArrays = this->InputCellArrays.size(); i < numArrays; ++i)
          {
            auto inputArray = this->InputCellArrays[i];
            auto sourceArray = this->SourceCellArrays[i];
            if (sourceArray)
            {
              inputArray->SetTuple(ptId, cellId, sourceArray);
            }
          }

          maskArray[ptId] = static_cast<char>(1);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
class vtkProbeFilter::ProbeImageDataWorklet
{
public:
  ProbeImageDataWorklet(vtkProbeFilter* probeFilter, vtkDataSet* source, int srcBlockId,
    vtkImageData* input, vtkPointData* outPD, char* maskArray, int maxCellSize)
    : ProbeFilter(probeFilter)
    , Source(source)
    , Input(input)
    , SrcBlockId(srcBlockId)
    , OutPointData(outPD)
    , MaskArray(maskArray)
    , MaxCellSize(maxCellSize)
  {
    // make source API threadsafe by calling it once in a single thread.
    source->GetCellType(0);
    source->GetCell(0, this->TLGenericCell.Local());
  }

  void Initialize() { this->TLWeights.Local().resize(this->MaxCellSize); }

  void operator()(vtkIdType cellBegin, vtkIdType cellEnd)
  {
    double* weights = this->TLWeights.Local().data();

    auto sourceGhostFlags = vtkUnsignedCharArray::SafeDownCast(
      this->Source->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));

    auto& cell = this->TLGenericCell.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((cellEnd - cellBegin) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = cellBegin; cellId < cellEnd; ++cellId)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->ProbeFilter->CheckAbort();
        }
        if (this->ProbeFilter->GetAbortOutput())
        {
          break;
        }
      }
      if (IsBlankedCell(sourceGhostFlags, cellId))
      {
        continue;
      }

      this->ProbeFilter->ProbeImagePointsInCell(cell, cellId, this->Source, this->SrcBlockId,
        this->Input, this->OutPointData, this->MaskArray, weights);
    }
  }

  void Reduce() {}

private:
  vtkProbeFilter* ProbeFilter;
  vtkDataSet* Source;
  vtkImageData* Input;
  int SrcBlockId;
  vtkPointData* OutPointData;
  char* MaskArray;
  int MaxCellSize;

  vtkSMPThreadLocal<std::vector<double>> TLWeights;
  vtkSMPThreadLocalObject<vtkGenericCell> TLGenericCell;
};

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbePointsImageData(
  vtkImageData* input, int srcIdx, vtkDataSet* source, vtkImageData* output)
{
  vtkPointData* outPD = output->GetPointData();
  char* maskArray = this->MaskPoints->GetPointer(0);

  vtkIdType numSrcCells = source->GetNumberOfCells();
  if (numSrcCells > 0)
  {
    ProbeImageDataWorklet worklet(
      this, source, srcIdx, input, outPD, maskArray, source->GetMaxCellSize());
    vtkSMPTools::For(0, numSrcCells, worklet);
  }

  this->MaskPoints->Modified();
}

//------------------------------------------------------------------------------
namespace
{

// Thread local storage
struct ProbeImageDataPointsThreadLocal
{
  bool BaseThread = false;
  vtkSmartPointer<vtkIdList> PointIds;
};

} // anonymous namespace

//------------------------------------------------------------------------------
class vtkProbeFilter::ProbeImageDataPointsWorklet
{
public:
  ProbeImageDataPointsWorklet(vtkProbeFilter* probeFilter, vtkDataSet* input, vtkImageData* source,
    int srcIdx, vtkPointData* outPD, char* maskArray)
    : ProbeFilter(probeFilter)
    , Input(input)
    , Source(source)
    , BlockId(srcIdx)
    , OutPointData(outPD)
    , MaskArray(maskArray)
  {
  }

  void Initialize()
  {
    // BaseThread will be set 'true' for the thread that gets the first piece
    ProbeImageDataPointsThreadLocal& DataPoint = this->Thread.Local();
    DataPoint.BaseThread = false;
    DataPoint.PointIds = vtkSmartPointer<vtkIdList>::New();
    DataPoint.PointIds->SetNumberOfIds(8);
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    if (startId == 0)
    {
      this->Thread.Local().BaseThread = true;
    }
    this->ProbeFilter->ProbeImageDataPointsSMP(this->Input, this->Source, this->BlockId,
      this->OutPointData, this->MaskArray, this->Thread.Local().PointIds.GetPointer(), startId,
      endId, this->Thread.Local().BaseThread);
  }

  void Reduce() {}

private:
  vtkProbeFilter* ProbeFilter;
  vtkDataSet* Input;
  vtkImageData* Source;
  int BlockId;
  vtkPointData* OutPointData;
  char* MaskArray;
  vtkSMPThreadLocal<ProbeImageDataPointsThreadLocal> Thread;
};

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbeImageDataPoints(
  vtkDataSet* input, int srcIdx, vtkImageData* sourceImage, vtkDataSet* output)
{
  vtkPointData* outPD = output->GetPointData();
  char* maskArray = this->MaskPoints->GetPointer(0);

  // Estimate the granularity for multithreading
  int threads = vtkSMPTools::GetEstimatedNumberOfThreads();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType grain = numPts / threads;
  vtkIdType minGrain = 100;
  vtkIdType maxGrain = 1000;
  grain = vtkMath::ClampValue(grain, minGrain, maxGrain);

  // Multithread the execution
  ProbeImageDataPointsWorklet worklet(this, input, sourceImage, srcIdx, outPD, maskArray);
  vtkSMPTools::For(0, numPts, grain, worklet);

  this->MaskPoints->Modified();
}

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbeImageDataPointsSMP(vtkDataSet* input, vtkImageData* source, int srcIdx,
  vtkPointData* outPD, char* maskArray, vtkIdList* pointIds, vtkIdType startId, vtkIdType endId,
  bool baseThread)
{
  vtkPointData* pd = source->GetPointData();
  vtkCellData* cd = source->GetCellData();

  // Get image information
  double spacing[3];
  source->GetSpacing(spacing);
  int extent[6];
  source->GetExtent(extent);

  // Compute the tolerance
  double tol2 = (this->Tolerance * this->Tolerance);
  if (this->ComputeTolerance)
  {
    // Use the diagonal of the cell as the tolerance
    double sLength2 = 0.0;
    for (int i = 0; i < 3; i++)
    {
      if (extent[2 * i] < extent[2 * i + 1])
      {
        sLength2 += spacing[i] * spacing[i];
      }
    }
    tol2 = sLength2 * CELL_TOLERANCE_FACTOR_SQR;
  }

  auto sourceGhostFlags =
    vtkUnsignedCharArray::SafeDownCast(cd->GetArray(vtkDataSetAttributes::GhostArrayName()));

  // Loop over all input points, interpolating source data
  vtkIdType progressInterval = endId / 20 + 1;
  for (vtkIdType ptId = startId; ptId < endId; ptId++)
  {
    if (baseThread && !(ptId % progressInterval))
    {
      // This is not ideal, because if the base thread executes more than one piece,
      // then the progress will repeat its 0.0 to 1.0 progression for each piece.
      this->UpdateProgress(static_cast<double>(ptId) / endId);
      if (this->CheckAbort())
      {
        break;
      }
    }

    if (maskArray[ptId] == static_cast<char>(1))
    {
      // skip points which have already been probed with success.
      // This is helpful for multiblock dataset probing.
      continue;
    }

    // Get the xyz coordinate of the point in the input dataset
    double x[3];
    input->GetPoint(ptId, x);

    // Find the cell and compute interpolation weights
    int subId;
    double pcoords[3], weights[8];
    vtkIdType cellId = source->FindCell(x, nullptr, -1, tol2, subId, pcoords, weights);
    if (cellId >= 0 && !::IsBlankedCell(sourceGhostFlags, cellId))
    {
      source->GetCellPoints(cellId, pointIds);

      // Interpolate the point data
      outPD->InterpolatePoint(*this->PointList, pd, srcIdx, ptId, pointIds, weights);
      for (size_t i = 0, numArrays = this->InputCellArrays.size(); i < numArrays; ++i)
      {
        auto inputArray = this->InputCellArrays[i];
        auto sourceArray = this->SourceCellArrays[i];
        if (sourceArray)
        {
          inputArray->SetTuple(ptId, cellId, sourceArray);
        }
      }
      maskArray[ptId] = static_cast<char>(1);
    }
  }
}

//------------------------------------------------------------------------------
int vtkProbeFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // A variation of the bug fix from John Biddiscombe.
  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (vtkImageData::HasScalarType(sourceInfo))
  {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo), outInfo);
  }
  if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(sourceInfo), outInfo);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkProbeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happened to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output &&
    (!strcmp(output->GetClassName(), "vtkUnstructuredGrid") ||
      !strcmp(output->GetClassName(), "vtkPolyData")))
  {
    usePiece = 1;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  sourceInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  if (!this->SpatialMatch)
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  else if (this->SpatialMatch == 1)
  {
    if (usePiece)
    {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1);
    }
    else
    {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  }

  if (usePiece)
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  }

#if !VTK_USE_FUTURE_BOOL
  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
  {
    vtkErrorMacro("SpatialMatch should be boolean, don't pass other than 0 or 1.");

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
#endif

  return 1;
}

//------------------------------------------------------------------------------
void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject* source = this->GetSource();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << (this->SpatialMatch ? "On" : "Off") << "\n";
  os << indent << "ValidPointMaskArrayName: "
     << (this->ValidPointMaskArrayName ? this->ValidPointMaskArrayName : "vtkValidPointMask")
     << "\n";
  os << indent << "PassFieldArrays: " << (this->PassFieldArrays ? "On" : " Off") << "\n";

  os << indent
     << "CellLocator: " << (this->CellLocator ? this->CellLocator->GetClassName() : "NULL") << "\n";
}
VTK_ABI_NAMESPACE_END
