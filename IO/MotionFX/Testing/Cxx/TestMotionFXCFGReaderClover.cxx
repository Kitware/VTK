// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestMotionFXCFGReaderCommon.h"

#include <vtkCamera.h>
#include <vtkInformation.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTestUtilities.h>

#include <vector>

int TestMotionFXCFGReaderClover(int argc, char* argv[])
{
  return impl::Test(argc, argv, "Data/MotionFX/clover/clover_utm.cfg",
    [](vtkRenderWindow*, vtkRenderer* renderer, vtkMotionFXCFGReader*) {
      auto camera = renderer->GetActiveCamera();
      camera->SetFocalPoint(1.1, 2.25, -0.75);
      camera->SetPosition(-16.0, 15.0, 13.0);
      camera->SetViewUp(0.0, 1.0, 0.0);
      renderer->ResetCamera();
    });
}
