/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor3DDepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkTextActor3D with depth peeling.
// As this actor uses vtkImageActor underneath, it also tests vtkImageActor
// with depth peeling.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"

int TestTextActor3DDepthPeeling(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  
  renWin->SetMultiSamples(1);
  renWin->SetAlphaBitPlanes(1);
  
  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();
  
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);
  
  renderer->SetBackground(0.0,0.0,0.5); 
  renWin->SetSize(300,300);
  
  vtkTextActor3D *actor=vtkTextActor3D::New();
  renderer->AddActor(actor);
  actor->Delete();

  actor->SetInput("0123456789.");
  vtkTextProperty *textProperty=vtkTextProperty::New();
  actor->SetTextProperty(textProperty);
  textProperty->Delete();
  
  actor->SetPosition(3,4,5);
  actor->SetScale(0.05,0.05,1);
  textProperty->SetJustificationToCentered();
  textProperty->SetVerticalJustificationToCentered(); // default
  textProperty->SetFontFamilyToArial(); // default.
  
  renWin->Render();
  if(renderer->GetLastRenderingUsedDepthPeeling())
    {
    cout<<"depth peeling was used"<<endl;
    }
  else
    {
    cout<<"depth peeling was not used (alpha blending instead)"<<endl;
    }
  
  renderer->ResetCamera();
  
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanup
  iren->Delete();
  return !retVal;
}
