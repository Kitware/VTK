/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLagrangianIntegrationModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianMatidaIntegrationModel.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLagrangianParticle.h"
#include "vtkLagrangianParticleTracker.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataNormals.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

bool doubleEquals(double a, double b, double epsilon = 0.001)
{
  return std::abs(a - b) < epsilon;
}

int TestLagrangianIntegrationModel(int, char*[])
{
  // Create a tracker for parent tracker only
  vtkNew<vtkLagrangianParticleTracker> tracker;

  // Create a wavelet
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->Update();
  vtkImageData* waveletImg = wavelet->GetOutput();

  // Create flow data
  vtkCellData* cd = waveletImg->GetCellData();
  vtkPointData* pdFlow = waveletImg->GetPointData();
  vtkNew<vtkDoubleArray> flowVel;
  flowVel->SetNumberOfComponents(3);
  flowVel->SetName("FlowVelocity");
  flowVel->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowVel->FillComponent(0, 19);
  flowVel->FillComponent(1, 19);
  flowVel->FillComponent(2, 19);

  vtkNew<vtkDoubleArray> flowDens;
  flowDens->SetNumberOfComponents(1);
  flowDens->SetName("FlowDensity");
  flowDens->SetNumberOfTuples(waveletImg->GetNumberOfPoints());
  flowDens->FillComponent(0, 11);

  vtkNew<vtkDoubleArray> flowDynVisc;
  flowDynVisc->SetNumberOfComponents(1);
  flowDynVisc->SetName("FlowDynamicViscosity");
  flowDynVisc->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowDynVisc->FillComponent(0, 13.3);

  cd->AddArray(flowVel.Get());
  pdFlow->AddArray(flowDens.Get());
  cd->AddArray(flowDynVisc.Get());

  // Put flow in triangle
  vtkNew<vtkDataSetTriangleFilter> triangle;
  triangle->SetInputData(waveletImg);

  // Translate it
  vtkNew<vtkTransform> translation;
  int xTranslation = 20;
  translation->Translate(xTranslation, 0.0, 0.0);

  vtkNew<vtkTransformFilter> transform;
  transform->SetTransform(translation.Get());
  transform->SetInputConnection(triangle->GetOutputPort());
  transform->Update();


  // Create a particle
  vtkNew<vtkDoubleArray> vel;
  vel->SetNumberOfComponents(3);
  vel->InsertNextTuple3(17., 17., 17.);
  vel->SetName("ParticleVelocity");

  vtkNew<vtkDoubleArray> diam;
  diam->SetNumberOfComponents(1);
  diam->InsertNextValue(10);
  diam->SetName("ParticleDiameter");

  vtkNew<vtkDoubleArray> dens;
  dens->SetNumberOfComponents(1);
  dens->InsertNextValue(13);
  dens->SetName("ParticleDensity");

  vtkNew<vtkPointData> pd;
  pd->AddArray(vel.Get());
  pd->AddArray(diam.Get());
  pd->AddArray(dens.Get());

  // Test on a vtkImageData
  vtkNew<vtkLagrangianMatidaIntegrationModel> odeWavelet;
  odeWavelet->SetTracker(tracker.Get());
  double tolerance = odeWavelet->GetTolerance();
  if (tolerance != 1.0e-8)
  {
    std::cerr << "Incorect Tolerance" << std::endl;
    return EXIT_FAILURE;
  }

  int nvar = odeWavelet->GetNumberOfIndependentVariables();
  int seedIdx = 13;
  vtkLagrangianParticle* part =
    new vtkLagrangianParticle(nvar, seedIdx, seedIdx, 0, 0, pd.Get());

  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "");
  odeWavelet->SetInputArrayToProcess(3, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowVelocity");
  odeWavelet->SetInputArrayToProcess(4, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "FlowDensity");
  odeWavelet->SetInputArrayToProcess(5, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDynamicViscosity");
  odeWavelet->SetInputArrayToProcess(6, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDiameter");
  odeWavelet->SetInputArrayToProcess(7, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDensity");
  odeWavelet->SetCurrentParticle(part);
  vtkNew<vtkCellLocator> locator;
  odeWavelet->SetLocator(locator.Get());
  odeWavelet->AddDataSet(wavelet->GetOutput());

  // Test other methods
  odeWavelet->Print(std::cout);
  if (odeWavelet->GetLocator() != locator.Get())
  {
    std::cerr << "Problem with locator" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  odeWavelet->SetUseInitialIntegrationTime(true);
  if (!odeWavelet->GetUseInitialIntegrationTime())
  {
    std::cerr << "Problems with UseInitialIntegrationTime" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  odeWavelet->UseInitialIntegrationTimeOff();
  if (odeWavelet->GetUseInitialIntegrationTime())
  {
    std::cerr << "Problems with UseInitialIntegrationTime" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  odeWavelet->UseInitialIntegrationTimeOn();
  if (!odeWavelet->GetUseInitialIntegrationTime())
  {
    std::cerr << "Problems with UseInitialIntegrationTime" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  odeWavelet->SetUseInitialIntegrationTime(false);

  odeWavelet->InitializeVariablesParticleData(pd.Get());
  odeWavelet->InsertVariablesParticleData(part, pd.Get(), 0);
  odeWavelet->InitializeParticle(part);
  if (odeWavelet->CheckFreeFlightTermination(part))
  {
    std::cerr << "CheckFreeFlightTermination should not return true with a matida model" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  odeWavelet->NonPlanarQuadSupportOn();
  if (!odeWavelet->GetNonPlanarQuadSupport())
  {
    std::cerr << "Something went wrong with NonPlanarQuadSupport" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  if (odeWavelet->GetWeightsSize() != 8)
  {
    std::cerr << "Incorrect Weights Size" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  odeWavelet->ParallelManualShift(part);
  vtkPolyData* tmpPd = NULL;
  vtkDataObject* tmpDo = NULL;
  if (!odeWavelet->FinalizeOutputs(tmpPd, tmpDo))
  {
    std::cerr << "FinalizeOutputs should be doing nothing and return true with matida model" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  if (odeWavelet->GetSeedArrayNames()->GetNumberOfValues() != 4)
  {
    std::cerr << "Unexpected number of seed array names" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (odeWavelet->GetSeedArrayComps()->GetNumberOfValues() != 4)
  {
    std::cerr << "Unexpected number of seed array comps" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (odeWavelet->GetSeedArrayTypes()->GetNumberOfValues() != 4)
  {
    std::cerr << "Unexpected number of seed array type" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  if (odeWavelet->GetSurfaceArrayNames()->GetNumberOfValues() != 1)
  {
    std::cerr << "Unexpected number of surface array names" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (odeWavelet->GetSurfaceArrayComps()->GetNumberOfValues() != 1)
  {
    std::cerr << "Unexpected number of surface array comps" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (odeWavelet->GetSurfaceArrayEnumValues()->GetNumberOfValues() != 11)
  {
    std::cerr << "Unexpected number of surface array enum values" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (odeWavelet->GetSurfaceArrayTypes()->GetNumberOfValues() != 1)
  {
    std::cerr << "Unexpected number of surface array types" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  double* tmpPt = NULL;
  double tmpVald = 0;
  int tmpVali = 0;
  if (odeWavelet->ManualIntegration(tmpPt, tmpPt, 0, tmpVald, tmpVald, 0, 0, 0, tmpVald, tmpVali))
  {
    std::cerr << "ManualIntegration should do nothing and return false with matida model" << std::endl;
  }

  // Test on a vtkUnstructuredGrid
  vtkNew<vtkLagrangianMatidaIntegrationModel> odeTriangle;
  odeTriangle->SetTracker(tracker.Get());
  odeTriangle->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "");
  odeTriangle->SetInputArrayToProcess(3, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowVelocity");
  odeTriangle->SetInputArrayToProcess(4, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "FlowDensity");
  odeTriangle->SetInputArrayToProcess(5, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDynamicViscosity");
  odeTriangle->SetInputArrayToProcess(6, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDiameter");
  odeTriangle->SetInputArrayToProcess(7, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDensity");
  odeTriangle->SetCurrentParticle(part);
  odeTriangle->SetLocator(locator.Get());
  odeTriangle->AddDataSet(triangle->GetOutput());

  // Test on multiple mixed dataset
  vtkNew<vtkLagrangianMatidaIntegrationModel> odeTransform;
  odeTransform->SetTracker(tracker.Get());
  odeTransform->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "");
  odeTransform->SetInputArrayToProcess(3, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowVelocity");
  odeTransform->SetInputArrayToProcess(4, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "FlowDensity");
  odeTransform->SetInputArrayToProcess(5, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDynamicViscosity");
  odeTransform->SetInputArrayToProcess(6, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDiameter");
  odeTransform->SetInputArrayToProcess(7, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDensity");
  odeTransform->SetCurrentParticle(part);
  odeTransform->SetLocator(locator.Get());
  odeTransform->AddDataSet(transform->GetOutput());
  odeTransform->AddDataSet(wavelet->GetOutput());

  // Test domain inclusion
  double x[6];
  double y[6];
  double x0 = 10 - tolerance * 10;
  double f[6];
  for (int i = 0; i < 6; i++)
  {
    x[i] = y[i] = f[i] = 0;
  }

  while (x0 <= 10 + tolerance)
  {
    x[0]  = x0;
    y[0]  = x0 + xTranslation;
    bool imageTest = (odeWavelet->FunctionValues(x, f) == 1);
    bool locatorsTest = odeWavelet->FindInLocators(x);
    bool unstrucTest = (odeTriangle->FunctionValues(x, f) == 1);
    bool multiTest = (odeTransform->FunctionValues(y, f) == 1);
    std::cerr.precision(15);
    if (!imageTest && x[0] < 10)
    {
      std::cerr << "Image Test fail" << std::endl;
      delete part;
      return EXIT_FAILURE;
    }
    if (!locatorsTest && x[0] < 10)
    {
      std::cerr << "Locators Test fail" << std::endl;
      delete part;
      return EXIT_FAILURE;
    }
    if (!multiTest && y[0] < 10)
    {
      std::cerr << "Multi Test fail" << std::endl;
      delete part;
      return EXIT_FAILURE;
    }
    if (!unstrucTest && x[0] < 10)
    {
      std::cerr << "Ustruct Test fail" << std::endl;
      delete part;
      return EXIT_FAILURE;
    }
    x0 += tolerance;
  }

  // Test clear
  odeTriangle->ClearDataSets();
  odeTriangle->AddDataSet(transform->GetOutput());
  x[0] = 0;
  if (odeTriangle->FunctionValues(x, f) == 1)
  {
    std::cerr << "ClearDataSets does not seem to work" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  x[3] = 1.3;
  x[4] = 1.4;
  x[5] = 1.5;
  odeTransform->FunctionValues(x, f);
  if (f[0] != 1.3 ||
      f[1] != 1.4 ||
      f[2] != 1.5 ||
      !doubleEquals(f[3], 26.2735) ||
      !doubleEquals(f[4], 26.125) ||
      !doubleEquals(f[5], 24.4689))
  {
    std::cerr << "Unexpected value from Integration Model" << std::endl;
    std::cerr << f[0] << " " << f[1] << " " << f[2] << " " << f[3] << " " << f[4]
      << " " << f[5] << " " << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  odeWavelet->ClearDataSets();

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputData(waveletImg);

  vtkNew<vtkPolyDataNormals> normals;
  normals->ComputePointNormalsOff();
  normals->ComputeCellNormalsOn();
  normals->SetInputConnection(surface->GetOutputPort());
  normals->Update();
  vtkPolyData* surfacePd = normals->GetOutput();

  // Create Surface data
  vtkNew<vtkDoubleArray> surfaceTypeModel;
  surfaceTypeModel->SetNumberOfComponents(1);
  surfaceTypeModel->SetName("SurfaceTypeModel");
  surfaceTypeModel->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypeModel->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_MODEL);
  surfacePd->GetCellData()->AddArray(surfaceTypeModel.Get());

  vtkNew<vtkDoubleArray> surfaceTypeTerm;
  surfaceTypeTerm->SetNumberOfComponents(1);
  surfaceTypeTerm->SetName("SurfaceTypeTerm");
  surfaceTypeTerm->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypeTerm->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_TERM);
  surfacePd->GetCellData()->AddArray(surfaceTypeTerm.Get());

  vtkNew<vtkDoubleArray> surfaceTypeBounce;
  surfaceTypeBounce->SetNumberOfComponents(1);
  surfaceTypeBounce->SetName("SurfaceTypeBounce");
  surfaceTypeBounce->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypeBounce->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BOUNCE);
  surfacePd->GetCellData()->AddArray(surfaceTypeBounce.Get());

  vtkNew<vtkDoubleArray> surfaceTypeBreak;
  surfaceTypeBreak->SetNumberOfComponents(1);
  surfaceTypeBreak->SetName("SurfaceTypeBreak");
  surfaceTypeBreak->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypeBreak->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BREAK);
  surfacePd->GetCellData()->AddArray(surfaceTypeBreak.Get());

  vtkNew<vtkDoubleArray> surfaceTypePass;
  surfaceTypePass->SetNumberOfComponents(1);
  surfaceTypePass->SetName("SurfaceTypePass");
  surfaceTypePass->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypePass->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS);
  surfacePd->GetCellData()->AddArray(surfaceTypePass.Get());

  odeWavelet->AddDataSet(surfacePd, true);
  if (odeWavelet->GetSurfaceArrayDefaultValues()->GetNumberOfValues() != 1)
  {
    std::cerr << "Unexpected number of surface array default values" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  double* pos = part->GetPosition();
  pos[0] = 0;
  pos[1] = 0;
  pos[2] = 0;

  double* nextPos = part->GetNextPosition();
  std::queue<vtkLagrangianParticle*> particles;
  unsigned int interactedSurfaceFlatIndex;
  vtkLagrangianBasicIntegrationModel::PassThroughParticlesType passThroughParticles;

  // Check no effect
  odeWavelet->PreIntegrate(particles);

  nextPos[0] = 20;
  nextPos[1] = 0;
  nextPos[2] = 0;
  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceTypeModel");
  vtkLagrangianParticle* interactionParticle = odeWavelet->ComputeSurfaceInteraction(
    part, particles, interactedSurfaceFlatIndex, passThroughParticles);
  if (!interactionParticle)
  {
    std::cerr << "No interaction with SurfaceTypeModel" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  delete interactionParticle;

  if (nextPos[0] > 10 + tolerance || nextPos[0] < 10 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypeModel"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 0 || passThroughParticles.size() != 0)
  {
    std::cerr << "Unexpected new particles created with SurfaceTypeModel"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index with SurfaceTypeModel"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  part->SetLastSurfaceCell(NULL, -1);
  nextPos[0] = 20;
  nextPos[1] = 0;
  nextPos[2] = 0;
  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceTypeTerm");
  vtkLagrangianParticle* interactionParticle2 = odeWavelet->ComputeSurfaceInteraction(
    part, particles, interactedSurfaceFlatIndex, passThroughParticles);
  if (!interactionParticle2)
  {
    std::cerr << "No interaction with SurfaceTypeTerm" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  delete interactionParticle2;
  if (nextPos[0] > 10 + tolerance || nextPos[0] < 10 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypeTerm"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 0 || passThroughParticles.size() != 0)
  {
    std::cerr << "Unexpected number particles created with SurfaceTypeTerm"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index with SurfaceTypeTerm"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  part->SetLastSurfaceCell(NULL, -1);
  nextPos[0] = 20;
  nextPos[1] = 0;
  nextPos[2] = 0;
  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceTypeBounce");
  vtkLagrangianParticle* interactionParticle3 = odeWavelet->ComputeSurfaceInteraction(
    part, particles, interactedSurfaceFlatIndex, passThroughParticles);
  if (!interactionParticle3)
  {
    std::cerr << "No interaction with SurfaceTypeBounce" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  delete interactionParticle3;
  if (nextPos[0] > 10 + tolerance || nextPos[0] < 10 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypeBounce"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 0 || passThroughParticles.size() != 0)
  {
    std::cerr << "Unexpected number particles created with SurfaceTypeBounce:"
      << particles.size() << " " << passThroughParticles.size() << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index with SurfaceTypeBounce"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  pos[0] = 9;
  pos[1] = 0;
  pos[2] = 0;
  nextPos[0] = 12;
  nextPos[1] = 0;
  nextPos[2] = 0;
  vtkLagrangianParticle* interactionParticle4 = odeWavelet->ComputeSurfaceInteraction(
    part, particles, interactedSurfaceFlatIndex, passThroughParticles);
  if (interactionParticle4)
  {
    std::cerr << "Unexpected interaction with SurfaceTypeBounce perforation management"
      << std::endl;
    delete part;
    delete interactionParticle4;
    return EXIT_FAILURE;
  }
  if (nextPos[0] > 6 + tolerance || nextPos[0] < 6 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypeBounce perforation"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 0 || passThroughParticles.size() != 0)
  {
    std::cerr << "Unexpected number particles created with SurfaceTypeBounce perforation:"
      << particles.size() << " " << passThroughParticles.size() << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index "
      "with SurfaceTypeBounce perforation" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  part->SetLastSurfaceCell(NULL, -1);
  pos[0] = 0;
  pos[1] = 0;
  pos[2] = 0;
  nextPos[0] = 20;
  nextPos[1] = 0;
  nextPos[2] = 0;
  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceTypeBreak");
  vtkLagrangianParticle* interactionParticle5 = odeWavelet->ComputeSurfaceInteraction(part, particles,
    interactedSurfaceFlatIndex, passThroughParticles);
  if (!interactionParticle5)
  {
    std::cerr << "No interaction with SurfaceTypeBreak" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  delete interactionParticle5;
  if (nextPos[0] > 10 + tolerance || nextPos[0] < 10 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypeBreak"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 2 || passThroughParticles.size() != 0)
  {
    std::cerr << "Unexpected number particles created with SurfaceTypeBreak:"
      << particles.size() << " " << passThroughParticles.size() << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index with SurfaceTypeBreak"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  part->SetLastSurfaceCell(NULL, -1);
  delete particles.front();
  particles.pop();
  delete particles.front();
  particles.pop();
  nextPos[0] = 20;
  nextPos[1] = 0;
  nextPos[2] = 0;
  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceTypePass");
  vtkLagrangianParticle* interactionParticle6 = odeWavelet->ComputeSurfaceInteraction(part, particles,
    interactedSurfaceFlatIndex, passThroughParticles);
  if (interactionParticle6)
  {
    std::cerr << "Unexpected interaction with SurfaceTypePass" << std::endl;
    delete part;
    delete interactionParticle6;
    return EXIT_FAILURE;
  }

  if (nextPos[0] > 20 + tolerance || nextPos[0] < 20 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypePass"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 0 || passThroughParticles.size() != 1)
  {
    std::cerr << "Unexpected number particles created with SurfaceTypePass: "
      << particles.size() << " " << passThroughParticles.size() << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index with SurfaceTypePass"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }

  odeWavelet->ClearDataSets(true);
  part->SetLastSurfaceCell(NULL, -1);
  delete passThroughParticles.front().second;
  passThroughParticles.pop();
  nextPos[0] = 20;
  nextPos[1] = 0;
  nextPos[2] = 0;
  odeWavelet->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceTypeModel");
  vtkLagrangianParticle* interactionParticle7 = odeWavelet->ComputeSurfaceInteraction(
    part, particles, interactedSurfaceFlatIndex, passThroughParticles);
  if (interactionParticle7)
  {
    std::cerr << "Unexpected interaction with SurfaceTypeModel Cleared"
      << std::endl;
    delete interactionParticle7;
    delete part;
    return EXIT_FAILURE;
  }
  if (nextPos[0] > 20 + tolerance || nextPos[0] < 20 - tolerance
    || nextPos[1] != 0 || nextPos[2] != 0)
  {
    std::cerr << "Unexpected interaction position with SurfaceTypeModel Cleared"
      << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (particles.size() != 0 || passThroughParticles.size() != 0)
  {
    std::cerr << "Unexpected new particles created with SurfaceTypeModel Cleared"
      << particles.size() << " " << passThroughParticles.size() << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  if (interactedSurfaceFlatIndex != 0)
  {
    std::cerr << "Unexpected Interacted surface flat index "
      "with SurfaceTypeModel Cleared" << std::endl;
    delete part;
    return EXIT_FAILURE;
  }
  delete part;
  return EXIT_SUCCESS;
}
