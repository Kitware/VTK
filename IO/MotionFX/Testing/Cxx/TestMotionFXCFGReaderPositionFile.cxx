/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMotionFXCFGReaderPositionFile.h

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

int TestMotionFXCFGReaderPositionFile(int argc, char* argv[])
{
  return impl::Test(argc, argv, "Data/MotionFX/position_file/generic_chain_input.cfg",
    [](vtkRenderWindow*, vtkRenderer* renderer, vtkMotionFXCFGReader*) {
      auto camera = renderer->GetActiveCamera();
      camera->SetFocalPoint(0.09, -0.02, -0.13);
      camera->SetPosition(0.15, -0.37, 0.15);
      camera->SetViewUp(-0.89, -0.35, -0.25);
      renderer->ResetCamera();
    });
}
