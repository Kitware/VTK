/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestGlyph3DMapperEdges(int argc, char* argv[])
{
  int res=1;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res,res);

  vtkNew<vtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-1,-1,-1);
  colors->SetHighPoint(0.5,0.5,0.5);

  vtkNew<vtkSphereSource> squad;
  squad->SetPhiResolution(5);
  squad->SetThetaResolution(9);

  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetScaleFactor(1.2);
  glypher->SetSourceConnection(squad->GetOutputPort());

  vtkNew<vtkActor> glyphActor1;
  glyphActor1->SetMapper(glypher.Get());
  glyphActor1->GetProperty()->SetEdgeVisibility(1);
  glyphActor1->GetProperty()->SetEdgeColor(1.0,0.5,0.5);
  // glyphActor1->GetProperty()->SetRenderLinesAsTubes(1);
  // glyphActor1->GetProperty()->SetLineWidth(5);

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer.Get());
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  //set up the view
  renderer->SetBackground(0.2,0.2,0.2);
  renWin->SetSize(300,300);

  renderer->AddActor(glyphActor1.Get());

  ////////////////////////////////////////////////////////////

  //run the test

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
