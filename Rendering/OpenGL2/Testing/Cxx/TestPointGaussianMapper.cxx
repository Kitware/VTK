/*=========================================================================

 Program:   Visualization Toolkit
 Module:    TestSprites.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// this program tests the point sprite support by vtkPointSpriteProperty.


#include "vtkActor.h"
#include "vtkBrownianPoints.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkPointGaussianMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkNew.h"
#include "vtkTimerLog.h"

#include "vtkPointSource.h"
#include "vtkColorTransferFunction.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestPointGaussianMapper(int argc, char *argv[])
{
  int desiredPoints = 1.0e4;

  vtkNew<vtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints,0.33)*5.0);

  vtkNew<vtkBrownianPoints> randomVector;
  randomVector->SetMinimumSpeed(0);
  randomVector->SetMaximumSpeed(1);
  randomVector->SetInputConnection(points->GetOutputPort());

  vtkNew<vtkRandomAttributeGenerator> randomAttr;
  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->SetInputConnection(randomVector->GetOutputPort());

  randomAttr->Update();

  vtkNew<vtkPointGaussianMapper> mapper;
  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointScalars");

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(0.0,0.1,1.0,0.8);
 // ctf->AddHSVPoint(0.2,0.2,0.0,1.0);
 // ctf->AddHSVPoint(0.7,0.6,0.0,1.0);
  ctf->AddHSVPoint(1.0,0.2,0.5,1.0);
  ctf->SetColorSpaceToRGB();
  mapper->SetLookupTable(ctf.Get());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(900, 900);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 85;
  for (int i = 0; i < numRenders; ++i)
    {
    renderer->GetActiveCamera()->Azimuth(1);
    renderer->GetActiveCamera()->Elevation(1);
    renderWindow->Render();
    }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  cerr << "number of points: " <<  desiredPoints << endl;
  cerr << "points per second: " <<  desiredPoints*(numRenders/elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();

  renderWindow->SetSize(300, 300);
  renderer->GetActiveCamera()->Zoom(10.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return EXIT_SUCCESS;
}
