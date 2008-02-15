/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExodusTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkCamera.h"

#include "vtkExodusReader.h"
#include "vtkTemporalShiftScale.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkActor.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkSmartPointer.h"
#include "vtkTemporalDataSet.h"
#include "vtkThreshold.h"
#include "vtkTemporalInterpolator.h"
#include "vtkPolyDataMapper.h"


//-------------------------------------------------------------------------
int TestExodusTime(int argc, char *argv[])
{
  // we have to use a compsite pipeline
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // create the reader
  vtkSmartPointer<vtkExodusReader> reader = 
    vtkSmartPointer<vtkExodusReader>::New();
  reader->SetFileName("C:/can.ex2");

  // shift and scale the time range to that it run from -0.5 to 0.5
  vtkSmartPointer<vtkTemporalShiftScale> tempss = 
    vtkSmartPointer<vtkTemporalShiftScale>::New();
  tempss->SetScale(232.5);
  tempss->SetInputConnection(reader->GetOutputPort());

  // interpolate if needed
  vtkSmartPointer<vtkTemporalInterpolator> interp = 
    vtkSmartPointer<vtkTemporalInterpolator>::New();
  interp->SetInputConnection(tempss->GetOutputPort());
  
  vtkSmartPointer<vtkThreshold> contour = 
    vtkSmartPointer<vtkThreshold>::New();
  contour->SetInputConnection(interp->GetOutputPort());
  contour->ThresholdByUpper(0.5);

  vtkSmartPointer<vtkCompositeDataGeometryFilter> geom = 
    vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
  geom->SetInputConnection(contour->GetOutputPort());

  // map them
  vtkSmartPointer<vtkPolyDataMapper> mapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(geom->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("BlockId");
  mapper->SetScalarRange(0,3);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer = 
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderer->AddActor( actor );
  renderer->SetBackground(0.5, 0.5, 0.5);

  renWin->AddRenderer( renderer );
  renWin->SetSize( 300, 300 ); 
  iren->SetRenderWindow( renWin );
  renWin->Render();

  renderer->GetActiveCamera()->Elevation(-120);

  // ask for some specific data points
  vtkStreamingDemandDrivenPipeline *sdd = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(geom->GetExecutive());
  double times[1];
  times[0] = 0;
  int i;
  for (i = 0; i < 100; ++i)
    {
    times[0] = i/100.0;
    sdd->SetUpdateTimeSteps(0, times, 1);
    mapper->Modified();
    renderer->ResetCamera();
    //renderer->SetBackground(0.5*(i%2), 0.5, 0.5*((i/2)%2));
    renWin->Render();
    }
  
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  return !retVal;
}
