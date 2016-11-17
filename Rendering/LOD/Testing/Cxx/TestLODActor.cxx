/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLODActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The test creates a Kline, replaces the default strategy from vtkMaskPoints
// to vtkQuadricClustering ; so instead of seeing a point cloud during
// interaction, (when run with -I) you will see a low res kline.

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkLODActor.h"
#include "vtkLoopSubdivisionFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkProperty.h"
#include "vtkQuadricClustering.h"
#include "vtkMaskPoints.h"

int TestLODActor( int argc, char* argv[])
{
  // Create a Kline.

  vtkPoints *points = vtkPoints::New();
  points->InsertNextPoint(0,-16,0);
  points->InsertNextPoint(0,0,-14);
  points->InsertNextPoint(0,0,14);
  points->InsertNextPoint(14,0,0);
  points->InsertNextPoint(10,20,-10);
  points->InsertNextPoint(10,20,10);
  points->InsertNextPoint(10,-20,-10);
  points->InsertNextPoint(10,-20,10);
  points->InsertNextPoint(-10,-20,-10);
  points->InsertNextPoint(-10,-20,10);
  points->InsertNextPoint(-10,20,-10);
  points->InsertNextPoint(-10,20,10);
  points->InsertNextPoint(-2,27,0);
  points->InsertNextPoint(0,27,2);
  points->InsertNextPoint(0,27,-2);
  points->InsertNextPoint(2,27,0);
  points->InsertNextPoint(-14,4,-1);
  points->InsertNextPoint(-14,3,0);
  points->InsertNextPoint(-14,5,0);
  points->InsertNextPoint(-14,4,1);
  points->InsertNextPoint(-1,38,-2);
  points->InsertNextPoint(-1,38,2);
  points->InsertNextPoint(2,35,-2);
  points->InsertNextPoint(2,35,2);
  points->InsertNextPoint(17,42,0);
  points->InsertNextPoint(15,40,2);
  points->InsertNextPoint(15,39,-2);
  points->InsertNextPoint(13,37,0);
  points->InsertNextPoint(19,-2,-2);
  points->InsertNextPoint(19,-2,2);
  points->InsertNextPoint(15,2,-2);
  points->InsertNextPoint(15,2,2);

  vtkCellArray *faces = vtkCellArray::New();
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(5);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(7);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(7);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(9);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(8);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(8);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(10);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(9);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(7);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(5);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(11);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(5);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(15);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(11);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(13);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(10);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(12);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(14);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(12);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(20);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(14);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(22);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(15);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(23);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(13);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(21);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(24);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(20);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(26);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(22);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(27);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(23);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(25);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(21);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(24);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(28);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(26);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(30);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(27);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(31);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(25);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(29);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(28);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(30);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(31);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(29);


  vtkPolyData * model = vtkPolyData::New();
  model->SetPolys(faces);
  model->SetPoints(points);

  // Create the RenderWindow, Renderer and both Actors

  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkLoopSubdivisionFilter *subdivide = vtkLoopSubdivisionFilter::New();
    subdivide->SetInputData (model);
    subdivide->SetNumberOfSubdivisions(6);

  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
     mapper->SetInputConnection (subdivide->GetOutputPort());

  vtkLODActor *rose = vtkLODActor::New();
      rose->SetMapper (mapper);

  // Now replace the default strategy of the LOD Actor from to show a low
  // resolution kline. We will use vtkQuadricClustering for this purpose.

  vtkQuadricClustering * q = vtkQuadricClustering::New();
  q->SetNumberOfXDivisions(8);
  q->SetNumberOfYDivisions(8);
  q->SetNumberOfZDivisions(8);
  q->UseInputPointsOn();
  rose->SetLowResFilter(q);
  q->Delete();

  q = vtkQuadricClustering::New();
  q->SetNumberOfXDivisions(5);
  q->SetNumberOfYDivisions(5);
  q->SetNumberOfZDivisions(5);
  q->UseInputPointsOn();
  rose->SetMediumResFilter(q);
  q->Delete();

  // Add the actors to the renderer, set the background and size

  ren1->AddActor(rose);

  vtkProperty *backP = vtkProperty::New();
  backP->SetDiffuseColor (1, 1, .3);
  rose->SetBackfaceProperty (backP);

  rose->GetProperty()->SetDiffuseColor( 1, .4, .3);
  rose->GetProperty()->SetSpecular(.4);
  rose->GetProperty()->SetDiffuse(.8);
  rose->GetProperty()->SetSpecularPower(40);

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image

  ren1->ResetCamera();
  vtkCamera * cam1 = ren1->GetActiveCamera();
  cam1->Azimuth(-90);
  ren1->ResetCameraClippingRange();
  iren->Initialize();
  iren->SetDesiredUpdateRate(500);

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  model->Delete();
  backP->Delete();
  mapper->Delete();
  subdivide->Delete();
  ren1->Delete();
  renWin->Delete();
  rose->Delete();
  faces->Delete();
  points->Delete();
  iren->Delete();

  return !retVal;
}
