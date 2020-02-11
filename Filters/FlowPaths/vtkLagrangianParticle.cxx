/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianParticle.h"

#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkSetGet.h"

//---------------------------------------------------------------------------
vtkLagrangianParticle::vtkLagrangianParticle(int numberOfVariables, vtkIdType seedId,
  vtkIdType particleId, vtkIdType seedArrayTupleIndex, double integrationTime,
  vtkPointData* seedData, int weightsSize, int numberOfTrackedUserData)
  : Id(particleId)
  , ParentId(-1)
  , SeedId(seedId)
  , NumberOfSteps(0)
  , SeedArrayTupleIndex(seedArrayTupleIndex)
  , SeedData(seedData)
  , StepTime(0)
  , IntegrationTime(integrationTime)
  , PrevIntegrationTime(0)
  , Termination(vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
  , Interaction(vtkLagrangianParticle::SURFACE_INTERACTION_NO_INTERACTION)
  , UserFlag(0)
  , NumberOfVariables(numberOfVariables)
  , PInsertPreviousPosition(false)
  , PManualShift(false)
{
  // Initialize equation variables and associated pointers
  this->PrevEquationVariables.resize(this->NumberOfVariables, 0);
  this->PrevVelocity = this->PrevEquationVariables.data() + 3;
  this->PrevUserVariables = this->PrevEquationVariables.data() + 6;

  this->EquationVariables.resize(this->NumberOfVariables, 0);
  this->Velocity = this->EquationVariables.data() + 3;
  this->UserVariables = this->EquationVariables.data() + 6;

  this->NextEquationVariables.resize(this->NumberOfVariables, 0);
  this->NextVelocity = this->NextEquationVariables.data() + 3;
  this->NextUserVariables = this->NextEquationVariables.data() + 6;

  // Initialize cell cache
  this->LastCellId = -1;
  this->LastDataSet = nullptr;
  this->LastLocator = nullptr;
  this->WeightsSize = weightsSize;
  this->LastWeights.resize(this->WeightsSize);

  // Initialize surface cell cache
  this->LastSurfaceCellId = -1;
  this->LastSurfaceDataSet = nullptr;

  // Initialize tracked user data
  this->PrevTrackedUserData.resize(numberOfTrackedUserData, 0);
  this->TrackedUserData.resize(numberOfTrackedUserData, 0);
  this->NextTrackedUserData.resize(numberOfTrackedUserData, 0);
}

//---------------------------------------------------------------------------
// Default destructor in implementation in order to be able to use
// vtkNew in header
vtkLagrangianParticle::~vtkLagrangianParticle() = default;

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::NewInstance(int numberOfVariables, vtkIdType seedId,
  vtkIdType particleId, vtkIdType seedArrayTupleIndex, double integrationTime,
  vtkPointData* seedData, int weightsSize, int numberOfTrackedUserData, vtkIdType numberOfSteps,
  double previousIntegrationTime)
{
  vtkLagrangianParticle* particle = new vtkLagrangianParticle(numberOfVariables, seedId, particleId,
    seedArrayTupleIndex, integrationTime, seedData, weightsSize, numberOfTrackedUserData);
  particle->NumberOfSteps = numberOfSteps;
  particle->PrevIntegrationTime = previousIntegrationTime;
  return particle;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::NewParticle(vtkIdType particleId)
{
  // Create particle and copy members
  vtkLagrangianParticle* particle =
    vtkLagrangianParticle::NewInstance(this->GetNumberOfVariables(), this->GetSeedId(), particleId,
      this->SeedArrayTupleIndex, this->IntegrationTime + this->StepTime, this->SeedData,
      this->WeightsSize, static_cast<int>(this->TrackedUserData.size()));
  particle->ParentId = this->GetId();
  particle->NumberOfSteps = this->GetNumberOfSteps() + 1;

  // Copy Variables
  std::copy(this->EquationVariables.begin(), this->EquationVariables.end(),
    particle->PrevEquationVariables.begin());
  std::copy(this->NextEquationVariables.begin(), this->NextEquationVariables.end(),
    particle->EquationVariables.begin());
  std::fill(particle->NextEquationVariables.begin(), particle->NextEquationVariables.end(), 0);

  // Copy UserData
  std::copy(this->TrackedUserData.begin(), this->TrackedUserData.end(),
    particle->PrevTrackedUserData.begin());
  std::copy(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(),
    particle->TrackedUserData.begin());
  std::fill(particle->NextTrackedUserData.begin(), particle->NextTrackedUserData.end(), 0);

  // Copy thread-specific data as well
  particle->ThreadedData = this->ThreadedData;

  return particle;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::CloneParticle()
{
  vtkLagrangianParticle* clone = vtkLagrangianParticle::NewInstance(this->GetNumberOfVariables(),
    this->GetSeedId(), this->GetId(), this->SeedArrayTupleIndex, this->IntegrationTime,
    this->GetSeedData(), this->WeightsSize, static_cast<int>(this->TrackedUserData.size()));
  clone->Id = this->Id;
  clone->ParentId = this->ParentId;
  clone->NumberOfSteps = this->NumberOfSteps;

  std::copy(this->PrevEquationVariables.begin(), this->PrevEquationVariables.end(),
    clone->PrevEquationVariables.begin());
  std::copy(this->EquationVariables.begin(), this->EquationVariables.end(),
    clone->EquationVariables.begin());
  std::copy(this->NextEquationVariables.begin(), this->NextEquationVariables.end(),
    clone->NextEquationVariables.begin());
  std::copy(this->PrevTrackedUserData.begin(), this->PrevTrackedUserData.end(),
    clone->PrevTrackedUserData.begin());
  std::copy(
    this->TrackedUserData.begin(), this->TrackedUserData.end(), clone->TrackedUserData.begin());
  std::copy(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(),
    clone->NextTrackedUserData.begin());
  clone->StepTime = this->StepTime;

  // Copy thread-specific data as well
  clone->ThreadedData = this->ThreadedData;

  return clone;
}

//---------------------------------------------------------------------------
double* vtkLagrangianParticle::GetLastWeights()
{
  return this->LastWeights.data();
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetLastCellId()
{
  return this->LastCellId;
}

//---------------------------------------------------------------------------
double* vtkLagrangianParticle::GetLastCellPosition()
{
  return this->LastCellPosition;
}

//---------------------------------------------------------------------------
vtkDataSet* vtkLagrangianParticle::GetLastDataSet()
{
  return this->LastDataSet;
}

//---------------------------------------------------------------------------
vtkAbstractCellLocator* vtkLagrangianParticle::GetLastLocator()
{
  return this->LastLocator;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetLastCell(
  vtkAbstractCellLocator* locator, vtkDataSet* dataset, vtkIdType cellId, double cellPosition[3])
{
  this->LastLocator = locator;
  this->LastDataSet = dataset;
  this->LastCellId = cellId;
  std::copy(cellPosition, cellPosition + 3, this->LastCellPosition);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetLastSurfaceCell(vtkDataSet* dataset, vtkIdType cellId)
{
  this->LastSurfaceDataSet = dataset;
  this->LastSurfaceCellId = cellId;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetLastSurfaceCellId()
{
  return this->LastSurfaceCellId;
}

//---------------------------------------------------------------------------
vtkDataSet* vtkLagrangianParticle::GetLastSurfaceDataSet()
{
  return this->LastSurfaceDataSet;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetId()
{
  return this->Id;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetParentId(vtkIdType parentId)
{
  this->ParentId = parentId;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetParentId()
{
  return this->ParentId;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetSeedId()
{
  return this->SeedId;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetNumberOfSteps()
{
  return this->NumberOfSteps;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticle::GetNumberOfVariables()
{
  return this->NumberOfVariables;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticle::GetNumberOfUserVariables()
{
  return this->NumberOfVariables - 7;
}

//---------------------------------------------------------------------------
vtkPointData* vtkLagrangianParticle::GetSeedData()
{
  return this->SeedData;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetSeedArrayTupleIndex() const
{
  return this->SeedArrayTupleIndex;
}

//---------------------------------------------------------------------------
double& vtkLagrangianParticle::GetStepTimeRef()
{
  return this->StepTime;
}

//---------------------------------------------------------------------------
double vtkLagrangianParticle::GetIntegrationTime()
{
  return this->IntegrationTime;
}

//---------------------------------------------------------------------------
double vtkLagrangianParticle::GetPrevIntegrationTime()
{
  return this->PrevIntegrationTime;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetIntegrationTime(double time)
{
  this->IntegrationTime = time;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetTermination(int termination)
{
  this->Termination = termination;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticle::GetTermination()
{
  return this->Termination;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetUserFlag(int flag)
{
  this->UserFlag = flag;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticle::GetUserFlag()
{
  return this->UserFlag;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetInteraction(int interaction)
{
  this->Interaction = interaction;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticle::GetInteraction()
{
  return this->Interaction;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetPInsertPreviousPosition(bool val)
{
  this->PInsertPreviousPosition = val;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticle::GetPInsertPreviousPosition()
{
  return this->PInsertPreviousPosition;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetPManualShift(bool val)
{
  this->PManualShift = val;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticle::GetPManualShift()
{
  return this->PManualShift;
}

//---------------------------------------------------------------------------
double vtkLagrangianParticle::GetPositionVectorMagnitude()
{
  double* current = this->GetPosition();
  double* next = this->GetNextPosition();
  double vector[3];
  vtkMath::Subtract(next, current, vector);
  return vtkMath::Norm(vector, 3);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::MoveToNextPosition()
{
  std::copy(this->EquationVariables.begin(), this->EquationVariables.end(),
    this->PrevEquationVariables.begin());
  std::copy(this->NextEquationVariables.begin(), this->NextEquationVariables.end(),
    this->EquationVariables.begin());
  std::fill(this->NextEquationVariables.begin(), this->NextEquationVariables.end(), 0);
  std::copy(
    this->TrackedUserData.begin(), this->TrackedUserData.end(), this->PrevTrackedUserData.begin());
  std::copy(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(),
    this->TrackedUserData.begin());
  std::fill(this->NextTrackedUserData.begin(), this->NextTrackedUserData.end(), 0);

  this->NumberOfSteps++;
  this->PrevIntegrationTime = this->IntegrationTime;
  this->IntegrationTime += this->StepTime;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Id: " << this->Id << std::endl;
  os << indent << "LastCellId: " << this->LastCellId << std::endl;
  os << indent << "LastDataSet: " << this->LastDataSet << std::endl;
  os << indent << "LastLocator: " << this->LastLocator << std::endl;
  os << indent << "NumberOfSteps: " << this->NumberOfSteps << std::endl;
  os << indent << "NumberOfVariables: " << this->NumberOfVariables << std::endl;
  os << indent << "ParentId: " << this->ParentId << std::endl;
  os << indent << "SeedData: " << this->SeedData << std::endl;
  os << indent << "SeedArrayTupleIndex: " << this->SeedArrayTupleIndex << std::endl;
  os << indent << "SeedId: " << this->SeedId << std::endl;
  os << indent << "StepTime: " << this->StepTime << std::endl;
  os << indent << "IntegrationTime: " << this->IntegrationTime << std::endl;
  os << indent << "Termination: " << this->Termination << std::endl;
  os << indent << "UserFlag: " << this->UserFlag << std::endl;
  os << indent << "Interaction: " << this->Interaction << std::endl;

  os << indent << "PrevEquationVariables:";
  for (auto var : this->PrevEquationVariables)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "EquationVariables:";
  for (auto var : this->EquationVariables)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "NextEquationVariables:";
  for (auto var : this->NextEquationVariables)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "PrevTrackedUserData:";
  for (auto var : this->PrevTrackedUserData)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "TrackedUserData:";
  for (auto var : this->TrackedUserData)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "NextTrackedUserData:";
  for (auto var : this->NextTrackedUserData)
  {
    os << indent << " " << var;
  }
  os << std::endl;

  os << indent << "ThreadedData: " << this->ThreadedData << std::endl;
}
