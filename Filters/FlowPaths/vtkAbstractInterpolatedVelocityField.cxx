// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractInterpolatedVelocityField.h"

#include "vtkAbstractCellLocator.h"
#include "vtkAbstractPointLocator.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkClosestPointStrategy.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkAbstractInterpolatedVelocityField, FindCellStrategy, vtkFindCellStrategy);

//------------------------------------------------------------------------------
const double vtkAbstractInterpolatedVelocityField::TOLERANCE_SCALE = 1.0E-8;
const double vtkAbstractInterpolatedVelocityField::SURFACE_TOLERANCE_SCALE = 1.0E-5;

//------------------------------------------------------------------------------
vtkAbstractInterpolatedVelocityField::vtkAbstractInterpolatedVelocityField()
{
  this->NumFuncs = 3;     // u, v, w
  this->NumIndepVars = 4; // x, y, z, t

  this->Caching = true; // Caching on by default
  this->CacheHit = 0;
  this->CacheMiss = 0;

  this->LastCellId = -1;
  this->LastDataSet = nullptr;
  this->LastPCoords[0] = 0.0;
  this->LastPCoords[1] = 0.0;
  this->LastPCoords[2] = 0.0;

  this->LastClosestPoint[0] = 0.0;
  this->LastClosestPoint[1] = 0.0;
  this->LastClosestPoint[2] = 0.0;

  this->VectorsType = vtkDataObject::POINT;
  this->VectorsSelection = nullptr;
  this->NormalizeVector = false;
  this->ForceSurfaceTangentVector = false;
  this->SurfaceDataset = false;

  this->InitializationState = NOT_INITIALIZED;
  this->FindCellStrategy = nullptr;
}

//------------------------------------------------------------------------------
vtkAbstractInterpolatedVelocityField::~vtkAbstractInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;

  this->LastDataSet = nullptr;
  this->SetVectorsSelection(nullptr);

  // Need to free strategies and other information associated with each
  // dataset. There is a special case where the strategy cannot be deleted
  // because is has been specified by the user.
  for (auto& datasetInfo : this->DataSetsInfo)
  {
    if (datasetInfo.Strategy != nullptr)
    {
      datasetInfo.Strategy->Delete();
    }
  }
  this->DataSetsInfo.clear();

  this->SetFindCellStrategy(nullptr);
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::Initialize(vtkCompositeDataSet* compDS, int initStrategy)
{
  // Clear the datasets info, subclasses may want to put stuff into it.
  this->DataSetsInfo.clear();

  // See whether the subclass should take over the initialization process.
  if (this->SelfInitialize())
  {
    return;
  }

  // Proceed to initialize the composite dataset
  this->InitializationState = initStrategy;

  // Obtain this find cell strategy or create the default one as necessary
  vtkSmartPointer<vtkFindCellStrategy> strategy = this->FindCellStrategy;
  vtkFindCellStrategy* strategyClone;
  if (strategy == nullptr)
  {
    strategy = vtkSmartPointer<vtkClosestPointStrategy>::New(); // default strategy if not provided
  }

  // These are the datasets to process from the input to the filter.
  auto datasets = vtkCompositeDataSet::GetDataSets(compDS);

  // For each dataset in the list of datasets, make sure a FindCell
  // strategy has been defined and initialized. The potential for composite
  // datasets which may contain instances of (vtkPointSet) make the process
  // more complex. We only care about find cell strategies if the dataset is
  // a vtkPointSet because the other dataset types (e.g., volumes) have their
  // own built-in FindCell() methods.
  vtkDataArray* vectors;
  for (auto& dataset : datasets)
  {
    if (!this->VectorsSelection) // if a selection is not specified,
    {
      // use the first one in the point set (this is a behavior for backward compatibility)
      vectors = dataset->GetPointData()->GetVectors(nullptr);
    }
    else
    {
      vectors =
        dataset->GetAttributesAsFieldData(this->VectorsType)->GetArray(this->VectorsSelection);
    }

    strategyClone = nullptr;
    if (vtkPointSet::SafeDownCast(dataset))
    {
      strategyClone = strategy->NewInstance();
    }
    this->AddToDataSetsInfo(dataset, strategyClone, vectors);
  } // for all datasets of composite dataset

  // Now initialize the new strategies
  for (auto& datasetInfo : this->DataSetsInfo)
  {
    if (auto pointSet = vtkPointSet::SafeDownCast(datasetInfo.DataSet))
    {
      if (auto closestPointStrategy = vtkClosestPointStrategy::SafeDownCast(datasetInfo.Strategy))
      {
        auto providedClosestPointStrategy = vtkClosestPointStrategy::SafeDownCast(strategy);
        // if locator is set, create a new instance of it and set it on the strategy
        if (auto pointLocator = providedClosestPointStrategy->GetPointLocator())
        {
          closestPointStrategy->SetPointLocator(vtk::TakeSmartPointer(pointLocator->NewInstance()));
        }
      }
      else if (auto cellLocatorStrategy =
                 vtkCellLocatorStrategy::SafeDownCast(datasetInfo.Strategy))
      {
        auto providedCellLocatorStrategy = vtkCellLocatorStrategy::SafeDownCast(strategy);
        // if locator is set, create a new instance of it and set it on the strategy
        if (auto cellLocator = providedCellLocatorStrategy->GetCellLocator())
        {
          cellLocatorStrategy->SetCellLocator(vtk::TakeSmartPointer(cellLocator->NewInstance()));
        }
      }
      datasetInfo.Strategy->Initialize(pointSet);
    }
  }

  // Now perform initialization on certain data sets
  for (auto& datasetInfo : this->DataSetsInfo)
  {
    datasetInfo.DataSet->ComputeBounds();
    if (auto polyData = vtkPolyData::SafeDownCast(datasetInfo.DataSet))
    {
      // build cells is needed for both vtkClosestPointStrategy and vtkCellLocatorStrategy
      if (polyData->NeedToBuildCells())
      {
        polyData->BuildCells();
      }
    }
    if (vtkClosestPointStrategy::SafeDownCast(datasetInfo.Strategy))
    {
      if (auto ugrid = vtkUnstructuredGrid::SafeDownCast(datasetInfo.DataSet))
      {
        if (ugrid->GetLinks() == nullptr)
        {
          ugrid->BuildLinks();
        }
      }
      else if (auto polyData = vtkPolyData::SafeDownCast(datasetInfo.DataSet))
      {
        if (polyData->GetLinks() == nullptr)
        {
          polyData->BuildLinks();
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::FunctionValues(vtkDataSet* dataset, double* x, double* f)
{
  // Make sure the velocity field has been initialized. If not initialized,
  // then self initialization is invoked which may not be thead safe, and
  // data races may result. Meant to support backward legacy in serial
  // execution.
  if (this->InitializationState == NOT_INITIALIZED)
  {
    vtkWarningMacro(<< "Velocity field not initialized for threading!");
    this->SelfInitialize();
  }

  // See if a dataset has been specified and if there are input vectors
  if (!dataset)
  {
    vtkErrorMacro(<< "Can't evaluate dataset!");
    return 0;
  }

  // Retrieve cached function array
  vtkDataArray* vectors = nullptr;
  auto datasetInfoIter = this->GetDataSetInfo(dataset);
  if (datasetInfoIter != this->DataSetsInfo.end())
  {
    vectors = datasetInfoIter->Vectors;
  }

  if (!vectors)
  {
    vtkErrorMacro(<< "No vectors for dataset!");
    return 0;
  }

  // Compute function values for the dataset
  f[0] = f[1] = f[2] = 0.0;

  if (!this->FindAndUpdateCell(dataset, datasetInfoIter->Strategy, x))
  {
    vectors = nullptr;
    return 0;
  }

  // if the cell is valid
  if (this->LastCellId >= 0)
  {
    // interpolate the vectors
    if (this->VectorsType == vtkDataObject::POINT)
    {
      double vec[3];
      for (vtkIdType j = 0, numPts = this->CurrentCell->GetNumberOfPoints(); j < numPts; j++)
      {
        vectors->GetTuple(this->CurrentCell->PointIds->GetId(j), vec);
        for (vtkIdType i = 0; i < 3; i++)
        {
          f[i] += vec[i] * this->Weights[j];
        }
      }
    }
    else
    {
      vectors->GetTuple(this->LastCellId, f);
    }

    if (this->ForceSurfaceTangentVector)
    {
      dataset->GetCellPoints(this->LastCellId, this->PointIds);
      if (this->PointIds->GetNumberOfIds() < 3)
      {
        vtkErrorMacro(<< "Cannot compute normal on cells with less than 3 points");
      }
      else
      {
        double p1[3];
        double p2[3];
        double p3[3];
        dataset->GetPoint(this->PointIds->GetId(0), p1);
        dataset->GetPoint(this->PointIds->GetId(1), p2);
        dataset->GetPoint(this->PointIds->GetId(2), p3);

        // Compute orthogonal component
        const double v1[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
        const double v2[3] = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };

        double normal[3];
        vtkMath::Cross(v1, v2, normal);
        vtkMath::Normalize(normal);
        const double k = vtkMath::Dot(normal, f);

        // Remove non-orthogonal component.
        f[0] -= (normal[0] * k);
        f[1] -= (normal[1] * k);
        f[2] -= (normal[2] * k);
      }
    }

    if (this->NormalizeVector)
    {
      vtkMath::Normalize(f);
    }
  }
  // if not, return false
  else
  {
    vectors = nullptr;
    return 0;
  }

  vectors = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::FindAndUpdateCell(
  vtkDataSet* dataset, vtkFindCellStrategy* strategy, double* x)
{
  const double diagonalLength2 = dataset->GetLength2();
  const double tol2 = diagonalLength2 *
    (this->SurfaceDataset ? vtkAbstractInterpolatedVelocityField::SURFACE_TOLERANCE_SCALE
                          : vtkAbstractInterpolatedVelocityField::TOLERANCE_SCALE);
  const double tol = std::sqrt(tol2);

  double dist2;
  int inside;
  vtkIdType closestPointFound;
  bool foundInCache = false;
  // See if the point is in the cached cell
  if (this->Caching && this->LastCellId != -1)
  {
    // Use cache cell only if point is inside
    int ret = this->CurrentCell->EvaluatePosition(
      x, this->LastClosestPoint, this->LastSubId, this->LastPCoords, dist2, this->Weights.data());
    // this->LastClosestPoint has been computed

    // check if point is inside the cell
    if (ret == 1)
    {
      this->CacheHit++;
      foundInCache = true;
    }
  }
  if (!foundInCache)
  {
    if (strategy)
    {
      // strategies are used for subclasses of vtkPointSet
      if (vtkCellLocatorStrategy::SafeDownCast(strategy))
      {
        // this location strategy uses a vtkStaticCellLocator which is a 3D grid with bins
        // and each bin has the cellIds that are inside this bin (robust but possibly slower)
        this->LastCellId = strategy->FindCell(x, nullptr, this->CurrentCell, -1, tol2 /*not used*/,
          this->LastSubId, this->LastPCoords, this->Weights.data());
        // this strategy once it finds a cell where the given point is inside it stops
        // immediately, so this->CurrentCell contains the cell we want
      }
      else // vtkClosestPointStrategy
      {
        // this location strategy will first look at the neighbor cells of the cached cell (if any)
        // and if that fails it will use jump and walk technique (not robust but possibly faster)
        if (this->Caching && this->LastCellId != -1)
        {
          // closest-point cell location can benefit from the initial cached cell, so we extract it
          dataset->GetCell(this->LastCellId, this->LastCell);
          this->LastCellId = strategy->FindCell(x, this->LastCell, this->CurrentCell,
            this->LastCellId, tol2, this->LastSubId, this->LastPCoords, this->Weights.data());
          foundInCache = this->LastCellId != -1;
        }
        else
        {
          this->LastCellId = strategy->FindCell(x, nullptr, this->CurrentCell, -1, tol2,
            this->LastSubId, this->LastPCoords, this->Weights.data());
        }
        // this strategy once it finds a cell where the given point is inside it stops
        // immediately, so this->CurrentCell contains the cell we want
      }
    }
    else
    {
      // the classes that do not use a strategy are vtkUniformGrid, vtkImageData, vtkRectilinearGrid
      this->LastCellId = dataset->FindCell(
        x, nullptr, nullptr, -1, tol2, this->LastSubId, this->LastPCoords, this->Weights.data());
      // these classes don't use CurrentCell, so we will need to extract it if we found something
    }
    // if we found a cell
    if (this->LastCellId != -1)
    {
      if (foundInCache)
      {
        this->CacheHit++;
      }
      else
      {
        this->CacheMiss++;
      }
      // extract the cell that we found if we didn't use a strategy
      if (!strategy)
      {
        dataset->GetCell(this->LastCellId, this->CurrentCell);
      }
      // pcoords, weights and subid are all valid, so we can compute the closest point
      // using EvaluateLocation
      this->CurrentCell->EvaluateLocation(
        this->LastSubId, this->LastPCoords, this->LastClosestPoint, this->Weights.data());
    }
    else
    {
      this->CacheMiss++;
      if (this->SurfaceDataset)
      {
        // if we are on a surface dataset, we can use the strategy to find the closest point
        closestPointFound = strategy->FindClosestPointWithinRadius(x, tol, this->LastClosestPoint,
          this->CurrentCell, this->LastCellId, this->LastSubId, dist2, inside);
        // FindClosestPointWithinRadius does not return the correct CurrentCell, so in case we find
        // something we need to extract it and calculate the weights
        if (closestPointFound == 1)
        {
          dataset->GetCell(this->LastCellId, this->CurrentCell);
          // we don't need to calculate the closest point, but we do need to calculate the weights
          this->CurrentCell->EvaluatePosition(x, nullptr /*closestPoint*/, this->LastSubId,
            this->LastPCoords, dist2, this->Weights.data());
        }
        else
        {
          this->LastCellId = -1;
          return false;
        }
      }
      else
      {
        this->LastCellId = -1;
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::GetLastWeights(double* w)
{
  if (this->LastCellId < 0)
  {
    return 0;
  }
  std::copy_n(this->Weights.data(), this->CurrentCell->GetNumberOfPoints(), w);

  return 1;
}

//------------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::GetLastLocalCoordinates(double pcoords[3])
{
  if (this->LastCellId < 0)
  {
    return 0;
  }

  pcoords[0] = this->LastPCoords[0];
  pcoords[1] = this->LastPCoords[1];
  pcoords[2] = this->LastPCoords[2];

  return 1;
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::FastCompute(vtkDataArray* vectors, double f[3])
{
  this->FastCompute(this, vectors, f);
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::FastCompute(
  vtkAbstractInterpolatedVelocityField* inIVF, vtkDataArray* vectors, double f[3])
{
  int pntIdx;
  int numPts = inIVF->CurrentCell->GetNumberOfPoints();
  double vector[3];
  f[0] = f[1] = f[2] = 0.0;

  for (int i = 0; i < numPts; i++)
  {
    pntIdx = inIVF->CurrentCell->PointIds->GetId(i);
    vectors->GetTuple(pntIdx, vector);
    f[0] += vector[0] * this->Weights[i];
    f[1] += vector[1] * this->Weights[i];
    f[2] += vector[2] * this->Weights[i];
  }
}

//------------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::InterpolatePoint(vtkPointData* outPD, vtkIdType outIndex)
{
  return this->InterpolatePoint(this, outPD, outIndex);
}

//------------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::InterpolatePoint(
  vtkAbstractInterpolatedVelocityField* inIVF, vtkPointData* outPD, vtkIdType outIndex)
{
  if (!this->LastDataSet)
  {
    return false;
  }
  vtkPointData* inPD = inIVF->LastDataSet->GetPointData();
  outPD->InterpolatePoint(inPD, outIndex, this->CurrentCell->PointIds, this->Weights.data());
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::CopyParameters(
  vtkAbstractInterpolatedVelocityField* from)
{
  this->Caching = from->Caching;
  this->SetFindCellStrategy(from->GetFindCellStrategy());
  this->NormalizeVector = from->NormalizeVector;
  this->ForceSurfaceTangentVector = from->ForceSurfaceTangentVector;
  this->SurfaceDataset = from->SurfaceDataset;
  this->VectorsType = from->VectorsType;
  this->SetVectorsSelection(from->VectorsSelection);

  // Copy the datasets' info, including possibly strategies, from the
  // prototype. In a threaded situation, there must be separate strategies
  // for each interpolated velocity field.
  this->InitializationState = from->InitializationState;
  this->DataSetsInfo.clear();
  for (const auto& datasetInfo : from->DataSetsInfo)
  {
    vtkFindCellStrategy* strategy = nullptr;
    if (datasetInfo.Strategy != nullptr)
    {
      strategy = datasetInfo.Strategy->NewInstance();
      strategy->CopyParameters(datasetInfo.Strategy);
      strategy->Initialize(vtkPointSet::SafeDownCast(datasetInfo.DataSet));
    }
    this->AddToDataSetsInfo(datasetInfo.DataSet, strategy, datasetInfo.Vectors);
  }
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::AddToDataSetsInfo(
  vtkDataSet* ds, vtkFindCellStrategy* s, vtkDataArray* vectors)
{
  this->DataSetsInfo.emplace_back(ds, s, vectors);
}

std::vector<vtkAbstractInterpolatedVelocityField::vtkDataSetInformation>::iterator
vtkAbstractInterpolatedVelocityField::GetDataSetInfo(vtkDataSet* dataset)
{
  return std::find_if(this->DataSetsInfo.begin(), this->DataSetsInfo.end(),
    [dataset](const vtkDataSetInformation& datasetInfo) { return datasetInfo.DataSet == dataset; });
}

//------------------------------------------------------------------------------
size_t vtkAbstractInterpolatedVelocityField::GetDataSetsInfoSize()
{
  return this->DataSetsInfo.size();
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::SelectVectors(int associationType, const char* fieldName)
{
  this->VectorsType = associationType;
  this->SetVectorsSelection(fieldName);
}

//------------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "VectorsSelection: " << (this->VectorsSelection ? this->VectorsSelection : "(none)")
     << endl;
  os << indent << "NormalizeVector: " << (this->NormalizeVector ? "on." : "off.") << endl;
  os << indent
     << "ForceSurfaceTangentVector: " << (this->ForceSurfaceTangentVector ? "on." : "off.") << endl;
  os << indent << "SurfaceDataset: " << (this->SurfaceDataset ? "on." : "off.") << endl;

  os << indent << "Caching Status: " << (this->Caching ? "on." : "off.") << endl;
  os << indent << "Cache Hit: " << this->CacheHit << endl;
  os << indent << "Cache Miss: " << this->CacheMiss << endl;
  os << indent << "Last Dataset: " << this->LastDataSet << endl;
  os << indent << "Last Cell Id: " << this->LastCellId << endl;
  os << indent << "Last Cell: " << endl;
  this->LastCell->PrintSelf(os, indent);
  os << indent << "Current Cell: " << endl;
  this->CurrentCell->PrintSelf(os, indent);
  os << indent << "Last P-Coords: " << this->LastPCoords[0] << ", " << this->LastPCoords[1] << ", "
     << this->LastPCoords[2] << endl;
  os << indent << "Last ClosestPoint: " << this->LastClosestPoint[0] << ", "
     << this->LastClosestPoint[1] << ", " << this->LastClosestPoint[2] << endl;
  os << indent << "Last Weights: " << endl;
  for (size_t i = 0; i < this->Weights.size(); ++i)
  {
    os << indent << this->Weights[i] << ", ";
  }
  os << endl;
  os << indent << "FindCell Strategy: " << endl;
  this->FindCellStrategy->PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
