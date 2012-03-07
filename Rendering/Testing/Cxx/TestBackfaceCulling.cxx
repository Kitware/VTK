/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBackfaceCulling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests backface culling with a text actor.
//

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkSphereSource.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkNew.h"

int TestBackfaceCulling(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.GetPointer());
  renderer->SetBackground(0.0, 0.0, 0.5);
  renWin->SetSize(300, 300);

  // Set up the sphere
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  mapper->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(mapper.GetPointer());
  actor->GetProperty()->SetColor(0, 1, 0);
  actor->GetProperty()->SetBackfaceCulling(1);
  renderer->AddActor(actor.GetPointer());

  // Set up the text renderer.
  vtkNew<vtkTextActor> text;
  renderer->AddActor(text.GetPointer());
  text->SetInput("Can you see me?");
  text->SetDisplayPosition(3, 4);

  renWin->Render();
  renderer->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
