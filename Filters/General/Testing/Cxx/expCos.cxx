/*=========================================================================

  Program:   Visualization Toolkit
  Module:    expCos.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// Brute force computation of Bessel functions. Might be better to create a
// filter (or source) object. Might also consider vtkSampleFunction.

#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkWarpScalar.h"

int expCos( int , char *[] )
{
  int i, numPts;
  double x[3];
  double r, deriv;

  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // create plane to warp
  vtkSmartPointer<vtkPlaneSource> plane =
    vtkSmartPointer<vtkPlaneSource>::New();
  plane->SetResolution (300,300);

  vtkSmartPointer<vtkTransform> transform =
    vtkSmartPointer<vtkTransform>::New();
  transform->Scale(10.0,10.0,1.0);

  vtkSmartPointer<vtkTransformPolyDataFilter> transF =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transF->SetInputConnection(plane->GetOutputPort());
  transF->SetTransform(transform);
  transF->Update();

  // compute Bessel function and derivatives. This portion could be
  // encapsulated into source or filter object.
  //
  vtkSmartPointer<vtkPolyData> input = transF->GetOutput();
  numPts = input->GetNumberOfPoints();

  vtkSmartPointer<vtkPoints> newPts =
    vtkSmartPointer<vtkPoints>::New();
  newPts->SetNumberOfPoints(numPts);

  vtkSmartPointer<vtkFloatArray> derivs =
    vtkSmartPointer<vtkFloatArray>::New();
  derivs->SetNumberOfTuples(numPts);

  vtkSmartPointer<vtkPolyData> bessel =
    vtkSmartPointer<vtkPolyData>::New();
  bessel->CopyStructure(input);
  bessel->SetPoints(newPts);
  bessel->GetPointData()->SetScalars(derivs);

  for (i=0; i<numPts; i++)
  {
    input->GetPoint(i,x);
    r = sqrt(static_cast<double>(x[0]*x[0]) + x[1]*x[1]);
    x[2] = exp(-r) * cos (10.0*r);
    newPts->SetPoint(i,x);
    deriv = -exp(-r) * (cos(10.0*r) + 10.0*sin(10.0*r));
    derivs->SetValue(i,deriv);
  }

  // warp plane
  vtkSmartPointer<vtkWarpScalar> warp =
    vtkSmartPointer<vtkWarpScalar>::New();
  warp->SetInputData(bessel);
  warp->XYPlaneOn();
  warp->SetScaleFactor(0.5);

  // mapper and actor
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputConnection(warp->GetOutputPort());
  double tmp[2];
  bessel->GetScalarRange(tmp);
  mapper->SetScalarRange(tmp[0],tmp[1]);

  vtkSmartPointer<vtkActor> carpet =
    vtkSmartPointer<vtkActor>::New();
  carpet->SetMapper(mapper);

  // assign our actor to the renderer
  ren->AddActor(carpet);
  ren->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // draw the resulting scene
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);
  ren->GetActiveCamera()->Elevation(-55);
  ren->GetActiveCamera()->Azimuth(25);
  ren->ResetCameraClippingRange();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
