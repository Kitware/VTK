/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeFilter.h"

#include "vtkAbstractCellLocator.h"
#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkCharArray.h"
#include "vtkClosestPointStrategy.h"
#include "vtkFindCellStrategy.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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

vtkStandardNewMacro(vtkProbeFilter);
vtkCxxSetObjectMacro(vtkProbeFilter, CellLocatorPrototype, vtkAbstractCellLocator);
vtkCxxSetObjectMacro(vtkProbeFilter, FindCellStrategy, vtkFindCellStrategy);

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

  this->CellLocatorPrototype = nullptr;
  this->FindCellStrategy = nullptr;

  this->PointList = nullptr;
  this->CellList = nullptr;

  this->PassCellArrays = 0;
  this->PassPointArrays = 0;
  this->PassFieldArrays = 1;
  this->Tolerance = 1.0;
  this->ComputeTolerance = true;
  this->SnapToCellWithClosestPoint = false;
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
  this->SetCellLocatorPrototype(nullptr);
  this->SetFindCellStrategy(nullptr);

  delete this->PointList;
  delete this->CellList;
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
    this->ValidPoints->Allocate(numValidPoints);
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

  this->CellArrays.clear();
  int numCellArrays = tempCellData->GetNumberOfArrays();
  for (int cc = 0; cc < numCellArrays; cc++)
  {
    vtkDataArray* inArray = tempCellData->GetArray(cc);
    if (inArray && inArray->GetName() && !outPD->GetArray(inArray->GetName()))
    {
      outPD->AddArray(inArray);
      this->CellArrays.push_back(inArray);
    }
  }
  tempCellData->Delete();

  this->InitializeOutputArrays(outPD, numPts);
  outPD->AddArray(this->MaskPoints);
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
  vtkFindCellStrategy* Strategy;
  vtkUnsignedCharArray* SourceGhostFlags;
  vtkCharArray* MaskArray;
  double Tol2;
  int MaxCellSize;

  struct LocalData
  {
    vtkSmartPointer<vtkFindCellStrategy> Strategy;
    vtkCellLocatorStrategy* CellLocatorStrategy;
    vtkClosestPointStrategy* ClosestPointStrategy;
    vtkSmartPointer<vtkGenericCell> CurrentCell;
    vtkSmartPointer<vtkGenericCell> LastCell;
    std::vector<double> Weights;
    double LastPCoords[3];
    int LastSubId;
    double LastClosestPoint[3];
    double LastCellBounds[6];
    double LastLength2;
    vtkIdType LastCellId;
  };
  vtkSMPThreadLocal<LocalData> TLData;

public:
  ProbeEmptyPointsWorklet(vtkProbeFilter* probeFilter, int sourceIndex, vtkDataSet* input,
    vtkDataSet* source, vtkPointData* outputPD, vtkFindCellStrategy* strategy,
    vtkUnsignedCharArray* sourceGhostFlags, vtkCharArray* maskArray, double tol2, int maxCellSize)
    : ProbeFilter(probeFilter)
    , SourceIdx(sourceIndex)
    , Input(input)
    , Source(source)
    , SourcePD(source->GetPointData())
    , SourceCD(source->GetCellData())
    , OutputPD(outputPD)
    , Strategy(strategy)
    , SourceGhostFlags(sourceGhostFlags)
    , MaskArray(maskArray)
    , Tol2(tol2)
    , MaxCellSize(maxCellSize)
  {
    // instantiate the cell map for polydata
    vtkNew<vtkGenericCell> cell;
    this->Source->GetCell(0, cell);
  }

  void Initialize()
  {
    auto& tlData = this->TLData.Local();
    if (this->Strategy)
    {
      tlData.Strategy = vtk::TakeSmartPointer(this->Strategy->NewInstance());
      tlData.Strategy->CopyParameters(this->Strategy);
      tlData.Strategy->Initialize(vtkPointSet::SafeDownCast(this->Source));
      tlData.CellLocatorStrategy = vtkCellLocatorStrategy::SafeDownCast(tlData.Strategy);
      tlData.ClosestPointStrategy = vtkClosestPointStrategy::SafeDownCast(tlData.Strategy);
    }
    else
    {
      tlData.Strategy = nullptr;
      tlData.CellLocatorStrategy = nullptr;
      tlData.ClosestPointStrategy = nullptr;
    }
    tlData.CurrentCell = vtkSmartPointer<vtkGenericCell>::New();
    tlData.LastCell = vtkSmartPointer<vtkGenericCell>::New();
    tlData.Weights.resize(static_cast<size_t>(this->MaxCellSize));
    tlData.LastCellId = -1;
  }

  static double GetLength2(const double bounds[6])
  {
    double diff, l = 0.0;
    int i;

    for (i = 0; i < 3; i++)
    {
      diff = bounds[2 * i + 1] - bounds[2 * i];
      l += diff * diff;
    }
    return l;
  }

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    // global data
    auto maskArray = this->MaskArray->GetPointer(0);
    // thread local data
    auto& tlData = this->TLData.Local();
    auto& strategy = tlData.Strategy;
    auto& cellLocatorStrategy = tlData.CellLocatorStrategy;
    auto& closestPointStrategy = tlData.ClosestPointStrategy;
    auto& currentCell = tlData.CurrentCell;
    auto& lastCell = tlData.LastCell;
    auto weights = tlData.Weights.data();
    auto& lastPCoords = tlData.LastPCoords;
    auto& lastSubId = tlData.LastSubId;
    auto& lastClosestPoint = tlData.LastClosestPoint;
    auto& lastCellBounds = tlData.LastCellBounds;
    auto& lastLength2 = tlData.LastLength2;
    auto& lastCellId = tlData.LastCellId;
    // local data
    double x[3], dist2;
    vtkIdType closestPointFound;
    int inside;
    bool foundInCache, insideCellBounds;

    for (vtkIdType pointId = beginPointId; pointId < endPointId; ++pointId)
    {
      if (maskArray[pointId] == static_cast<char>(1))
      {
        // skip points which have already been probed with success.
        // This is helpful for multiblock dataset probing.
        continue;
      }

      // Get the xyz coordinate of the point in the input dataset
      this->Input->GetPoint(pointId, x);

      foundInCache = false;
      if (lastCellId != -1)
      {
        // check if it's inside cell bounds
        insideCellBounds = lastCellBounds[0] <= x[0] && x[0] <= lastCellBounds[1] &&
          lastCellBounds[2] <= x[1] && x[1] <= lastCellBounds[3] && lastCellBounds[4] <= x[2] &&
          x[2] <= lastCellBounds[5];
        if (insideCellBounds)
        {
          // Use cache cell only if point is inside
          inside = currentCell->EvaluatePosition(
            x, lastClosestPoint, lastSubId, lastPCoords, dist2, weights);
          if (inside == 1)
          {
            foundInCache = true;
          }
        }
      }
      if (!foundInCache)
      {
        // strategies are used for subclasses of vtkPointSet
        if (strategy)
        {
          if (cellLocatorStrategy)
          {
            // this location strategy uses a cell locator
            lastCellId = cellLocatorStrategy->FindCell(x, nullptr, currentCell, -1,
              this->Tol2 /*not used*/, lastSubId, lastPCoords, weights);
            // this strategy once it finds a cell where the given point is inside it stops
            // immediately, so currentCell contains the cell we want
          }
          else // vtkClosestPointStrategy
          {
            // this location strategy will first look at the neighbor cells of the cached cell (if
            // any) and if that fails it will use jump and walk technique
            if (lastCellId != -1)
            {
              // Use cache cell only if point is inside
              this->Source->GetCell(lastCellId, lastCell);
              lastCellId = closestPointStrategy->FindCell(
                x, lastCell, currentCell, lastCellId, this->Tol2, lastSubId, lastPCoords, weights);
              foundInCache = lastCellId != -1;
            }
            else
            {
              lastCellId = closestPointStrategy->FindCell(
                x, nullptr, currentCell, -1, this->Tol2, lastSubId, lastPCoords, weights);
            }
            // this strategy once it finds a cell where the given point is inside it stops
            // immediately, so currentCell contains the cell we want
          }
        }
        else
        {
          // the classes that do not use a strategy are vtkImageData, vtkRectilinearGrid
          lastCellId = this->Source->FindCell(
            x, nullptr, currentCell, -1, this->Tol2, lastSubId, lastPCoords, weights);
          // these classes don't use currentCell, so we will need to extract it if we found anything
        }
        if (lastCellId != -1)
        {
          // extract the cell that we found if we didn't use a strategy
          if (!strategy)
          {
            this->Source->GetCell(lastCellId, currentCell);
          }
          // pcoords, weights and subid are all valid, so we can compute the closest point
          // using EvaluateLocation
          currentCell->EvaluateLocation(lastSubId, lastPCoords, lastClosestPoint, weights);
          // copy bounds
          std::copy_n(currentCell->GetBounds(), 6, lastCellBounds);
          // compute lastLength2
          lastLength2 = ProbeEmptyPointsWorklet::GetLength2(lastCellBounds);
        }
        else
        {
          if (this->ProbeFilter->SnapToCellWithClosestPoint && strategy)
          {
            // Find the closest point and the cell that it belong to
            constexpr double snappingRadius = std::numeric_limits<double>::infinity();
            closestPointFound = strategy->FindClosestPointWithinRadius(x, snappingRadius,
              lastClosestPoint, currentCell, lastCellId, lastSubId, dist2, inside);
            if (closestPointFound)
            {
              // pcoords, weights and subid are all valid, so we can compute the closest point
              // using EvaluateLocation
              this->Source->GetCell(lastCellId, currentCell);
              // we don't need to calculate the closest point, but we do need to calculate the
              // weights
              currentCell->EvaluateLocation(lastSubId, lastPCoords, lastClosestPoint, weights);
              // copy bounds
              std::copy_n(currentCell->GetBounds(), 6, lastCellBounds);
              // compute lastLength2
              lastLength2 = ProbeEmptyPointsWorklet::GetLength2(lastCellBounds);
            }
            else
            {
              lastCellId = -1;
            }
          }
        }
      }

      if (lastCellId >= 0 && !::IsBlankedCell(this->SourceGhostFlags, lastCellId))
      {
        if (this->ProbeFilter->ComputeTolerance)
        {
          // If ComputeTolerance is set, compute a tolerance proportional to the
          // cell length.
          dist2 = vtkMath::Distance2BetweenPoints(x, lastClosestPoint);
          if (dist2 > (lastLength2 * CELL_TOLERANCE_FACTOR_SQR))
          {
            continue;
          }
        }

        // Interpolate the point data
        this->OutputPD->InterpolatePoint(*this->ProbeFilter->PointList, this->SourcePD,
          this->SourceIdx, pointId, currentCell->PointIds, weights);
        for (auto& cellArray : this->ProbeFilter->CellArrays)
        {
          if (auto inArray = this->SourceCD->GetArray(cellArray->GetName()))
          {
            this->OutputPD->CopyTuple(inArray, cellArray, lastCellId, pointId);
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
    // to compute a reasonable starting tolerance we use
    // a fraction of the largest cell length we come across
    // out of the first few cells. Tolerance is meant
    // to be an epsilon for cases such as probing 2D
    // cells where the XYZ may be a tad off the surface
    // but "close enough"
    double sLength2 = 0;
    for (vtkIdType i = 0; i < 20 && i < source->GetNumberOfCells(); i++)
    {
      double cLength2 = source->GetCell(i)->GetLength2();
      if (sLength2 < cLength2)
      {
        sLength2 = cLength2;
      }
    }
    // use 1% of the diagonal (1% has to be squared)
    tol2 = sLength2 * CELL_TOLERANCE_FACTOR_SQR;
  }
  else
  {
    tol2 = (this->Tolerance * this->Tolerance);
  }

  // vtkPointSet based datasets do not have an implicit structure to their
  // points. A locator is needed to accelerate the search for cells, i.e.,
  // perform the FindCell() operation. Because of backward legacy there are
  // multiple ways to do this. A vtkFindCellStrategy is preferred, but users
  // can also directly specify a cell locator (via the cell locator
  // prototype). If neither of these is specified, then
  // vtkDataSet::FindCell() is used to accelerate the search.
  vtkFindCellStrategy* strategy = nullptr;
  vtkNew<vtkCellLocatorStrategy> cellLocStrategy;
  vtkNew<vtkClosestPointStrategy> closestPointStrategy;
  if (auto ps = vtkPointSet::SafeDownCast(source))
  {
    if (this->FindCellStrategy != nullptr)
    {
      this->FindCellStrategy->Initialize(ps);
      strategy = this->FindCellStrategy;
    }
    else if (this->CellLocatorPrototype != nullptr)
    {
      // if the existing locator is not the same type, set the locator of dataset instead of the
      // strategy to allow other filters to reuse the locator.
      auto existingLocator = ps->GetCellLocator();
      bool sameLocatorType =
        existingLocator ? this->CellLocatorPrototype->IsA(existingLocator->GetClassName()) : false;
      if (!sameLocatorType)
      {
        auto cellLocator = vtk::TakeSmartPointer(this->CellLocatorPrototype->NewInstance());
        ps->SetCellLocator(cellLocator);
        cellLocator->SetDataSet(ps);
        cellLocator->BuildLocator();
      }
      cellLocStrategy->Initialize(ps);
      strategy = cellLocStrategy;
    }
    else // if no strategy or cell locator is specified, use the default strategy
    {
      closestPointStrategy->Initialize(ps);
      strategy = closestPointStrategy;
    }
    if (vtkClosestPointStrategy::SafeDownCast(strategy))
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

  ProbeEmptyPointsWorklet worker(this, srcIdx, input, source, outPD, strategy, sourceGhostFlags,
    this->MaskPoints, tol2, maxCellSize);
  vtkSMPTools::For(0, input->GetNumberOfPoints(), worker);

  this->MaskPoints->Modified();
}

//------------------------------------------------------------------------------
static void GetPointIdsInRange(double rangeMin, double rangeMax, double start, double stepsize,
  int numSteps, int& minid, int& maxid)
{
  if (stepsize == 0)
  {
    minid = maxid = 0;
    return;
  }

  minid = vtkMath::Ceil((rangeMin - start) / stepsize);
  if (minid < 0)
  {
    minid = 0;
  }

  maxid = vtkMath::Floor((rangeMax - start) / stepsize);
  if (maxid > numSteps - 1)
  {
    maxid = numSteps - 1;
  }
}

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbeImagePointsInCell(vtkCell* cell, vtkIdType cellId, vtkDataSet* source,
  int srcBlockId, const double start[3], const double spacing[3], const int dim[3],
  vtkPointData* outPD, char* maskArray, double* wtsBuff)
{
  vtkPointData* pd = source->GetPointData();
  vtkCellData* cd = source->GetCellData();

  // get coordinates of sampling grids
  double cellBounds[6];
  cell->GetBounds(cellBounds);

  int idxBounds[6];
  GetPointIdsInRange(
    cellBounds[0], cellBounds[1], start[0], spacing[0], dim[0], idxBounds[0], idxBounds[1]);
  GetPointIdsInRange(
    cellBounds[2], cellBounds[3], start[1], spacing[1], dim[1], idxBounds[2], idxBounds[3]);
  GetPointIdsInRange(
    cellBounds[4], cellBounds[5], start[2], spacing[2], dim[2], idxBounds[4], idxBounds[5]);

  if ((idxBounds[1] - idxBounds[0]) < 0 || (idxBounds[3] - idxBounds[2]) < 0 ||
    (idxBounds[5] - idxBounds[4]) < 0)
  {
    return;
  }

  double cpbuf[3];
  double dist2 = 0;
  double* closestPoint = cpbuf;
  if (cell->IsA("vtkCell3D"))
  {
    // we only care about closest point and its distance for 2D cells
    closestPoint = nullptr;
  }

  double userTol2 = this->Tolerance * this->Tolerance;
  for (int iz = idxBounds[4]; iz <= idxBounds[5]; iz++)
  {
    double p[3];
    p[2] = start[2] + iz * spacing[2];
    for (int iy = idxBounds[2]; iy <= idxBounds[3]; iy++)
    {
      p[1] = start[1] + iy * spacing[1];
      for (int ix = idxBounds[0]; ix <= idxBounds[1]; ix++)
      {
        // For each grid point within the cell bound, interpolate values
        p[0] = start[0] + ix * spacing[0];

        double pcoords[3];
        int subId;
        int inside = cell->EvaluatePosition(p, closestPoint, subId, pcoords, dist2, wtsBuff);

        // If ComputeTolerance is set, compute a tolerance proportional to the
        // cell length. Otherwise, use the user specified absolute tolerance.
        double tol2 =
          this->ComputeTolerance ? (CELL_TOLERANCE_FACTOR_SQR * cell->GetLength2()) : userTol2;

        if ((inside == 1) && (dist2 <= tol2))
        {
          vtkIdType ptId = ix + dim[0] * (iy + dim[1] * iz);

          // Interpolate the point data
          outPD->InterpolatePoint(*this->PointList, pd, srcBlockId, ptId, cell->PointIds, wtsBuff);

          // Assign cell data
          for (auto& cellArray : this->CellArrays)
          {
            vtkDataArray* inArray = cd->GetArray(cellArray->GetName());
            if (inArray)
            {
              outPD->CopyTuple(inArray, cellArray, cellId, ptId);
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
    const double start[3], const double spacing[3], const int dim[3], vtkPointData* outPD,
    char* maskArray, int maxCellSize)
    : ProbeFilter(probeFilter)
    , Source(source)
    , SrcBlockId(srcBlockId)
    , Start(start)
    , Spacing(spacing)
    , Dim(dim)
    , OutPointData(outPD)
    , MaskArray(maskArray)
    , MaxCellSize(maxCellSize)
  {
    // make source API threadsafe by calling it once in a single thread.
    source->GetCellType(0);
    source->GetCell(0, this->GenericCell.Local());
  }

  void operator()(vtkIdType cellBegin, vtkIdType cellEnd)
  {
    double fastweights[256];
    double* weights;
    if (this->MaxCellSize <= 256)
    {
      weights = fastweights;
    }
    else
    {
      std::vector<double>& dynamicweights = this->WeightsBuffer.Local();
      dynamicweights.resize(this->MaxCellSize);
      weights = dynamicweights.data();
    }

    auto sourceGhostFlags = vtkUnsignedCharArray::SafeDownCast(
      this->Source->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));

    auto& cell = this->GenericCell.Local();
    for (vtkIdType cellId = cellBegin; cellId < cellEnd; ++cellId)
    {
      if (IsBlankedCell(sourceGhostFlags, cellId))
      {
        continue;
      }

      this->Source->GetCell(cellId, cell);
      this->ProbeFilter->ProbeImagePointsInCell(cell, cellId, this->Source, this->SrcBlockId,
        this->Start, this->Spacing, this->Dim, this->OutPointData, this->MaskArray, weights);
    }
  }

private:
  vtkProbeFilter* ProbeFilter;
  vtkDataSet* Source;
  int SrcBlockId;
  const double* Start;
  const double* Spacing;
  const int* Dim;
  vtkPointData* OutPointData;
  char* MaskArray;
  int MaxCellSize;

  vtkSMPThreadLocal<std::vector<double>> WeightsBuffer;
  vtkSMPThreadLocalObject<vtkGenericCell> GenericCell;
};

//------------------------------------------------------------------------------
void vtkProbeFilter::ProbePointsImageData(
  vtkImageData* input, int srcIdx, vtkDataSet* source, vtkImageData* output)
{
  vtkPointData* outPD = output->GetPointData();
  char* maskArray = this->MaskPoints->GetPointer(0);

  //----------------------------------------
  double spacing[3];
  input->GetSpacing(spacing);
  int extent[6];
  input->GetExtent(extent);
  int dim[3];
  input->GetDimensions(dim);
  double start[3];
  input->GetOrigin(start);
  start[0] += static_cast<double>(extent[0]) * spacing[0];
  start[1] += static_cast<double>(extent[2]) * spacing[1];
  start[2] += static_cast<double>(extent[4]) * spacing[2];

  vtkIdType numSrcCells = source->GetNumberOfCells();

  if (numSrcCells > 0)
  {
    ProbeImageDataWorklet worklet(
      this, source, srcIdx, start, spacing, dim, outPD, maskArray, source->GetMaxCellSize());
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
  for (vtkIdType ptId = startId; ptId < endId && !GetAbortExecute(); ptId++)
  {
    if (baseThread && !(ptId % progressInterval))
    {
      // This is not ideal, because if the base thread executes more than one piece,
      // then the progress will repeat its 0.0 to 1.0 progression for each piece.
      this->UpdateProgress(static_cast<double>(ptId) / endId);
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
      for (auto& cellArray : this->CellArrays)
      {
        vtkDataArray* inArray = cd->GetArray(cellArray->GetName());
        if (inArray)
        {
          outPD->CopyTuple(inArray, cellArray, cellId, ptId);
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

  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
  {
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

  os << indent << "FindCellStrategy: "
     << (this->FindCellStrategy ? this->FindCellStrategy->GetClassName() : "NULL") << "\n";
  os << indent << "CellLocatorPrototype: "
     << (this->CellLocatorPrototype ? this->CellLocatorPrototype->GetClassName() : "NULL") << "\n";
}
