// SPDX-FileCopyrightText: Copyright (c) Peng Fei
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLagrangianMatidaIntegrationModel.h"
#include "vtkLagrangianParticleTracker.h"
#include "vtkLagrangianThreadedData.h"
#include "vtkNew.h"
#include "vtkPointData.h"

int TestLagrangianParticleTrackerWithGravity(int, char*[])
{
  // 1/3 flow data

  vtkNew<vtkImageData> flow;
  flow->SetDimensions(20, 20, 60);
  double voxel = 1e-3;
  flow->SetSpacing(voxel, voxel, voxel);

  vtkNew<vtkFloatArray> flow_velocity;
  flow_velocity->SetNumberOfComponents(3);
  flow_velocity->SetNumberOfTuples(flow->GetNumberOfCells());
  flow_velocity->SetName("FlowVelocity");

  vtkNew<vtkFloatArray> flow_density;
  flow_density->SetNumberOfComponents(1);
  flow_density->SetNumberOfTuples(flow->GetNumberOfCells());
  flow_density->SetName("FlowDensity");

  vtkNew<vtkFloatArray> flow_viscosity;
  flow_viscosity->SetNumberOfComponents(1);
  flow_viscosity->SetNumberOfTuples(flow->GetNumberOfCells());
  flow_viscosity->SetName("FlowDynamicViscosity");

  flow_velocity->FillComponent(0, 0);
  flow_velocity->FillComponent(1, 0);
  flow_velocity->FillComponent(2, -0.36);
  flow_density->FillComponent(0, 1.225);
  flow_viscosity->FillComponent(0, 1.79e-5);

  flow->GetCellData()->AddArray(flow_velocity);
  flow->GetCellData()->AddArray(flow_density);
  flow->GetCellData()->AddArray(flow_viscosity);

  // 2/3 seed data

  vtkNew<vtkPoints> pts;
  pts->InsertNextPoint(flow->GetCenter());
  vtkNew<vtkPolyData> seeds;
  seeds->SetPoints(pts);

  vtkNew<vtkFloatArray> particle_velocity;
  particle_velocity->SetNumberOfComponents(3);
  particle_velocity->SetNumberOfTuples(seeds->GetNumberOfPoints());
  particle_velocity->SetName("InitialVelocity");

  vtkNew<vtkFloatArray> particle_density;
  particle_density->SetNumberOfComponents(1);
  particle_density->SetNumberOfTuples(seeds->GetNumberOfPoints());
  particle_density->SetName("ParticleDensity");

  vtkNew<vtkFloatArray> particle_diameter;
  particle_diameter->SetNumberOfComponents(1);
  particle_diameter->SetNumberOfTuples(seeds->GetNumberOfPoints());
  particle_diameter->SetName("ParticleDiameter");

  particle_velocity->FillComponent(0, 0);
  particle_velocity->FillComponent(1, 0);
  particle_velocity->FillComponent(2, 0);
  particle_density->FillComponent(0, 1550);
  particle_diameter->FillComponent(0, 1e-4);

  seeds->GetPointData()->AddArray(particle_velocity);
  seeds->GetPointData()->AddArray(particle_density);
  seeds->GetPointData()->AddArray(particle_diameter);

  // 3/3 vtkLagrangianParticleTracker

  vtkNew<vtkLagrangianMatidaIntegrationModel> integrationModel;
  integrationModel->SetInputArrayToProcess(
    0, 1, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "InitialVelocity");
  integrationModel->SetInputArrayToProcess(
    6, 1, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDiameter");
  integrationModel->SetInputArrayToProcess(
    7, 1, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDensity");
  integrationModel->SetInputArrayToProcess(
    3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowVelocity");
  integrationModel->SetInputArrayToProcess(
    4, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDensity");
  integrationModel->SetInputArrayToProcess(
    5, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDynamicViscosity");

  // The flow velocity is [0, 0, -0.36]
  // and apply a gravity [9.8, 0, 0] perpendicular to the flow velocity direction
  //
  // The particle pathline should have a significant offset in the direction of gravity
  integrationModel->SetGravity(9.8, 0, 0);

  vtkNew<vtkLagrangianParticleTracker> tracker;
  tracker->SetIntegrationModel(integrationModel);
  tracker->SetInputData(flow);
  tracker->SetSourceData(seeds);
  tracker->AdaptiveStepReintegrationOn();
  tracker->SetMaximumNumberOfSteps(300);
  tracker->Update();

  vtkPolyData* streams = vtkPolyData::SafeDownCast(tracker->GetOutput());
  streams->GetPointData()->SetActiveScalars("StepNumber");

  double pos_start[3] = { 0, 0, 0 };
  double pos_stop[3] = { 0, 0, 0 };
  streams->GetPoint(0, pos_start);
  streams->GetPoint(streams->GetNumberOfPoints() - 1, pos_stop);

  double offset[3] = { std::abs(pos_stop[0] - pos_start[0]), std::abs(pos_stop[1] - pos_start[1]),
    std::abs(pos_stop[2] - pos_start[2]) };

  int offset_x = (int)std::ceil(offset[0] / voxel);
  if (10 > offset_x)
  {
    std::cerr << "offset " << offset_x << " voxels in X, gravity doesn't work" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
