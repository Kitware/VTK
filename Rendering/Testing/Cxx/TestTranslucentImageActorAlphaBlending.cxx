/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTranslucentImageActorAlphaBlending.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering of translucent image actor with alpha blending.
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
#include "vtkPNGReader.h"
#include "vtkImageActor.h"
#include "vtkCamera.h"

int TestTranslucentImageActorAlphaBlending(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  
  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();
  
  vtkImageActor *ia=vtkImageActor::New();
  renderer->AddActor(ia);
  ia->Delete();
  
  vtkPNGReader *pnmReader=vtkPNGReader::New();
  ia->SetInput(pnmReader->GetOutput());
  pnmReader->Delete();
  
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/alphachannel.png");
  
  pnmReader->SetFileName(fname);
  delete[] fname;
  
  renderer->SetBackground(0.1,0.2,0.4);
  renWin->SetSize(400,400);
  
  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  iren->Delete();
  
  return !retVal;
}
