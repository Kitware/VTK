/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAxisActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the spider plot capabilities in VTK.
#include "vtkAxisActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkStringArray.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestAxisActor3D( int argc, char * argv [] )
{
  // Create the axis actor
  vtkSmartPointer<vtkAxisActor> axis = vtkSmartPointer<vtkAxisActor>::New();
  axis->SetPoint1(0,0,0);
  axis->SetPoint2(1,1,1);
  axis->SetBounds(0,1,0,0,0,0);
  axis->SetTickLocationToBoth();
  axis->SetAxisTypeToX();
  axis->SetTitle("1.0");
  axis->SetMajorTickSize(0.01);
  axis->SetRange(0,1);
  vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();
  labels->SetNumberOfTuples(3);
  labels->SetValue(0,"X");
  labels->SetValue(1,"Y");
  labels->SetValue(2,"Z");
//  axis->SetLabels(labels);
  axis->SetLabelScale(0.01);
  axis->SetTitleScale(0.02);
  axis->MinorTicksVisibleOff();
  axis->SetDeltaMajor(0.1);

  // Create the RenderWindow, Renderer and both Actors
  vtkSmartPointer<vtkRenderer> ren1 = vtkRenderer::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  axis->SetCamera(ren1->GetActiveCamera());

  ren1->AddActor(axis);
  ren1->SetBackground(0,0,0);
  renWin->SetSize(500,200);

  // render the image
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
