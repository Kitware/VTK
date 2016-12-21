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
vtkLagrangianParticle::vtkLagrangianParticle(int numberOfVariables,
  vtkIdType seedId, vtkIdType particleId, vtkIdType seedArrayTupleIndex,
  double integrationTime, vtkPointData* seedData):
  Id(particleId),
  ParentId(-1),
  SeedId(seedId),
  SeedArrayTupleIndex(seedArrayTupleIndex),
  NumberOfSteps(0),
  SeedData(seedData),
  StepTime(0),
  IntegrationTime(integrationTime),
  PrevIntegrationTime(0),
  Termination(vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED),
  Interaction(vtkLagrangianParticle::SURFACE_INTERACTION_NO_INTERACTION),
  UserFlag(0),
  NumberOfVariables(numberOfVariables),
  PInsertPreviousPosition(false),
  PManualShift(false)
{
  // Initialize equation variables and associated pointers
  this->PrevEquationVariables = new double[this->NumberOfVariables];
  this->PrevVelocity = this->PrevEquationVariables + 3;
  this->PrevUserVariables = this->PrevEquationVariables + 6;

  this->EquationVariables = new double[this->NumberOfVariables];
  this->Velocity = this->EquationVariables + 3;
  this->UserVariables = this->EquationVariables + 6;

  this->NextEquationVariables = new double [this->NumberOfVariables];
  this->NextVelocity = this->NextEquationVariables + 3;
  this->NextUserVariables = this->NextEquationVariables + 6;

  memset(
    this->PrevEquationVariables, 0, this->NumberOfVariables * sizeof(double));
  memset(
    this->EquationVariables, 0, this->NumberOfVariables * sizeof(double));
  memset(
    this->NextEquationVariables, 0, this->NumberOfVariables*sizeof(double));

  // Initialize cell cache
  this->LastCellId = -1;
  this->LastDataSet = NULL;

  // Initialize surface cell cache
  this->LastSurfaceCellId = -1;
  this->LastSurfaceDataSet = NULL;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::NewInstance(int numberOfVariables,
  vtkIdType seedId, vtkIdType particleId, vtkIdType seedArrayTupleIndex,
  double integrationTime, vtkPointData* seedData)
{
  return new vtkLagrangianParticle(numberOfVariables, seedId, particleId,
    seedArrayTupleIndex, integrationTime, seedData);
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::NewInstance(int numberOfVariables,
  vtkIdType seedId, vtkIdType particleId, vtkIdType seedArrayTupleIndex,
  double integrationTime, vtkPointData* seedData,
  vtkIdType numberOfSteps, double previousIntegrationTime)
{
  vtkLagrangianParticle* particle = new vtkLagrangianParticle(numberOfVariables,
    seedId, particleId, seedArrayTupleIndex, integrationTime, seedData);
  particle->NumberOfSteps = numberOfSteps;
  particle->PrevIntegrationTime = previousIntegrationTime;
  return particle;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::NewParticle(vtkIdType particleId)
{
  // Copy point data tuples
  vtkPointData* seedData = this->GetSeedData();
  vtkIdType seedArrayTupleIndex = this->GetSeedArrayTupleIndex();
  if (seedData->GetNumberOfArrays() > 0)
  {
    vtkIdType parentSeedArrayTupleIndex = seedArrayTupleIndex;
    seedArrayTupleIndex = seedData->GetArray(0)->GetNumberOfTuples();
    seedData->CopyAllocate(
      seedData, seedArrayTupleIndex + 1);
    seedData->CopyData(
      seedData, parentSeedArrayTupleIndex, seedArrayTupleIndex);
  }

  // Create particle and copy members
  vtkLagrangianParticle* particle = this->NewInstance(
    this->GetNumberOfVariables(), this->GetSeedId(), particleId,
    seedArrayTupleIndex, this->IntegrationTime + this->StepTime, seedData);
  particle->ParentId = this->GetId();
  particle->NumberOfSteps = this->GetNumberOfSteps() + 1;

  // Copy Variables
  memcpy(particle->GetPrevEquationVariables(), this->GetEquationVariables(),
    this->NumberOfVariables * sizeof(double));
  memcpy(particle->GetEquationVariables(), this->GetNextEquationVariables(),
    this->NumberOfVariables * sizeof(double));
  memset(particle->NextEquationVariables, 0, this->NumberOfVariables * sizeof(double));
  return particle;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianParticle::CloneParticle()
{
  vtkLagrangianParticle* clone = this->NewInstance(
    this->GetNumberOfVariables(), this->GetSeedId(), this->GetId(),
    this->GetSeedArrayTupleIndex(), this->IntegrationTime, this->GetSeedData());
  clone->Id = this->Id;
  clone->ParentId = this->ParentId;
  clone->NumberOfSteps = this->NumberOfSteps;

  memcpy(clone->GetPrevEquationVariables(), this->GetPrevEquationVariables(),
    this->NumberOfVariables * sizeof(double));
  memcpy(clone->GetEquationVariables(), this->GetEquationVariables(),
    this->NumberOfVariables * sizeof(double));
  memcpy(clone->GetNextEquationVariables(), this->GetNextEquationVariables(),
    this->NumberOfVariables * sizeof(double));
  clone->StepTime = this->StepTime;
  return clone;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle::~vtkLagrangianParticle()
{
  // Delete equation variables
  delete[] this->PrevEquationVariables;
  delete[] this->NextEquationVariables;
  delete[] this->EquationVariables;
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticle::GetLastCellId()
{
  return this->LastCellId;
}

//---------------------------------------------------------------------------
vtkDataSet* vtkLagrangianParticle::GetLastDataSet()
{
  return this->LastDataSet;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::SetLastCell(vtkDataSet* dataset, vtkIdType cellId)
{
  this->LastDataSet = dataset;
  this->LastCellId = cellId;
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
vtkIdType vtkLagrangianParticle::GetSeedArrayTupleIndex()
{
  return this->SeedArrayTupleIndex;
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
  double* current = this->GetEquationVariables();
  double* next = this->GetNextEquationVariables();
  double vector[3];
  vtkMath::Subtract(next, current, vector);
  return vtkMath::Norm(vector, 3);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticle::MoveToNextPosition()
{
  memcpy(this->PrevEquationVariables, this->EquationVariables,
    this->NumberOfVariables * sizeof(double));
  memcpy(this->EquationVariables, this->NextEquationVariables,
    this->NumberOfVariables * sizeof(double));
  memset(
    this->NextEquationVariables, 0, this->NumberOfVariables * sizeof(double));

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
  os << indent << "NumberOfSteps: " << this->NumberOfSteps << std::endl;
  os << indent << "NumberOfVariables: " << this->NumberOfVariables << std::endl;
  os << indent << "ParentId: " << this->ParentId << std::endl;
  os << indent << "SeedData: " << this->SeedData << std::endl;
  os << indent << "SeedId: " << this->SeedId << std::endl;
  os << indent << "SeedArrayTupleIndex: " << this->SeedArrayTupleIndex << std::endl;
  os << indent << "StepTime: " << this->StepTime << std::endl;
  os << indent << "IntegrationTime: " << this->IntegrationTime << std::endl;
  os << indent << "Termination: " << this->Termination << std::endl;
  os << indent << "UserFlag: " << this->UserFlag << std::endl;
  os << indent << "Interaction: " << this->Interaction << std::endl;

  os << indent << "PrevEquationVariables:";
  for (int i = 0; i < this->NumberOfVariables; i++)
  {
    os << indent << " " << this->PrevEquationVariables[i];
  }
  os << std::endl;

  os << indent << "EquationVariables:";
  for (int i = 0; i < this->NumberOfVariables; i++)
  {
    os << indent << " " << this->EquationVariables[i];
  }
  os << std::endl;

  os << indent << "NextEquationVariables:";
  for (int i = 0; i < this->NumberOfVariables; i++)
  {
    os << indent << " " << this->NextEquationVariables[i];
  }
  os << std::endl;
}
