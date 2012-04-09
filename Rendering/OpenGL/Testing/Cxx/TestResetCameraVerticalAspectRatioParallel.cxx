/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResetCameraVerticalAspectRatioParallel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// Make sure that on a window with vertical aspect ratio, the camera is
// reset properly with parallel projection.
//
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkCylinderSource.h"

// int main( int argc, char* argv[] )
int TestResetCameraVerticalAspectRatioParallel(int argc, char* argv[])
{
  vtkCylinderSource *c=vtkCylinderSource::New();
  c->SetHeight(4.0);
  
  vtkPolyDataMapper *m=vtkPolyDataMapper::New();
  m->SetInputConnection(c->GetOutputPort());
  
  vtkActor *a=vtkActor::New();
  a->SetMapper(m);
  a->RotateZ(-90.0);
  
  vtkRenderer *ren1=vtkRenderer::New();
  vtkRenderWindow *renWin=vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  
  ren1->AddActor(a);
  ren1->SetBackground(0.1,0.2,0.4);
  
  // Width cannot be smaller than 104 and 108 respectively on Windows XP and
  // Vista because of decorations. And apparently not smaller than 116 on
  // Vista with standard style and 24" wide screen.
  renWin->SetSize(128,400);
  
  ren1->GetActiveCamera()->ParallelProjectionOn();
  // ren1->GetActiveCamera()->SetUseHorizontalViewAngle(1);
  ren1->ResetCamera();
  
  int retVal = vtkRegressionTestImage( renWin );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
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
