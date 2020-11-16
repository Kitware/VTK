/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResetCameraScreenSpace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// Test the method ResetCameraScreenSpace
//
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

// int main( int argc, char* argv[] )
int TestResetCameraScreenSpace(int argc, char* argv[])
{
  vtkCylinderSource* c = vtkCylinderSource::New();
  c->SetHeight(8.0);

  vtkPolyDataMapper* m = vtkPolyDataMapper::New();
  m->SetInputConnection(c->GetOutputPort());

  vtkActor* a = vtkActor::New();
  a->SetMapper(m);
  a->RotateZ(90);
  a->RotateX(80);

  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren1->AddActor(a);
  ren1->SetBackground(0.1, 0.2, 0.4);

  renWin->SetSize(200, 300);

  ren1->GetActiveCamera()->SetUseHorizontalViewAngle(true);
  ren1->ResetCameraScreenSpace();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  c->Delete();
  m->Delete();
  a->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
