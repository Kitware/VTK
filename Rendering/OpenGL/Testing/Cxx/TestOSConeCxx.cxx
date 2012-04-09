/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSConeCxx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers offscreen rendering.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkConeSource.h"

int TestOSConeCxx(int argc, char* argv[])
{
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->OffScreenRenderingOn();
  
  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();
  
  vtkConeSource *cone=vtkConeSource::New();
  vtkPolyDataMapper *mapper=vtkPolyDataMapper::New();
  mapper->SetInputConnection(cone->GetOutputPort());
  cone->Delete();
  
  vtkActor *actor=vtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();
  
  renderer->AddActor(actor);
  actor->Delete();
  
#if 0
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  
  renWin->Render();
  
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  
  // Cleanup
  iren->Delete();
#else // the interactor version fails with OSMesa.
  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  renWin->Delete();
#endif
  return !retVal;
}
