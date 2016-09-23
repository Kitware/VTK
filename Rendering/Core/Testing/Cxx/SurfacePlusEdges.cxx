/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SurfacePlusEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// This test draws a sphere with the edges shown.  It also turns on coincident
// topology resolution with a z-shift to both make sure the wireframe is
// visible and to exercise that type of coincident topology resolution.

#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

int SurfacePlusEdges(int argc, char *argv[])
{
  vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
  vtkMapper::SetResolveCoincidentTopologyZShift(0.1);

  VTK_CREATE(vtkSphereSource, sphere);

  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(sphere->GetOutputPort());

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.0);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  renwin->SetSize(250, 250);
  renwin->SetMultiSamples(0);

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    VTK_CREATE(vtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
