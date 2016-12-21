/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianMatidaIntegrationModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianMatidaIntegrationModel.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkLagrangianParticle.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"

#include <cstring>

vtkObjectFactoryNewMacro(vtkLagrangianMatidaIntegrationModel);

//---------------------------------------------------------------------------
vtkLagrangianMatidaIntegrationModel::vtkLagrangianMatidaIntegrationModel()
{
  // Fill the helper array
  this->SeedArrayNames->InsertNextValue("ParticleDiameter");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);
  this->SeedArrayNames->InsertNextValue("ParticleDensity");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);

  this->NumFuncs     = 6; // u, v, w, du/dt, dv/dt, dw/dt
  this->NumIndepVars = 7; // x, y, z, u, v, w, t
}

//---------------------------------------------------------------------------
vtkLagrangianMatidaIntegrationModel::~vtkLagrangianMatidaIntegrationModel()
{
}

//---------------------------------------------------------------------------
void vtkLagrangianMatidaIntegrationModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int vtkLagrangianMatidaIntegrationModel::FunctionValues(vtkDataSet* dataSet,
  vtkIdType cellId, double* weights, double* x, double* f)
{
  // Initialize output
  std::fill(f, f + 6, 0.0);

  // Check for a particle
  if (this->CurrentParticle == NULL)
  {
    vtkErrorMacro(<< "No particle to integrate");
    return 0;
  }

  // Sanity Check
  if (dataSet == NULL || cellId == -1)
  {
    vtkErrorMacro(<< "No cell or dataset to integrate the particle on. Dataset: "
      << dataSet << " CellId:" << cellId);
    return 0;
  }

  // Recover Flow Properties
  int nComponent;
  double* tmp;

  // Fetch flowVelocity at index 3
  if (!this->GetFlowOrSurfaceData(3, dataSet, cellId, weights, tmp, nComponent)
      || nComponent != 3)
  {
    vtkErrorMacro(<< "Flow velocity is not set in source flow dataset or "
      "have incorrect number of components, cannot use Matida equations");
    return 0;
  }
  double flowVelocity[3];
  std::memcpy(flowVelocity, tmp, sizeof(double) * nComponent);

  // Fetch flowDensity at index 4
  if (!this->GetFlowOrSurfaceData(4, dataSet, cellId, weights, tmp, nComponent)
      || nComponent != 1)
  {
    vtkErrorMacro(<< "Flow density is not set in source flow dataset or "
      "have incorrect number of components, cannot use Matida equations");
    return 0;
  }
  double flowDensity = *tmp;

  // Fetch flowDynamicViscosity at index 5
  if (!this->GetFlowOrSurfaceData(5, dataSet, cellId, weights, tmp, nComponent)
      || nComponent != 1)
  {
    vtkErrorMacro(<< "Flow dynamic viscosity is not set in source flow dataset or "
      "have incorrect number of components, cannot use Matida equations");
    return 0;
  }
  double flowDynamicViscosity = *tmp;

  // Fetch Particle Properties
  vtkIdType tupleIndex = this->CurrentParticle->GetSeedArrayTupleIndex();

  // Fetch Particle Diameter at index 6
  vtkDataArray* particleDiameters = vtkDataArray::SafeDownCast(
    this->GetSeedArray(6, this->CurrentParticle));
  if (particleDiameters == NULL)
  {
    vtkErrorMacro(<< "Particle diameter is not set in particle data, "
      "cannot use Matida equations");
    return 0;
  }
  double particleDiameter = particleDiameters->GetTuple1(tupleIndex);

  // Fetch Particle Density at index 7
  vtkDataArray* particleDensities = vtkDataArray::SafeDownCast(
    this->GetSeedArray(7, this->CurrentParticle));
  if (particleDensities == NULL)
  {
    vtkErrorMacro(<< "Particle density is not set in particle data, "
      "cannot use Matida equations");
    return 0;
  }
  double particleDensity = particleDensities->GetTuple1(tupleIndex);

  // Compute function values
  for (int i = 0; i<3; i++)
  {
    double drag =
      this->GetDragCoefficient(flowVelocity, this->CurrentParticle->GetVelocity(),
        flowDynamicViscosity, particleDiameter, flowDensity);
    double relax =
      this->GetRelaxationTime(flowDynamicViscosity, particleDiameter, particleDensity);
    // Matida Equation
    f[i + 3] = (relax == 0) ? std::numeric_limits<double>::infinity() :
      (flowVelocity[i] - x[i + 3]) * drag / relax;
    f[i] = x[i + 3];
  }

  const double G = 9.8; // Gravity
  f[5] -= G * (1 - (flowDensity / particleDensity));
  return 1;
}

//---------------------------------------------------------------------------
double vtkLagrangianMatidaIntegrationModel::GetRelaxationTime(
  double dynVisc, double diameter, double density)
{
  return (dynVisc == 0) ?
    std::numeric_limits<double>::infinity() :
    (density * diameter * diameter) / (18.0 * dynVisc);
}

//---------------------------------------------------------------------------
double vtkLagrangianMatidaIntegrationModel::GetDragCoefficient(
  const double* flowVelocity, const double* particleVelocity,
  double dynVisc, double particleDiameter, double flowDensity)
{
  if (dynVisc == 0)
  {
    return -1.0 * std::numeric_limits<double>::infinity();
  }
  double relativeVelocity[3];
  for (int i = 0; i < 3; i++)
  {
    relativeVelocity[i] = particleVelocity[i] - flowVelocity[i];
  }
  double relativeSpeed = vtkMath::Norm(relativeVelocity);
  double reynolds = flowDensity * relativeSpeed * particleDiameter / dynVisc;
  return (1.0 + 0.15 * pow(reynolds, 0.687));
}
