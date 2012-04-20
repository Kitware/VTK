/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBSplineWarp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This tests B-spline image warping

#include "vtkImageBSplineInterpolator.h"
#include "vtkImageBSplineCoefficients.h"
#include "vtkImageReslice.h"
#include "vtkBSplineTransform.h"
#include "vtkTransformToGrid.h"
#include "vtkThinPlateSplineTransform.h"
#include "vtkImageViewer.h"
#include "vtkImageGridSource.h"
#include "vtkImageBlend.h"
#include "vtkImageMapToColors.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"

int TestBSplineWarp(int argc, char *argv[])
{
  // first, create an image that looks like
  // graph paper by combining two image grid
  // sources via vtkImageBlend
  vtkSmartPointer<vtkImageGridSource> imageGrid1 =
    vtkSmartPointer<vtkImageGridSource>::New();
  imageGrid1->SetGridSpacing(4,4,0);
  imageGrid1->SetGridOrigin(0,0,0);
  imageGrid1->SetDataExtent(0,255,0,255,0,0);
  imageGrid1->SetDataScalarTypeToUnsignedChar();

  vtkSmartPointer<vtkImageGridSource> imageGrid2 =
    vtkSmartPointer<vtkImageGridSource>::New();
  imageGrid2->SetGridSpacing(16,16,0);
  imageGrid2->SetGridOrigin(0,0,0);
  imageGrid2->SetDataExtent(0,255,0,255,0,0);
  imageGrid2->SetDataScalarTypeToUnsignedChar();

  vtkSmartPointer<vtkLookupTable> table1 =
    vtkSmartPointer<vtkLookupTable>::New();
  table1->SetTableRange(0,1);
  table1->SetValueRange(1.0,0.7);
  table1->SetSaturationRange(0.0,1.0);
  table1->SetHueRange(0.12,0.12);
  table1->SetAlphaRange(1.0,1.0);
  table1->Build();

  vtkSmartPointer<vtkLookupTable> table2 =
    vtkSmartPointer<vtkLookupTable>::New();
  table2->SetTableRange(0,1);
  table2->SetValueRange(1.0,0.0);
  table2->SetSaturationRange(0.0,0.0);
  table2->SetHueRange(0.0,0.0);
  table2->SetAlphaRange(0.0,1.0);
  table2->Build();

  vtkSmartPointer<vtkImageMapToColors> map1 =
    vtkSmartPointer<vtkImageMapToColors>::New();
  map1->SetInputConnection(imageGrid1->GetOutputPort());
  map1->SetLookupTable(table1);

  vtkSmartPointer<vtkImageMapToColors> map2 =
    vtkSmartPointer<vtkImageMapToColors>::New();
  map2->SetInputConnection(imageGrid2->GetOutputPort());
  map2->SetLookupTable(table2);

  vtkSmartPointer<vtkImageBlend> blend =
    vtkSmartPointer<vtkImageBlend>::New();
  blend->AddInputConnection(map1->GetOutputPort());
  blend->AddInputConnection(map2->GetOutputPort());

  // next, create a ThinPlateSpline transform, which
  // will then be used to create the B-spline transform
  vtkSmartPointer<vtkPoints> p1 =
    vtkSmartPointer<vtkPoints>::New();
  p1->SetNumberOfPoints(8);
  p1->SetPoint(0,0,0,0);
  p1->SetPoint(1,0,255,0);
  p1->SetPoint(2,255,0,0);
  p1->SetPoint(3,255,255,0);
  p1->SetPoint(4,96,96,0);
  p1->SetPoint(5,96,159,0);
  p1->SetPoint(6,159,159,0);
  p1->SetPoint(7,159,96,0);

  vtkSmartPointer<vtkPoints> p2 =
    vtkSmartPointer<vtkPoints>::New();
  p2->SetNumberOfPoints(8);
  p2->SetPoint(0,0,0,0);
  p2->SetPoint(1,0,255,0);
  p2->SetPoint(2,255,0,0);
  p2->SetPoint(3,255,255,0);
  p2->SetPoint(4,96,159,0);
  p2->SetPoint(5,159,159,0);
  p2->SetPoint(6,159,96,0);
  p2->SetPoint(7,96,96,0);

  vtkSmartPointer<vtkThinPlateSplineTransform> thinPlate =
    vtkSmartPointer<vtkThinPlateSplineTransform>::New();
  thinPlate->SetSourceLandmarks(p2);
  thinPlate->SetTargetLandmarks(p1);
  thinPlate->SetBasisToR2LogR();

  // convert the thin plate spline into a B-spline, by
  // sampling it onto a grid and then computing the
  // B-spline coefficients
  vtkSmartPointer<vtkTransformToGrid> transformToGrid =
    vtkSmartPointer<vtkTransformToGrid>::New();
  transformToGrid->SetInput(thinPlate);
  transformToGrid->SetGridSpacing(16.0, 16.0, 1.0);
  transformToGrid->SetGridOrigin(0.0, 0.0, 0.0);
  transformToGrid->SetGridExtent(0,16, 0,16, 0,0);

  vtkSmartPointer<vtkImageBSplineCoefficients> grid =
    vtkSmartPointer<vtkImageBSplineCoefficients>::New();
  grid->SetInputConnection(transformToGrid->GetOutputPort());
  grid->UpdateWholeExtent();

  // create the B-spline transform, scale the deformation by
  // half to demonstrate how deformation scaling works
  vtkSmartPointer<vtkBSplineTransform> transform =
    vtkSmartPointer<vtkBSplineTransform>::New();
  transform->SetCoefficientData(grid->GetOutput());
  transform->SetDisplacementScale(0.5);
  transform->SetBorderModeToZero();

  // invert the transform before passing it to vtkImageReslice
  transform->Inverse();

  // reslice the image through the B-spline transform,
  // using B-spline interpolation and the "Repeat"
  // boundary condition
  vtkSmartPointer<vtkImageBSplineCoefficients> prefilter =
    vtkSmartPointer<vtkImageBSplineCoefficients>::New();
  prefilter->SetInputConnection(blend->GetOutputPort());
  prefilter->SetBorderModeToRepeat();
  prefilter->SetSplineDegree(3);

  vtkSmartPointer<vtkImageBSplineInterpolator> interpolator =
    vtkSmartPointer<vtkImageBSplineInterpolator>::New();
  interpolator->SetSplineDegree(3);

  vtkSmartPointer<vtkImageReslice> reslice =
    vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInputConnection(prefilter->GetOutputPort());
  reslice->SetResliceTransform(transform);
  reslice->WrapOn();
  reslice->SetInterpolator(interpolator);
  reslice->SetOutputSpacing(1.0, 1.0, 1.0);
  reslice->SetOutputOrigin(-32.0, -32.0, 0.0);
  reslice->SetOutputExtent(0, 319, 0, 319, 0, 0);

  // set the window/level to 255.0/127.5 to view full range
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkImageViewer> viewer =
    vtkSmartPointer<vtkImageViewer>::New();
  viewer->SetupInteractor(iren);
  viewer->SetInputConnection(reslice->GetOutputPort());
  viewer->SetColorWindow(255.0);
  viewer->SetColorLevel(127.5);
  viewer->SetZSlice(0);
  viewer->Render();

  vtkRenderWindow *renWin = viewer->GetRenderWindow();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
