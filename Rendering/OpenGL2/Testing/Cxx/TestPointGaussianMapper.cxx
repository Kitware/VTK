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
// This file is based loosely on the PointSprites plugin developed
// and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>


#include "vtkActor.h"
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

#include "vtkPolyDataReader.h"

//#define TestPoints
//#define TestFile
#define TestSplats

int TestPointGaussianMapper(int argc, char *argv[])
{
  int desiredPoints = 1.0e4;

  vtkNew<vtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints,0.33)*20.0);
  points->Update();

  vtkNew<vtkRandomAttributeGenerator> randomAttr;
  randomAttr->SetInputConnection(points->GetOutputPort());

  vtkNew<vtkPointGaussianMapper> mapper;

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  renderer->AddActor(actor.Get());

#ifdef TestPoints
  randomAttr->SetDataTypeToUnsignedChar();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->SetMinimumComponentValue(0);
  randomAttr->SetMaximumComponentValue(255);
  randomAttr->Update();
  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetScaleFactor(0.0);
  mapper->EmissiveOff();
#endif

#ifdef TestFile
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName("filename");
  reader->Update();

  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SelectColorArray("Color");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetScalefactor(0.0);
  mapper->EmissiveOff();

  //actor->GetProperty()->SetPointSize(3.0);
#endif

#ifdef TestSplats
  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->Update();

  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointVectors");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointScalars");

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(0.0,0.1,1.0,0.8);
  ctf->AddHSVPoint(1.0,0.2,0.5,1.0);
  ctf->SetColorSpaceToRGB();
  mapper->SetLookupTable(ctf.Get());
#endif

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

  int numPts = mapper->GetInput()->GetPoints()->GetNumberOfPoints();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  cerr << "number of points: " <<  numPts << endl;
  cerr << "points per second: " <<  numPts*(numRenders/elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  //  renderer->GetActiveCamera()->Print(cerr);

  renderer->GetActiveCamera()->Zoom(10.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
