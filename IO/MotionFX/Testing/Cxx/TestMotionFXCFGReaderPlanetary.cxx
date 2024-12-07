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

int TestMotionFXCFGReaderPlanetary(int argc, char* argv[])
{
  return impl::Test(argc, argv, "Data/MotionFX/planetary/Planetary_prescribedCOMmotion.cfg",
    [](vtkRenderWindow*, vtkRenderer* renderer, vtkMotionFXCFGReader*)
    {
      auto camera = renderer->GetActiveCamera();
      camera->SetFocalPoint(-412.84, 121.00, -304.88);
      camera->SetPosition(-412.17, 121.27, -305.32);
      camera->SetViewUp(0.54, 0.032, 0.83);
      renderer->ResetCamera();
    });
}
