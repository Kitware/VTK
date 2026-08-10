// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkNew.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkSphereSource.h"

#include <iostream>

int TestLowMemMapAttributes(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(64);
  sphereSource->SetPhiResolution(64);

  vtkNew<vtkElevationFilter> elevationFilter;
  elevationFilter->SetInputConnection(sphereSource->GetOutputPort());
  elevationFilter->SetLowPoint(-1, 0, 0);
  elevationFilter->SetHighPoint(1, 0, 0);
  elevationFilter->SetScalarRange(0, 1);
  elevationFilter->Update();

  vtkNew<vtkRandomAttributeGenerator> randomAttributes;
  randomAttributes->SetInputConnection(elevationFilter->GetOutputPort());
  randomAttributes->GenerateAllDataOff();
  randomAttributes->GeneratePointScalarsOn();

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkOpenGLLowMemoryPolyDataMapper> mapper;
  mapper->SetInputConnection(randomAttributes->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Map the "Elevation" and "RandomPointScalars" data arrays to the same custom vertex attribute
  // named "custom". The two arrays are appended together in the same texture.
  mapper->MapDataArrayToVertexAttribute(
    "custom", "Elevation", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  mapper->MapDataArrayToVertexAttribute(
    "custom", "RandomPointScalars", vtkDataObject::FIELD_ASSOCIATION_POINTS);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddVertexShaderReplacement("//VTK::Normal::Dec", true,
    "//VTK::Normal::Dec\n"
    "uniform samplerBuffer custom;\n",
    false);

  // Retrieve the elevation and random values from the custom texture and use them to displace the
  // vertex position along the normal.
  sp->AddVertexShaderReplacement("//VTK::Normal::Impl", true,
    "//VTK::Normal::Impl\n"
    "  float elevation = texelFetchBuffer(custom, pointId).r;\n"
    "  float random = texelFetchBuffer(custom, pointCount + pointId).r;\n"
    "  vertexMC.xyz += 0.3 * normalMC * elevation * random;\n",
    false);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  // Warm-up render.
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
