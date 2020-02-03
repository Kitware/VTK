/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkEvenlySpacedStreamlines2D.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEvenlySpacedStreamlines2D.h"

#include "vtkAMRInterpolatedVelocityField.h"
#include "vtkAbstractInterpolatedVelocityField.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocatorInterpolatedVelocityField.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkInterpolatedVelocityField.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkModifiedBSPTree.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataCollection.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkStreamTracer.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <vector>

vtkObjectFactoryNewMacro(vtkEvenlySpacedStreamlines2D);
vtkCxxSetObjectMacro(vtkEvenlySpacedStreamlines2D, Integrator, vtkInitialValueProblemSolver);
vtkCxxSetObjectMacro(
  vtkEvenlySpacedStreamlines2D, InterpolatorPrototype, vtkAbstractInterpolatedVelocityField);

vtkEvenlySpacedStreamlines2D::vtkEvenlySpacedStreamlines2D()
{
  this->Integrator = vtkRungeKutta2::New();
  for (int i = 0; i < 3; i++)
  {
    this->StartPosition[i] = 0.0;
  }

  this->IntegrationStepUnit = vtkStreamTracer::CELL_LENGTH_UNIT;
  this->InitialIntegrationStep = 0.5;
  this->ClosedLoopMaximumDistance = 1.0e-6;
  this->ClosedLoopMaximumDistanceArcLength = 1.0e-6;
  this->LoopAngle = 0.349066; // 20 degrees in radians
  this->MaximumNumberOfSteps = 2000;
  this->MinimumNumberOfLoopPoints = 4;
  this->DirectionStart = 0;
  // invalid integration direction so that we trigger a change the first time
  this->PreviousDirection = 0;

  this->TerminalSpeed = 1.0E-12;

  this->ComputeVorticity = true;

  this->InterpolatorPrototype = nullptr;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
  this->SeparatingDistance = 1;
  this->SeparatingDistanceArcLength = 1;
  this->SeparatingDistanceRatio = 0.5;
  this->SuperposedGrid = vtkImageData::New();
  this->Streamlines = vtkPolyDataCollection::New();
  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

vtkEvenlySpacedStreamlines2D::~vtkEvenlySpacedStreamlines2D()
{
  this->SetIntegrator(nullptr);
  this->SetInterpolatorPrototype(nullptr);
  this->SuperposedGrid->Delete();
  this->Streamlines->Delete();
}

int vtkEvenlySpacedStreamlines2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->SetupOutput(inInfo, outInfo))
  {
    return 0;
  }
  double bounds[6];
  vtkEvenlySpacedStreamlines2D::GetBounds(this->InputData, bounds);
  if (!vtkMathUtilities::FuzzyCompare(bounds[4], bounds[5]))
  {
    this->InputData->UnRegister(this);
    vtkErrorMacro("vtkEvenlySpacedStreamlines2D does not support planes not aligned with XY.");
    return 0;
  }
  std::array<double, 3> v = { { bounds[1] - bounds[0], bounds[3] - bounds[2],
    bounds[5] - bounds[4] } };
  double length = vtkMath::Norm(&v[0]);

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // compute the separating distance arc length
  double cellLength = 0;
  if (!this->ComputeCellLength(&cellLength))
  {
    this->InputData->UnRegister(this);
    return 0;
  }
  this->SeparatingDistanceArcLength =
    this->ConvertToLength(this->SeparatingDistance, this->IntegrationStepUnit, cellLength);
  this->ClosedLoopMaximumDistanceArcLength =
    this->ConvertToLength(this->ClosedLoopMaximumDistance, this->IntegrationStepUnit, cellLength);
  this->InitializeSuperposedGrid(bounds);
  auto streamTracer = vtkSmartPointer<vtkStreamTracer>::New();
  streamTracer->SetInputDataObject(this->InputData);
  streamTracer->SetMaximumPropagation(length);
  streamTracer->SetMaximumNumberOfSteps(this->MaximumNumberOfSteps);
  streamTracer->SetIntegrationDirection(vtkStreamTracer::BOTH);
  streamTracer->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  streamTracer->SetStartPosition(this->StartPosition);
  streamTracer->SetTerminalSpeed(this->TerminalSpeed);
  streamTracer->SetInitialIntegrationStep(this->InitialIntegrationStep);
  streamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  streamTracer->SetIntegrator(this->Integrator);
  streamTracer->SetComputeVorticity(this->ComputeVorticity);
  streamTracer->SetInterpolatorPrototype(this->InterpolatorPrototype);
  // we end streamlines after one loop iteration
  streamTracer->AddCustomTerminationCallback(&vtkEvenlySpacedStreamlines2D::IsStreamlineLooping,
    this, vtkStreamTracer::FIXED_REASONS_FOR_TERMINATION_COUNT);
  streamTracer->Update();

  auto streamline = vtkSmartPointer<vtkPolyData>::New();
  streamline->ShallowCopy(streamTracer->GetOutput());
  this->AddToAllPoints(streamline);

  auto append = vtkSmartPointer<vtkAppendPolyData>::New();
  append->UserManagedInputsOn();
  append->SetNumberOfInputs(2);
  output->ShallowCopy(streamline);
  int currentSeedId = 1;
  int processedSeedId = 0;

  this->Streamlines->RemoveAllItems();
  this->Streamlines->AddItem(streamline);
  // we also end streamlines when they are close to other streamlines
  streamTracer->AddCustomTerminationCallback(
    &vtkEvenlySpacedStreamlines2D::IsStreamlineTooCloseToOthers, this,
    vtkStreamTracer::FIXED_REASONS_FOR_TERMINATION_COUNT + 1);

  const char* velocityName = this->GetInputArrayToProcessName();
  double deltaOne = this->SeparatingDistanceArcLength / 1000;
  double delta[3] = { deltaOne, deltaOne, deltaOne };
  int maxNumberOfItems = 0;
  float lastProgress = 0.0;
  while (this->Streamlines->GetNumberOfItems())
  {
    int numberOfItems = this->Streamlines->GetNumberOfItems();
    if (numberOfItems > maxNumberOfItems)
    {
      maxNumberOfItems = numberOfItems;
    }
    if (processedSeedId % 10 == 0)
    {
      float progress = (static_cast<float>(maxNumberOfItems) - numberOfItems) / maxNumberOfItems;
      if (progress > lastProgress)
      {
        this->UpdateProgress(progress);
        lastProgress = progress;
      }
    }

    streamline = vtkPolyData::SafeDownCast(this->Streamlines->GetItemAsObject(0));
    vtkDataArray* velocity = streamline->GetPointData()->GetArray(velocityName);
    for (vtkIdType pointId = 0; pointId < streamline->GetNumberOfPoints(); ++pointId)
    {
      // generate 2 new seeds for every streamline point
      double newSeedVector[3];
      double normal[3] = { 0, 0, 1 };
      vtkMath::Cross(normal, velocity->GetTuple(pointId), newSeedVector);
      // floating point errors move newSeedVector out of XY plane.
      newSeedVector[2] = 0;
      vtkMath::Normalize(newSeedVector);
      vtkMath::MultiplyScalar(newSeedVector, this->SeparatingDistanceArcLength);
      double point[3];
      streamline->GetPoint(pointId, point);
      std::array<std::array<double, 3>, 2> newSeeds;
      vtkMath::Add(point, newSeedVector, &newSeeds[0][0]);
      vtkMath::Subtract(point, newSeedVector, &newSeeds[1][0]);

      for (auto newSeed : newSeeds)
      {
        if (vtkMath::PointIsWithinBounds(&newSeed[0], bounds, delta) &&
          !this->ForEachCell(&newSeed[0], &vtkEvenlySpacedStreamlines2D::IsTooClose<DISTANCE>))
        {
          streamTracer->SetStartPosition(&newSeed[0]);
          streamTracer->Update();
          auto newStreamline = vtkSmartPointer<vtkPolyData>::New();
          newStreamline->ShallowCopy(streamTracer->GetOutput());

          vtkIntArray* seedIds =
            vtkIntArray::SafeDownCast(newStreamline->GetCellData()->GetArray("SeedIds"));
          for (int cellId = 0; cellId < newStreamline->GetNumberOfCells(); ++cellId)
          {
            seedIds->SetValue(cellId, currentSeedId);
          }
          currentSeedId++;
          this->AddToAllPoints(newStreamline);
          append->SetInputDataByNumber(0, output);
          append->SetInputDataByNumber(1, newStreamline);
          append->Update();
          output->ShallowCopy(append->GetOutput());
          this->Streamlines->AddItem(newStreamline);
        }
      }
    }
    this->Streamlines->RemoveItem(0);
    ++processedSeedId;
  }
  this->InputData->UnRegister(this);
  return 1;
}

int vtkEvenlySpacedStreamlines2D::ComputeCellLength(double* cellLength)
{
  vtkAbstractInterpolatedVelocityField* func;
  int maxCellSize = 0;
  if (this->CheckInputs(func, &maxCellSize) != VTK_OK)
  {
    if (func)
    {
      func->Delete();
    }
    return 0;
  }
  vtkDataSet* input;
  auto cell = vtkSmartPointer<vtkGenericCell>::New();
  double velocity[3];
  // access the start position
  if (!func->FunctionValues(this->StartPosition, velocity))
  {
    func->Delete();
    return 0;
  }
  // Make sure we use the dataset found by the vtkAbstractInterpolatedVelocityField
  input = func->GetLastDataSet();
  input->GetCell(func->GetLastCellId(), cell);
  *cellLength = sqrt(static_cast<double>(cell->GetLength2()));
  func->Delete();
  return 1;
}

int vtkEvenlySpacedStreamlines2D::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  return 1;
}

bool vtkEvenlySpacedStreamlines2D::IsStreamlineTooCloseToOthers(
  void* clientdata, vtkPoints* points, vtkDataArray* velocity, int direction)
{
  (void)velocity;
  (void)direction;
  vtkEvenlySpacedStreamlines2D* This = static_cast<vtkEvenlySpacedStreamlines2D*>(clientdata);
  vtkIdType count = points->GetNumberOfPoints();
  double point[3];
  points->GetPoint(count - 1, point);
  return This->ForEachCell(point, &vtkEvenlySpacedStreamlines2D::IsTooClose<DISTANCE_RATIO>);
}

bool vtkEvenlySpacedStreamlines2D::IsStreamlineLooping(
  void* clientdata, vtkPoints* points, vtkDataArray* velocity, int direction)
{
  vtkEvenlySpacedStreamlines2D* This = static_cast<vtkEvenlySpacedStreamlines2D*>(clientdata);
  vtkIdType p0 = points->GetNumberOfPoints() - 1;

  // reinitialize when changing direction
  if (direction != This->PreviousDirection)
  {
    This->InitializePoints(This->CurrentPoints);
    This->InitializeMinPointIds();
    This->PreviousDirection = direction;
    This->DirectionStart = p0;
  }

  double p0Point[3];
  points->GetPoint(p0, p0Point);
  int ijk[3] = { 0, 0, 0 };
  ijk[0] = floor(p0Point[0] / This->SeparatingDistanceArcLength);
  ijk[1] = floor(p0Point[1] / This->SeparatingDistanceArcLength);
  vtkIdType cellId = This->SuperposedGrid->ComputeCellId(&ijk[0]);

  bool retVal = This->ForEachCell(
    p0Point, &vtkEvenlySpacedStreamlines2D::IsLooping, points, velocity, direction);

  // add the point to the list
  This->CurrentPoints[cellId].push_back(p0);
  if (p0 < This->MinPointIds[cellId])
  {
    This->MinPointIds[cellId] = p0;
  }
  return retVal;
}

template <typename CellCheckerType>
bool vtkEvenlySpacedStreamlines2D::ForEachCell(
  double* point, CellCheckerType checker, vtkPoints* points, vtkDataArray* velocity, int direction)
{
  // point current cell
  int ijk[3] = { 0, 0, 0 };
  ijk[0] = floor(point[0] / this->SeparatingDistanceArcLength);
  ijk[1] = floor(point[1] / this->SeparatingDistanceArcLength);
  vtkIdType cellId = this->SuperposedGrid->ComputeCellId(&ijk[0]);
  if ((this->*checker)(point, cellId, points, velocity, direction))
  {
    return true;
  }
  // and check cells around the current cell
  std::array<std::array<int, 3>, 8> around = { {
    { { ijk[0] - 1, ijk[1] + 1, ijk[2] } },
    { { ijk[0], ijk[1] + 1, ijk[2] } },
    { { ijk[0] + 1, ijk[1] + 1, ijk[2] } },
    { { ijk[0] - 1, ijk[1], ijk[2] } },
    { { ijk[0] + 1, ijk[1], ijk[2] } },
    { { ijk[0] - 1, ijk[1] - 1, ijk[2] } },
    { { ijk[0], ijk[1] - 1, ijk[2] } },
    { { ijk[0] + 1, ijk[1] - 1, ijk[2] } },
  } };
  int extent[6];
  this->SuperposedGrid->GetExtent(extent);
  for (auto cellPos : around)
  {
    cellId = this->SuperposedGrid->ComputeCellId(&cellPos[0]);
    if (cellPos[0] >= extent[0] && cellPos[0] < extent[1] && cellPos[1] >= extent[2] &&
      cellPos[1] < extent[3] && (this->*checker)(point, cellId, points, velocity, direction))
    {
      return true;
    }
  }
  return false;
}

bool vtkEvenlySpacedStreamlines2D::IsLooping(
  double* point, vtkIdType cellId, vtkPoints* points, vtkDataArray* velocity, int direction)
{
  (void)point;
  // do we have enough points to form a loop
  vtkIdType p0 = points->GetNumberOfPoints() - 1;
  vtkIdType minLoopPoints = std::max(vtkIdType(3), this->MinimumNumberOfLoopPoints);
  if (!this->CurrentPoints[cellId].empty() && p0 - this->MinPointIds[cellId] + 1 >= minLoopPoints)
  {
    vtkIdType p1 = p0 - 1;
    double testDistance2 = this->SeparatingDistanceArcLength * this->SeparatingDistanceArcLength *
      this->SeparatingDistanceRatio * this->SeparatingDistanceRatio;
    double maxDistance2 =
      this->ClosedLoopMaximumDistanceArcLength * this->ClosedLoopMaximumDistanceArcLength;
    for (vtkIdType q : this->CurrentPoints[cellId])
    {
      // do we have enough points to form a loop
      if (p0 - q + 1 < minLoopPoints)
      {
        continue;
      }
      double p0Point[3];
      points->GetPoint(p0, p0Point);
      double qPoint[3];
      points->GetPoint(q, qPoint);
      double distance2 = vtkMath::Distance2BetweenPoints(p0Point, qPoint);
      if (distance2 <= maxDistance2)
      {
        // closed loop
        return true;
      }
      if (distance2 >= testDistance2)
      {
        // we might loop but points are too far.
        continue;
      }
      double p1Point[3];
      points->GetPoint(p1, p1Point);
      double v1[3];
      vtkMath::Subtract(p0Point, p1Point, v1);
      vtkMath::MultiplyScalar(v1, direction);
      double* qVector = velocity->GetTuple(q);
      if (vtkMath::Dot(qVector, v1) < cos(this->LoopAngle))
      {
        // qVector makes a large angle with p0p1
        continue;
      }
      double u0[3], u1[3];
      vtkMath::Subtract(p0Point, qPoint, u0);
      vtkMath::MultiplyScalar(u0, direction);
      vtkMath::Subtract(p1Point, qPoint, u1);
      vtkMath::MultiplyScalar(u1, direction);
      if (vtkMath::Dot(u0, v1) >= 0 && vtkMath::Dot(u1, v1) >= 0)
      {
        // we found a "proponent point" See Liu et al.
        continue;
      }
      // the algorithm in Liu at al. has another test that determines if the
      // loop is closed or spiraling. We don't care about that so we skip it.
      return true;
    }
  }
  return false;
}

template <int distanceType>
bool vtkEvenlySpacedStreamlines2D::IsTooClose(
  double* point, vtkIdType cellId, vtkPoints* points, vtkDataArray* velocity, int direction)
{
  (void)points;
  (void)velocity;
  (void)direction;
  double testDistance2 = this->SeparatingDistanceArcLength * this->SeparatingDistanceArcLength;
  if (distanceType == DISTANCE_RATIO)
  {
    testDistance2 *= (this->SeparatingDistanceRatio * this->SeparatingDistanceRatio);
  }
  for (auto cellPoint : this->AllPoints[cellId])
  {
    double distance2 = vtkMath::Distance2BetweenPoints(point, &cellPoint[0]);
    if (distance2 < testDistance2)
    {
      return true;
    }
  }
  return false;
}

int vtkEvenlySpacedStreamlines2D::GetIntegratorType()
{
  if (!this->Integrator)
  {
    return vtkStreamTracer::NONE;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta2"))
  {
    return vtkStreamTracer::RUNGE_KUTTA2;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta4"))
  {
    return vtkStreamTracer::RUNGE_KUTTA4;
  }
  return vtkStreamTracer::UNKNOWN;
}

void vtkEvenlySpacedStreamlines2D::SetInterpolatorTypeToDataSetPointLocator()
{
  this->SetInterpolatorType(
    static_cast<int>(vtkStreamTracer::INTERPOLATOR_WITH_DATASET_POINT_LOCATOR));
}

void vtkEvenlySpacedStreamlines2D::SetInterpolatorTypeToCellLocator()
{
  this->SetInterpolatorType(static_cast<int>(vtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR));
}

void vtkEvenlySpacedStreamlines2D::SetInterpolatorType(int interpType)
{
  if (interpType == vtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR)
  {
    // create an interpolator equipped with a cell locator
    vtkSmartPointer<vtkCellLocatorInterpolatedVelocityField> cellLoc =
      vtkSmartPointer<vtkCellLocatorInterpolatedVelocityField>::New();

    // specify the type of the cell locator attached to the interpolator
    vtkSmartPointer<vtkModifiedBSPTree> cellLocType = vtkSmartPointer<vtkModifiedBSPTree>::New();
    cellLoc->SetCellLocatorPrototype(cellLocType);

    this->SetInterpolatorPrototype(cellLoc);
  }
  else
  {
    // create an interpolator equipped with a point locator (by default)
    vtkSmartPointer<vtkInterpolatedVelocityField> pntLoc =
      vtkSmartPointer<vtkInterpolatedVelocityField>::New();
    this->SetInterpolatorPrototype(pntLoc);
  }
}

void vtkEvenlySpacedStreamlines2D::SetIntegratorType(int type)
{
  vtkInitialValueProblemSolver* ivp = nullptr;
  switch (type)
  {
    case vtkStreamTracer::RUNGE_KUTTA2:
      ivp = vtkRungeKutta2::New();
      break;
    case vtkStreamTracer::RUNGE_KUTTA4:
      ivp = vtkRungeKutta4::New();
      break;
    default:
      vtkWarningMacro("Unrecognized integrator type. Keeping old one.");
      break;
  }
  if (ivp)
  {
    this->SetIntegrator(ivp);
    ivp->Delete();
  }
}

void vtkEvenlySpacedStreamlines2D::SetIntegrationStepUnit(int unit)
{
  if (unit != vtkStreamTracer::LENGTH_UNIT && unit != vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    unit = vtkStreamTracer::CELL_LENGTH_UNIT;
  }

  if (unit == this->IntegrationStepUnit)
  {
    return;
  }

  this->IntegrationStepUnit = unit;
  this->Modified();
}

double vtkEvenlySpacedStreamlines2D::ConvertToLength(double interval, int unit, double cellLength)
{
  double retVal = 0.0;
  if (unit == vtkStreamTracer::LENGTH_UNIT)
  {
    retVal = interval;
  }
  else if (unit == vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    retVal = interval * cellLength;
  }
  return retVal;
}

int vtkEvenlySpacedStreamlines2D::SetupOutput(vtkInformation* inInfo, vtkInformation* outInfo)
{
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  if (hdInput)
  {
    this->InputData = hdInput;
    hdInput->Register(this);
    return 1;
  }
  else if (dsInput)
  {
    auto mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    mb->SetNumberOfBlocks(numPieces);
    mb->SetBlock(piece, dsInput);
    this->InputData = mb;
    mb->Register(this);
    return 1;
  }
  else
  {
    vtkErrorMacro(
      "This filter cannot handle input of type: " << (input ? input->GetClassName() : "(none)"));
    return 0;
  }
}

int vtkEvenlySpacedStreamlines2D::CheckInputs(
  vtkAbstractInterpolatedVelocityField*& func, int* maxCellSize)
{
  if (!this->InputData)
  {
    return VTK_ERROR;
  }

  vtkOverlappingAMR* amrData = vtkOverlappingAMR::SafeDownCast(this->InputData);

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->InputData->NewIterator());

  vtkDataSet* input0 = nullptr;
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal() && input0 == nullptr)
  {
    input0 = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    iter->GoToNextItem();
  }
  if (!input0)
  {
    return VTK_ERROR;
  }

  int vecType(0);
  vtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);
  if (!vectors)
  {
    return VTK_ERROR;
  }

  // Set the function set to be integrated
  if (!this->InterpolatorPrototype)
  {
    if (amrData)
    {
      func = vtkAMRInterpolatedVelocityField::New();
    }
    else
    {
      func = vtkInterpolatedVelocityField::New();
    }
  }
  else
  {
    if (amrData &&
      vtkAMRInterpolatedVelocityField::SafeDownCast(this->InterpolatorPrototype) == nullptr)
    {
      this->InterpolatorPrototype = vtkAMRInterpolatedVelocityField::New();
    }
    func = this->InterpolatorPrototype->NewInstance();
    func->CopyParameters(this->InterpolatorPrototype);
  }

  if (vtkAMRInterpolatedVelocityField::SafeDownCast(func))
  {
    assert(amrData);
    vtkAMRInterpolatedVelocityField::SafeDownCast(func)->SetAMRData(amrData);
    if (maxCellSize)
    {
      *maxCellSize = 8;
    }
  }
  else if (vtkCompositeInterpolatedVelocityField::SafeDownCast(func))
  {
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataSet* inp = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (inp)
      {
        int cellSize = inp->GetMaxCellSize();
        if (cellSize > *maxCellSize)
        {
          *maxCellSize = cellSize;
        }
        vtkCompositeInterpolatedVelocityField::SafeDownCast(func)->AddDataSet(inp);
      }
      iter->GoToNextItem();
    }
  }
  else
  {
    assert(false);
  }

  const char* vecName = vectors->GetName();
  func->SelectVectors(vecType, vecName);
  return VTK_OK;
}

void vtkEvenlySpacedStreamlines2D::InitializeSuperposedGrid(double* bounds)
{
  this->SuperposedGrid->SetExtent(floor(bounds[0] / this->SeparatingDistanceArcLength),
    ceil(bounds[1] / this->SeparatingDistanceArcLength),
    floor(bounds[2] / this->SeparatingDistanceArcLength),
    ceil(bounds[3] / this->SeparatingDistanceArcLength), 0, 0);
  this->SuperposedGrid->SetSpacing(this->SeparatingDistanceArcLength,
    this->SeparatingDistanceArcLength, this->SeparatingDistanceArcLength);
  this->InitializePoints(this->AllPoints);
  this->InitializePoints(this->CurrentPoints);
}

template <typename T>
void vtkEvenlySpacedStreamlines2D::InitializePoints(T& points)
{
  points.resize(this->SuperposedGrid->GetNumberOfCells());
  for (std::size_t i = 0; i < points.size(); ++i)
  {
    points[i].clear();
  }
}

void vtkEvenlySpacedStreamlines2D::InitializeMinPointIds()
{
  this->MinPointIds.resize(this->SuperposedGrid->GetNumberOfCells());
  for (std::size_t i = 0; i < this->MinPointIds.size(); ++i)
  {
    this->MinPointIds[i] = std::numeric_limits<vtkIdType>::max();
  }
}

void vtkEvenlySpacedStreamlines2D::AddToAllPoints(vtkPolyData* streamline)
{
  vtkPoints* points = streamline->GetPoints();
  if (points)
  {
    for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
    {
      double point[3];
      points->GetPoint(i, point);
      int ijk[3] = { 0, 0, 0 };
      ijk[0] = floor(point[0] / this->SeparatingDistanceArcLength);
      ijk[1] = floor(point[1] / this->SeparatingDistanceArcLength);
      vtkIdType cellId = this->SuperposedGrid->ComputeCellId(ijk);
      this->AllPoints[cellId].push_back({ { point[0], point[1], point[2] } });
    }
  }
}

void vtkEvenlySpacedStreamlines2D::GetBounds(vtkCompositeDataSet* cds, double bounds[6])
{
  if (vtkOverlappingAMR::SafeDownCast(cds))
  {
    vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(cds);
    amr->GetBounds(bounds);
  }
  else
  {
    // initialize bounds
    for (int i : { 0, 2, 4 })
    {
      bounds[i] = std::numeric_limits<double>::max();
    }
    for (int i : { 1, 3, 5 })
    {
      bounds[i] = -std::numeric_limits<double>::max();
    }
    // go over all datasets in the composite data and find min,max
    // for components of all bounds
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataSet* input = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (input)
      {
        double b[6];
        input->GetBounds(b);
        for (int i : { 0, 2, 4 })
        {
          if (b[i] < bounds[i])
          {
            bounds[i] = b[i];
          }
        }
        for (int i : { 1, 3, 5 })
        {
          if (b[i] > bounds[i])
          {
            bounds[i] = b[i];
          }
        }
      }
      iter->GoToNextItem();
    }
  }
}

const char* vtkEvenlySpacedStreamlines2D::GetInputArrayToProcessName()
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->InputData->NewIterator());

  vtkDataSet* input0 = nullptr;
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal() && input0 == nullptr)
  {
    input0 = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    iter->GoToNextItem();
  }
  if (!input0)
  {
    return "";
  }
  int vecType(0);
  vtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);
  if (vectors)
  {
    return vectors->GetName();
  }
  else
  {
    vtkErrorMacro("vtkEvenlySpacedStreamlines2D::SetInputArrayToProcess was not called");
    return nullptr;
  }
}

void vtkEvenlySpacedStreamlines2D::SetIntegratorTypeToRungeKutta2()
{
  this->SetIntegratorType(vtkStreamTracer::RUNGE_KUTTA2);
}

void vtkEvenlySpacedStreamlines2D::SetIntegratorTypeToRungeKutta4()
{
  this->SetIntegratorType(vtkStreamTracer::RUNGE_KUTTA4);
}

void vtkEvenlySpacedStreamlines2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Start position: " << this->StartPosition[0] << " " << this->StartPosition[1]
     << " " << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;

  os << indent << "Integration step unit: "
     << ((this->IntegrationStepUnit == vtkStreamTracer::LENGTH_UNIT) ? "length." : "cell length.")
     << endl;

  os << indent << "Initial integration step: " << this->InitialIntegrationStep << endl;
  os << indent << "Separation distance: " << this->SeparatingDistance << endl;

  os << indent << "Integrator: " << this->Integrator << endl;
  os << indent << "Vorticity computation: " << (this->ComputeVorticity ? " On" : " Off") << endl;
}
