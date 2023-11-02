// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkMersenneTwister.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointGaussianMapper.h"
#include "vtkPointSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

int TestPointGaussianMapperAnisotropic(int argc, char* argv[])
{
  int desiredPoints = 100;

  vtkNew<vtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(10.0);
  points->Update();

  vtkPolyData* polyData = points->GetOutput();

  vtkNew<vtkFloatArray> scale;
  scale->SetName("scale");
  scale->SetNumberOfComponents(3);
  scale->SetNumberOfTuples(desiredPoints);

  vtkNew<vtkFloatArray> orientation;
  orientation->SetName("rotation");
  orientation->SetNumberOfComponents(4);
  orientation->SetNumberOfTuples(desiredPoints);

  vtkNew<vtkMersenneTwister> seq;
  seq->InitializeSequence(0, 0);

  constexpr float two_pi = vtkMath::Pi() * 2.f;

  for (int i = 0; i < desiredPoints; i++)
  {
    float s[3];
    s[0] = 0.01f + seq->GetValue();
    seq->Next();
    s[1] = 0.01f + seq->GetValue();
    seq->Next();
    s[2] = 0.01f + seq->GetValue();
    seq->Next();

    float u = seq->GetValue();
    seq->Next();
    float v = seq->GetValue();
    seq->Next();
    float w = seq->GetValue();
    seq->Next();
    float q[4] = { std::sqrt(1.f - u) * std::sin(two_pi * v),
      std::sqrt(1.f - u) * std::cos(two_pi * v), std::sqrt(u) * std::sin(two_pi * w),
      std::sqrt(u) * std::cos(two_pi * w) };

    scale->SetTypedTuple(i, s);
    orientation->SetTypedTuple(i, q);
  }

  polyData->GetPointData()->AddArray(scale);
  polyData->GetPointData()->AddArray(orientation);

  vtkNew<vtkPointGaussianMapper> mapper;
  mapper->SetInputData(polyData);
  mapper->EmissiveOff();
  mapper->SetSplatShaderCode("//VTK::Color::Impl\n"
                             "  float dist = sqrt(dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy));\n"
                             "  if (dist > 1.0) { discard; }\n"
                             "  float scale = (1.0 - dist);\n"
                             "  ambientColor *= scale;\n"
                             "  diffuseColor *= scale;\n");
  mapper->SetBoundScale(1.0);
  mapper->AnisotropicOn();
  mapper->SetScaleArray("scale");
  mapper->SetRotationArray("rotation");
  mapper->SetLowpassMatrix(1e-5, 0, 1e-5);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();

  renderer->GetActiveCamera()->Zoom(2.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
