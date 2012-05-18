/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalFractal.cxx

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

#include "vtkTemporalFractal.h"
#include "vtkTemporalShiftScale.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkContourFilter.h"
#include "vtkActor.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkSmartPointer.h"
#include "vtkThreshold.h"
#include "vtkTemporalInterpolator.h"
#include "vtkPolyDataMapper.h"

//-------------------------------------------------------------------------
int TestTemporalFractal(int argc, char *argv[])
{
  // we have to use a compsite pipeline
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // create temporal fractals
  vtkSmartPointer<vtkTemporalFractal> fractal =
    vtkSmartPointer<vtkTemporalFractal>::New();
  fractal->SetMaximumLevel(3);
  fractal->DiscreteTimeStepsOn();
  fractal->GenerateRectilinearGridsOn();
//  fractal->SetAdaptiveSubdivision(0);

  // shift and scale the time range to that it run from -0.5 to 0.5
  vtkSmartPointer<vtkTemporalShiftScale> tempss =
    vtkSmartPointer<vtkTemporalShiftScale>::New();
  tempss->SetScale(0.1);
  tempss->SetPostShift(-0.5);
  tempss->SetInputConnection(fractal->GetOutputPort());

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

  // ask for some specific data points
  vtkStreamingDemandDrivenPipeline *sdd =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(geom->GetExecutive());
  sdd->UpdateInformation();
  double time = -0.6;
  int i;
  for (i = 0; i < 10; ++i)
    {
    time = i/25.0 - 0.5;
    sdd->SetUpdateTimeStep(0, time);
    mapper->Modified();
    renderer->ResetCameraClippingRange();
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
