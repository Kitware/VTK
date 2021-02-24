/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMotionFXCFGReaderClover.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
