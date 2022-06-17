/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkTemporalInterpolatedVelocityField.h"

#include "vtkAbstractCellLinks.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkClosestPointStrategy.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeInterpolatedVelocityField.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFindCellStrategy.h"
#include "vtkGenericCell.h"
#include "vtkLinearTransformCellLocator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkStaticCellLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkUnstructuredGrid.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalInterpolatedVelocityField);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkTemporalInterpolatedVelocityField, FindCellStrategy, vtkFindCellStrategy);

//------------------------------------------------------------------------------
const double vtkTemporalInterpolatedVelocityField::WEIGHT_TO_TOLERANCE = 1E-3;

//------------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField::vtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs = 3;     // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->IVF[0] = vtkSmartPointer<vtkCompositeInterpolatedVelocityField>::New();
  this->IVF[1] = vtkSmartPointer<vtkCompositeInterpolatedVelocityField>::New();
  this->LastGoodVelocity[0] = 0.0;
  this->LastGoodVelocity[1] = 0.0;
  this->LastGoodVelocity[2] = 0.0;
  this->CurrentWeight = 0.0;
  this->OneMinusWeight = 1.0;
  this->ScaleCoeff = 1.0;
  this->FindCellStrategy = nullptr;

  this->Vals1[0] = this->Vals1[1] = this->Vals1[2] = 0.0;
  this->Vals2[0] = this->Vals2[1] = this->Vals2[2] = 0.0;
  this->Times[0] = this->Times[1] = 0.0;
}

//------------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField::~vtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->SetVectorsSelection(nullptr);
  this->SetFindCellStrategy(nullptr);
  this->IVF[0] = nullptr;
  this->IVF[1] = nullptr;
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::AddDataSetAtTime(int N, double T, vtkDataSet* dataset)
{
  if (N != 0 && N != 1)
  {
    vtkErrorMacro("Invalid time step index " << N);
    return;
  }
  this->Times[N] = T;
  if ((this->Times[1] - this->Times[0]) > 0)
  {
    this->ScaleCoeff = 1.0 / (this->Times[1] - this->Times[0]);
  }
  if (this->MeshOverTime == MeshOverTimeTypes::DIFFERENT)
  {
    this->IVF[N]->AddDataSet(dataset);
  }
  else
  {
    // equality will be true only when we build the first time step
    if (this->MaxCellSizes[N].size() == this->IVF[N]->DataSetsBoundsInfo.size())
    {
      size_t maxCellSize = static_cast<size_t>(dataset->GetMaxCellSize());
      this->MaxCellSizes[N].push_back(maxCellSize);
      this->IVF[N]->AddDataSet(dataset, maxCellSize);
    }
    else
    {
      size_t maxCellSize =
        static_cast<size_t>(this->MaxCellSizes[N][this->IVF[N]->DataSetsBoundsInfo.size()]);
      this->IVF[N]->AddDataSet(dataset, maxCellSize);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetVectorsSelection(const char* v)
{
  this->IVF[0]->SelectVectors(vtkDataObject::POINT, v);
  this->IVF[1]->SelectVectors(vtkDataObject::POINT, v);
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::CreateLocators(const std::vector<vtkDataSet*>& datasets,
  vtkFindCellStrategy* strategy, std::vector<vtkSmartPointer<vtkLocator>>& locators)
{
  locators.clear();
  locators.reserve(datasets.size());
  for (const auto& dataset : datasets)
  {
    if (auto pointSet = vtkPointSet::SafeDownCast(dataset))
    {
      if (vtkCellLocatorStrategy::SafeDownCast(strategy))
      {
        if (!pointSet->GetCellLocator())
        {
          pointSet->BuildCellLocator();
        }
        auto cellLocator = pointSet->GetCellLocator();
        // if cache cell bounds were not on, enable them and compute cell bounds
        if (cellLocator->GetCacheCellBounds() == 0)
        {
          cellLocator->CacheCellBoundsOn();
          cellLocator->ComputeCellBounds();
        }
        cellLocator->SetUseExistingSearchStructure(
          this->MeshOverTime != MeshOverTimeTypes::DIFFERENT);
        locators.emplace_back(cellLocator);
      }
      else // vtkClosestPointStrategy
      {
        if (!pointSet->GetPointLocator())
        {
          pointSet->BuildPointLocator();
        }
        auto pointLocator = pointSet->GetPointLocator();
        pointLocator->SetUseExistingSearchStructure(
          this->MeshOverTime != MeshOverTimeTypes::DIFFERENT);
        locators.emplace_back(pointLocator);
      }
    }
    else
    {
      locators.emplace_back(nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::CreateLinks(const std::vector<vtkDataSet*>& datasets,
  std::vector<vtkSmartPointer<vtkAbstractCellLinks>>& datasetLinks)
{
  datasetLinks.clear();
  datasetLinks.reserve(datasets.size());
  for (const auto& dataset : datasets)
  {
    if (vtkPointSet::SafeDownCast(dataset))
    {
      if (auto ugrid = vtkUnstructuredGrid::SafeDownCast(dataset))
      {
        if (ugrid->GetLinks() == nullptr)
        {
          ugrid->BuildLinks();
        }
        datasetLinks.emplace_back(ugrid->GetLinks());
      }
      else if (auto polyData = vtkPolyData::SafeDownCast(dataset))
      {
        if (polyData->GetLinks() == nullptr)
        {
          // Build links calls BuildCells internally
          polyData->BuildLinks();
        }
        datasetLinks.emplace_back(polyData->GetLinks());
      }
    }
    else
    {
      datasetLinks.emplace_back(nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::CreateLinearTransformCellLocators(
  const std::vector<vtkSmartPointer<vtkLocator>>& locators,
  std::vector<vtkSmartPointer<vtkLocator>>& linearCellLocators)
{
  linearCellLocators.clear();
  linearCellLocators.reserve(locators.size());
  for (const auto& locator : locators)
  {
    if (auto cellLocator = vtkAbstractCellLocator::SafeDownCast(locator))
    {
      auto linearTransformCellLocator = vtkSmartPointer<vtkLinearTransformCellLocator>::New();
      linearTransformCellLocator->SetCellLocator(cellLocator);
      linearCellLocators.emplace_back(linearTransformCellLocator);
    }
    else
    {
      linearCellLocators.emplace_back(nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::InitializeWithLocators(
  vtkCompositeInterpolatedVelocityField* ivf, const std::vector<vtkDataSet*>& datasets,
  vtkFindCellStrategy* strategy, const std::vector<vtkSmartPointer<vtkLocator>>& locators,
  const std::vector<vtkSmartPointer<vtkAbstractCellLinks>>& links)
{
  // Clear the datasets info, subclasses may want to put stuff into it.
  ivf->DataSetsInfo.clear();

  // Proceed to initialize the composite dataset
  ivf->InitializationState = vtkCompositeInterpolatedVelocityField::INITIALIZE_ALL_DATASETS;

  // For each dataset in the list of datasets, make sure a FindCell
  // strategy has been defined and initialized. The potential for composite
  // datasets which may contain instances of (vtkPointSet) make the process
  // more complex. We only care about find cell strategies if the dataset is
  // a vtkPointSet because the other dataset types (e.g., volumes) have their
  // own built-in FindCell() methods.
  vtkDataArray* vectors;
  vtkFindCellStrategy* strategyClone;
  for (size_t i = 0; i < datasets.size(); ++i)
  {
    auto& dataset = datasets[i];
    if (!ivf->VectorsSelection) // if a selection is not specified,
    {
      // use the first one in the point set (this is a behavior for backward compatibility)
      vectors = dataset->GetPointData()->GetVectors(nullptr);
    }
    else
    {
      vectors =
        dataset->GetAttributesAsFieldData(ivf->VectorsType)->GetArray(ivf->VectorsSelection);
    }

    strategyClone = nullptr;
    if (vtkPointSet::SafeDownCast(dataset))
    {
      strategyClone = strategy->NewInstance();
    }
    ivf->AddToDataSetsInfo(dataset, strategyClone, vectors);
  } // for all datasets of composite dataset

  // Now initialize the new strategies
  for (size_t i = 0; i < datasets.size(); ++i)
  {
    auto& datasetInfo = ivf->DataSetsInfo[i];
    if (auto pointSet = vtkPointSet::SafeDownCast(datasetInfo.DataSet))
    {
      if (auto cellLocatorStrategy = vtkCellLocatorStrategy::SafeDownCast(datasetInfo.Strategy))
      {
        cellLocatorStrategy->SetCellLocator(vtkAbstractCellLocator::SafeDownCast(locators[i]));
      }
      else // vtkClosestPointStrategy
      {
        auto pointLocatorStrategy = vtkClosestPointStrategy::SafeDownCast(datasetInfo.Strategy);
        pointLocatorStrategy->SetPointLocator(vtkAbstractPointLocator::SafeDownCast(locators[i]));
      }
      datasetInfo.Strategy->Initialize(pointSet);
    }
  }
  // Now perform initialization on certain data sets
  for (size_t i = 0; i < datasets.size(); ++i)
  {
    auto& datasetInfo = ivf->DataSetsInfo[i];
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
        ugrid->SetLinks(links[i]);
      }
      else if (auto polyData = vtkPolyData::SafeDownCast(datasetInfo.DataSet))
      {
        polyData->SetLinks(vtkCellLinks::SafeDownCast(links[i]));
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::Initialize(
  vtkCompositeDataSet* t0, vtkCompositeDataSet* t1)
{
  vtkSmartPointer<vtkFindCellStrategy> strategy = this->FindCellStrategy;
  if (strategy == nullptr)
  {
    strategy = vtkSmartPointer<vtkCellLocatorStrategy>::New(); // default strategy if not provided
  }

  std::vector<vtkDataSet*> datasets[2];
  datasets[1] = vtkCompositeDataSet::GetDataSets(t1);
  if (t0 == t1) // First time calling this method
  {
    if (vtkClosestPointStrategy::SafeDownCast(strategy))
    {
      this->CreateLinks(datasets[1], this->Links[1]);
    }
    // create one set of locators
    this->CreateLocators(datasets[1], strategy, this->Locators[1]);
    this->InitializeWithLocators(
      this->IVF[0], datasets[1], strategy, this->Locators[1], this->Links[1]);
    this->InitializeWithLocators(
      this->IVF[1], datasets[1], strategy, this->Locators[1], this->Links[1]);
    if (this->MeshOverTime == MeshOverTimeTypes::LINEAR_TRANSFORMATION &&
      vtkCellLocatorStrategy::SafeDownCast(strategy))
    {
      // save initial cell locators
      this->InitialCellLocators = std::move(this->Locators[1]);
      this->CreateLinearTransformCellLocators(this->InitialCellLocators, this->Locators[1]);
    }
  }
  else // t0 != t1
  {
    datasets[0] = vtkCompositeDataSet::GetDataSets(t0);
    switch (this->MeshOverTime)
    {
      case MeshOverTimeTypes::DIFFERENT:
        if (vtkClosestPointStrategy::SafeDownCast(strategy))
        {
          this->Links[0] = std::move(this->Links[1]);
          this->CreateLinks(datasets[1], this->Links[1]);
        }
        this->Locators[0] = std::move(this->Locators[1]);
        this->InitializeWithLocators(
          this->IVF[0], datasets[0], strategy, this->Locators[0], this->Links[0]);
        this->CreateLocators(datasets[1], strategy, this->Locators[1]);
        this->InitializeWithLocators(
          this->IVF[1], datasets[1], strategy, this->Locators[1], this->Links[1]);
        break;
      case MeshOverTimeTypes::STATIC:
        this->InitializeWithLocators(
          this->IVF[0], datasets[0], strategy, this->Locators[1], this->Links[1]);
        this->InitializeWithLocators(
          this->IVF[1], datasets[1], strategy, this->Locators[1], this->Links[1]);
        break;
      case MeshOverTimeTypes::LINEAR_TRANSFORMATION:
        this->Locators[0] = std::move(this->Locators[1]);
        this->InitializeWithLocators(
          this->IVF[0], datasets[0], strategy, this->Locators[0], this->Links[1]);
        if (vtkCellLocatorStrategy::SafeDownCast(strategy))
        {
          // Cell Locators support MeshOverTimeTypes::LINEAR_TRANSFORMATION
          this->CreateLinearTransformCellLocators(this->InitialCellLocators, this->Locators[1]);
        }
        else // vtkClosestPointStrategy
        {
          // PointLocators don't support MeshOverTimeTypes::LINEAR_TRANSFORMATION
          this->CreateLocators(datasets[1], strategy, this->Locators[1]);
        }
        this->InitializeWithLocators(
          this->IVF[1], datasets[1], strategy, this->Locators[1], this->Links[1]);
        break;
      case MeshOverTimeTypes::SAME_TOPOLOGY:
        // point locators can preserve the same links since the topology is the same
        this->Locators[0] = std::move(this->Locators[1]);
        this->InitializeWithLocators(
          this->IVF[0], datasets[0], strategy, this->Locators[0], this->Links[1]);
        this->CreateLocators(datasets[1], strategy, this->Locators[1]);
        this->InitializeWithLocators(
          this->IVF[1], datasets[1], strategy, this->Locators[1], this->Links[1]);
        break;
      default:
        vtkErrorMacro("MeshOverTime type not supported.");
    }
  }
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::CopyParameters(
  vtkTemporalInterpolatedVelocityField* from)
{
  this->MeshOverTime = from->MeshOverTime;
  this->SetFindCellStrategy(from->FindCellStrategy);
  this->IVF[0]->CopyParameters(from->IVF[0]);
  this->IVF[1]->CopyParameters(from->IVF[1]);
  this->Locators[0] = from->Locators[0];
  this->Locators[1] = from->Locators[1];
  this->InitialCellLocators = from->InitialCellLocators;
  this->Links[0] = from->Links[0];
  this->Links[1] = from->Links[1];
  this->MaxCellSizes[0] = from->MaxCellSizes[0];
  this->MaxCellSizes[1] = from->MaxCellSizes[1];
  std::copy_n(from->Times, 2, this->Times);
  this->ScaleCoeff = from->ScaleCoeff;
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::ClearCache()
{
  this->IVF[0]->SetLastCellId(-1, 0);
  this->IVF[1]->SetLastCellId(-1, 0);
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetCachedCellIds(vtkIdType id[2], int ds[2])
{
  if (id[0] != -1)
  {
    this->IVF[0]->SetLastCellId(id[0], ds[0]);
  }
  else
  {
    this->IVF[0]->SetLastCellId(-1, 0);
  }

  if (id[1] != -1)
  {
    this->IVF[1]->SetLastCellId(id[1], ds[1]);
  }
  else
  {
    this->IVF[1]->SetLastCellId(-1, 0);
  }
}

//------------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::GetCachedCellIds(vtkIdType id[2], int ds[2])
{
  id[0] = this->IVF[0]->LastCellId;
  ds[0] = (id[0] == -1) ? 0 : this->IVF[0]->LastDataSetIndex;

  id[1] = this->IVF[1]->LastCellId;
  ds[1] = (id[1] == -1) ? 0 : this->IVF[1]->LastDataSetIndex;
  return ((id[0] >= 0) && (id[1] >= 0));
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::AdvanceOneTimeStep()
{
  this->IVF[0] = vtkSmartPointer<vtkCompositeInterpolatedVelocityField>::New();
  this->IVF[1] = vtkSmartPointer<vtkCompositeInterpolatedVelocityField>::New();
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::ShowCacheResults()
{
  vtkErrorMacro(<< ")\n"
                << "T0 - (cell hit  : " << this->IVF[0]->CacheHit << ")\n"
                << "     (cell miss : " << this->IVF[0]->CacheMiss << ")\n"
                << "  (dataset hit  : " << this->IVF[0]->CacheDataSetHit << ")\n"
                << "  (dataset miss : " << this->IVF[0]->CacheDataSetMiss << ")\n"
                << "T1 - (cell hit  : " << this->IVF[1]->CacheHit << ")\n"
                << "     (cell miss : " << this->IVF[1]->CacheMiss << ")\n"
                << "  (dataset hit  : " << this->IVF[1]->CacheDataSetHit << ")\n"
                << "  (dataset miss : " << this->IVF[1]->CacheDataSetMiss << ")\n");
}

//------------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::TestPoint(double* x)
{
  this->CurrentWeight = (x[3] - this->Times[0]) * this->ScaleCoeff;
  this->OneMinusWeight = 1.0 - this->CurrentWeight;
  if (this->CurrentWeight < (0.0 + vtkTemporalInterpolatedVelocityField::WEIGHT_TO_TOLERANCE))
  {
    this->CurrentWeight = 0.0;
  }
  if (this->CurrentWeight > (1.0 - vtkTemporalInterpolatedVelocityField::WEIGHT_TO_TOLERANCE))
  {
    this->CurrentWeight = 1.0;
  }
  // are we inside dataset at T0
  if (this->IVF[0]->FunctionValues(x, this->Vals1))
  {
    // if we are inside at T0 and static, we must be inside at T1
    if (this->MeshOverTime == MeshOverTimeTypes::STATIC)
    {
      // compute using weights from dataset 0 and vectors from dataset 1
      this->IVF[1]->SetLastCellId(this->IVF[0]->LastCellId, this->IVF[0]->LastDataSetIndex);
      auto datasetInfo = this->IVF[1]->GetDataSetInfo(this->IVF[1]->LastDataSet);
      this->IVF[0]->FastCompute(this->IVF[1], datasetInfo->Vectors, this->Vals2);
      for (int i = 0; i < this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] =
          this->OneMinusWeight * this->Vals1[i] + this->CurrentWeight * this->Vals2[i];
      }
      return IDStates::INSIDE_ALL;
    }
    else
    {
      // dynamic, we need to test at T1
      if (!this->IVF[1]->FunctionValues(x, this->Vals2))
      {
        // inside at T0, but outside at T1, return velocity for T0
        for (int i = 0; i < this->NumFuncs; i++)
        {
          this->LastGoodVelocity[i] = this->Vals1[i];
        }
        return IDStates::OUTSIDE_T1;
      }
      // both valid, compute correct value
      for (int i = 0; i < this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] =
          this->OneMinusWeight * this->Vals1[i] + this->CurrentWeight * this->Vals2[i];
      }
      return IDStates::INSIDE_ALL;
    }
  }
  // Outside at T0, either abort or use T1
  // if we are outside at T0 and static, we must be outside at T1
  if (this->MeshOverTime == MeshOverTimeTypes::STATIC)
  {
    return IDStates::OUTSIDE_ALL;
  }
  else
  {
    // we are dynamic, so test T1
    if (this->IVF[1]->FunctionValues(x, this->Vals2))
    {
      // inside at T1, but outside at T0, return velocity for T1
      for (int i = 0; i < this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] = this->Vals2[i];
      }
      return IDStates::OUTSIDE_T0;
    }
    // failed both, so exit
    return IDStates::OUTSIDE_ALL;
  }
}

//------------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::QuickTestPoint(double* x)
{
  // if outside, return 0
  if (!this->IVF[0]->InsideTest(x))
  {
    return 0;
  }
  // if inside and static dataset hit, skip next test
  if (this->MeshOverTime != MeshOverTimeTypes::STATIC)
  {
    if (!this->IVF[1]->InsideTest(x))
    {
      return 0;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::FunctionValues(double* x, double* u)
{
  if (this->TestPoint(x) == IDStates::OUTSIDE_ALL)
  {
    return 0;
  }
  for (int i = 0; i < this->NumFuncs; i++)
  {
    u[i] = this->LastGoodVelocity[i];
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkTemporalInterpolatedVelocityField::FunctionValuesAtT(int T, double* x, double* u)
{
  // Try velocity at T0
  if (T == 0)
  {
    if (!this->IVF[0]->FunctionValues(x, this->Vals1))
    {
      return 0;
    }
    for (int i = 0; i < this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = u[i] = this->Vals1[i];
    }
    if (this->MeshOverTime == MeshOverTimeTypes::STATIC)
    {
      this->IVF[1]->SetLastCellId(this->IVF[0]->LastCellId, this->IVF[0]->LastDataSetIndex);
    }
  }
  // Try velocity at T1
  else if (T == 1)
  {
    if (!this->IVF[1]->FunctionValues(x, this->Vals2))
    {
      return 0;
    }
    for (int i = 0; i < this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = u[i] = this->Vals2[i];
    }
    if (this->MeshOverTime == MeshOverTimeTypes::STATIC)
    {
      this->IVF[0]->SetLastCellId(this->IVF[1]->LastCellId, this->IVF[1]->LastDataSetIndex);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::InterpolatePoint(
  vtkPointData* outPD1, vtkPointData* outPD2, vtkIdType outIndex)
{
  bool ok1 = this->IVF[0]->InterpolatePoint(outPD1, outIndex);
  bool ok2 = this->IVF[1]->InterpolatePoint(outPD2, outIndex);
  return (ok1 || ok2);
}

//------------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::InterpolatePoint(
  int T, vtkPointData* outPD1, vtkIdType outIndex)
{
  vtkCompositeInterpolatedVelocityField* inivf = this->IVF[T];
  // force use of correct weights/etc if static as only T0 are valid
  if (T == 1 && this->MeshOverTime == MeshOverTimeTypes::STATIC)
  {
    T = 0;
  }
  return this->IVF[T]->InterpolatePoint(inivf, outPD1, outIndex);
}

//------------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::GetVorticityData(
  int T, double pcoords[3], double* weights, vtkGenericCell*& cell, vtkDoubleArray* cellVectors)
{
  // force use of correct weights/etc if static as only T0 are valid
  if (T == 1 && this->MeshOverTime == MeshOverTimeTypes::STATIC)
  {
    T = 0;
  }
  if (this->IVF[T]->GetLastWeights(weights) && this->IVF[T]->GetLastLocalCoordinates(pcoords) &&
    (cell = this->IVF[T]->GetLastCell()))
  {
    vtkDataSet* ds = this->IVF[T]->LastDataSet;
    vtkPointData* pd = ds->GetPointData();
    vtkDataArray* da = pd->GetVectors(this->IVF[T]->GetVectorsSelection());
    da->GetTuples(cell->PointIds, cellVectors);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastGoodVelocity: " << this->LastGoodVelocity[0] << ", "
     << this->LastGoodVelocity[1] << ", " << this->LastGoodVelocity[2] << endl;
  os << indent << "CurrentWeight: " << this->CurrentWeight << endl;
  os << indent << "MeshOverTime: ";
  switch (this->MeshOverTime)
  {
    case MeshOverTimeTypes::DIFFERENT:
      os << "DIFFERENT" << endl;
      break;
    case MeshOverTimeTypes::STATIC:
      os << "STATIC" << endl;
      break;
    case MeshOverTimeTypes::LINEAR_TRANSFORMATION:
      os << "LINEAR_TRANSFORMATION" << endl;
      break;
    case MeshOverTimeTypes::SAME_TOPOLOGY:
      os << "SAME_TOPOLOGY" << endl;
      break;
    default:
      os << "UNKNOWN" << endl;
      break;
  }
  os << indent << "FindCellStrategy: ";
  if (this->FindCellStrategy)
  {
    os << this->FindCellStrategy << endl;
  }
  else
  {
    os << "(none)" << endl;
  }
}
