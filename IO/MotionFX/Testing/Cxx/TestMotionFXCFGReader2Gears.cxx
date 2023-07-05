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

int TestMotionFXCFGReader2Gears(int argc, char* argv[])
{
  return impl::Test(argc, argv, "Data/MotionFX/2_gears/rotate_motion.cfg",
    [](vtkRenderWindow*, vtkRenderer* renderer, vtkMotionFXCFGReader*) {
      auto camera = renderer->GetActiveCamera();
      camera->SetFocalPoint(0.09, -0.02, -0.13);
      camera->SetPosition(0.15, -0.37, 0.15);
      camera->SetViewUp(-0.89, -0.35, -0.25);
      renderer->ResetCamera();
    });
}
