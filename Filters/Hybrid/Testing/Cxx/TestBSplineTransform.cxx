/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBSplineTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
  This test builds a thin-plate spline transform, and then approximates
  it with a B-Spline transform.  It applies both the B-Spline transform
  and the original thin-plate spline transform to a polydata so that they
  can be compared.

  The output image is displayed as eight separate panels, as follows:

  Top row:
    1) thin-plate spline applied to a sphere
    2) B-spline applied to a sphere
    3) thin-plate spline applied to a sphere with normals
    4) B-spline applied to a sphere with normals
  Bottom row:
    Same as top row, but with inverted transform
*/

#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkBSplineTransform.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransformToGrid.h>
#include <vtkImageBSplineCoefficients.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>

int TestBSplineTransform(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(600,300);

  // Make a sphere
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);
  sphere->Update();

  // Make another sphere, with no normals
  vtkSmartPointer<vtkPolyData> sphereData =
    vtkSmartPointer<vtkPolyData>::New();
  sphereData->SetPoints(sphere->GetOutput()->GetPoints());
  sphereData->SetPolys(sphere->GetOutput()->GetPolys());

  // ---------------------------
  // start with a thin plate spline transform
  vtkSmartPointer<vtkPoints> spoints =
    vtkSmartPointer<vtkPoints>::New();
  spoints->SetNumberOfPoints(10);
  spoints->SetPoint(0,0.000,0.000,0.500);
  spoints->SetPoint(1,0.000,0.000,-0.500);
  spoints->SetPoint(2,0.433,0.000,0.250);
  spoints->SetPoint(3,0.433,0.000,-0.250);
  spoints->SetPoint(4,-0.000,0.433,0.250);
  spoints->SetPoint(5,-0.000,0.433,-0.250);
  spoints->SetPoint(6,-0.433,-0.000,0.250);
  spoints->SetPoint(7,-0.433,-0.000,-0.250);
  spoints->SetPoint(8,0.000,-0.433,0.250);
  spoints->SetPoint(9,0.000,-0.433,-0.250);

  vtkSmartPointer<vtkPoints> tpoints =
    vtkSmartPointer<vtkPoints>::New();
  tpoints->SetNumberOfPoints(10);
  tpoints->SetPoint(0,0.000,0.000,0.800);
  tpoints->SetPoint(1,0.000,0.000,-0.200);
  tpoints->SetPoint(2,0.433,0.000,0.350);
  tpoints->SetPoint(3,0.433,0.000,-0.150);
  tpoints->SetPoint(4,-0.000,0.233,0.350);
  tpoints->SetPoint(5,-0.000,0.433,-0.150);
  tpoints->SetPoint(6,-0.433,-0.000,0.350);
  tpoints->SetPoint(7,-0.433,-0.000,-0.150);
  tpoints->SetPoint(8,0.000,-0.233,0.350);
  tpoints->SetPoint(9,0.000,-0.433,-0.150);

  vtkSmartPointer<vtkThinPlateSplineTransform> thin =
    vtkSmartPointer<vtkThinPlateSplineTransform>::New();
  thin->SetSourceLandmarks(spoints);
  thin->SetTargetLandmarks(tpoints);
  thin->SetBasisToR2LogR();

  // First pane: thin-plate, no normals
  vtkSmartPointer<vtkTransformPolyDataFilter> f11 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f11->SetInputData(sphereData);
  f11->SetTransform(thin);

  vtkSmartPointer<vtkPolyDataMapper> m11 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m11->SetInputConnection(f11->GetOutputPort());

  vtkSmartPointer<vtkActor> a11 =
    vtkSmartPointer<vtkActor>::New();
  a11->SetMapper(m11);
  a11->RotateY(90);
  a11->GetProperty()->SetColor(1,0,0);

  vtkSmartPointer<vtkRenderer> ren11 =
    vtkSmartPointer<vtkRenderer>::New();
  ren11->SetViewport(0.0,0.5,0.25,1.0);
  ren11->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren11->AddActor(a11);
  renWin->AddRenderer(ren11);

  // Invert the transform
  vtkSmartPointer<vtkTransformPolyDataFilter> f12 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f12->SetInputData(sphereData);
  f12->SetTransform(thin->GetInverse());

  vtkSmartPointer<vtkPolyDataMapper> m12 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m12->SetInputConnection(f12->GetOutputPort());

  vtkSmartPointer<vtkActor> a12 =
    vtkSmartPointer<vtkActor>::New();
  a12->SetMapper(m12);
  a12->RotateY(90);
  a12->GetProperty()->SetColor(0.9,0.9,0);

  vtkSmartPointer<vtkRenderer> ren12 =
    vtkSmartPointer<vtkRenderer>::New();
  ren12->SetViewport(0.0,0.0,0.25,0.5);
  ren12->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren12->AddActor(a12);
  renWin->AddRenderer(ren12);

  // Second pane: b-spline transform, no normals
  vtkSmartPointer<vtkTransformToGrid> transformToGrid =
    vtkSmartPointer<vtkTransformToGrid>::New();
  transformToGrid->SetInput(thin);
  transformToGrid->SetGridOrigin(-1.5,-1.5,-1.5);
  transformToGrid->SetGridExtent(0,60,0,60,0,60);
  transformToGrid->SetGridSpacing(0.05,0.05,0.05);

  vtkSmartPointer<vtkImageBSplineCoefficients> coeffs =
    vtkSmartPointer<vtkImageBSplineCoefficients>::New();
  coeffs->SetInputConnection(transformToGrid->GetOutputPort());

  vtkSmartPointer<vtkBSplineTransform> t2 =
    vtkSmartPointer<vtkBSplineTransform>::New();
  t2->SetCoefficientConnection(coeffs->GetOutputPort());

  vtkSmartPointer<vtkTransformPolyDataFilter> f21 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f21->SetInputData(sphereData);
  f21->SetTransform(t2);

  vtkSmartPointer<vtkPolyDataMapper> m21 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m21->SetInputConnection(f21->GetOutputPort());

  vtkSmartPointer<vtkActor> a21 =
    vtkSmartPointer<vtkActor>::New();
  a21->SetMapper(m21);
  a21->RotateY(90);
  a21->GetProperty()->SetColor(1,0,0);

  vtkSmartPointer<vtkRenderer> ren21 =
    vtkSmartPointer<vtkRenderer>::New();
  ren21->SetViewport(0.25,0.5,0.50,1.0);
  ren21->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren21->AddActor(a21);
  renWin->AddRenderer(ren21);

  // Invert the transform
  vtkSmartPointer<vtkTransformPolyDataFilter> f22 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f22->SetInputData(sphereData);
  f22->SetTransform(t2->GetInverse());

  vtkSmartPointer<vtkPolyDataMapper> m22 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m22->SetInputConnection(f22->GetOutputPort());

  vtkSmartPointer<vtkActor> a22 =
    vtkSmartPointer<vtkActor>::New();
  a22->SetMapper(m22);
  a22->RotateY(90);
  a22->GetProperty()->SetColor(0.9,0.9,0);

  vtkSmartPointer<vtkRenderer> ren22 =
    vtkSmartPointer<vtkRenderer>::New();
  ren22->SetViewport(0.25,0.0,0.50,0.5);
  ren22->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren22->AddActor(a22);
  renWin->AddRenderer(ren22);

  // Third pane: thin-plate, no normals
  vtkSmartPointer<vtkTransformPolyDataFilter> f31 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f31->SetInputConnection(sphere->GetOutputPort());
  f31->SetTransform(thin);

  vtkSmartPointer<vtkPolyDataMapper> m31 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m31->SetInputConnection(f31->GetOutputPort());

  vtkSmartPointer<vtkActor> a31 =
    vtkSmartPointer<vtkActor>::New();
  a31->SetMapper(m31);
  a31->RotateY(90);
  a31->GetProperty()->SetColor(1,0,0);

  vtkSmartPointer<vtkRenderer> ren31 =
    vtkSmartPointer<vtkRenderer>::New();
  ren31->SetViewport(0.50,0.5,0.75,1.0);
  ren31->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren31->AddActor(a31);
  renWin->AddRenderer(ren31);

  // Invert the transform
  vtkSmartPointer<vtkTransformPolyDataFilter> f32 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f32->SetInputConnection(sphere->GetOutputPort());
  f32->SetTransform(thin->GetInverse());

  vtkSmartPointer<vtkPolyDataMapper> m32 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m32->SetInputConnection(f32->GetOutputPort());

  vtkSmartPointer<vtkActor> a32 =
    vtkSmartPointer<vtkActor>::New();
  a32->SetMapper(m32);
  a32->RotateY(90);
  a32->GetProperty()->SetColor(0.9,0.9,0);

  vtkSmartPointer<vtkRenderer> ren32 =
    vtkSmartPointer<vtkRenderer>::New();
  ren32->SetViewport(0.5,0.0,0.75,0.5);
  ren32->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren32->AddActor(a32);
  renWin->AddRenderer(ren32);

  // Third pane: b-spline, normals
  vtkSmartPointer<vtkBSplineTransform> t4 =
    vtkSmartPointer<vtkBSplineTransform>::New();
  t4->SetCoefficientConnection(coeffs->GetOutputPort());

  vtkSmartPointer<vtkTransformPolyDataFilter> f41 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f41->SetInputConnection(sphere->GetOutputPort());
  f41->SetTransform(t4);

  vtkSmartPointer<vtkPolyDataMapper> m41 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m41->SetInputConnection(f41->GetOutputPort());

  vtkSmartPointer<vtkActor> a41 =
    vtkSmartPointer<vtkActor>::New();
  a41->SetMapper(m41);
  a41->RotateY(90);
  a41->GetProperty()->SetColor(1,0,0);

  vtkSmartPointer<vtkRenderer> ren41 =
    vtkSmartPointer<vtkRenderer>::New();
  ren41->SetViewport(0.75,0.5,1.0,1.0);
  ren41->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren41->AddActor(a41);
  renWin->AddRenderer(ren41);

  // Invert the transform
  vtkSmartPointer<vtkTransformPolyDataFilter> f42 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  f42->SetInputConnection(sphere->GetOutputPort());
  f42->SetTransform(t4->GetInverse());

  vtkSmartPointer<vtkPolyDataMapper> m42 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  m42->SetInputConnection(f42->GetOutputPort());

  vtkSmartPointer<vtkActor> a42 =
    vtkSmartPointer<vtkActor>::New();
  a42->SetMapper(m42);
  a42->RotateY(90);
  a42->GetProperty()->SetColor(0.9,0.9,0);

  vtkSmartPointer<vtkRenderer> ren42 =
    vtkSmartPointer<vtkRenderer>::New();
  ren42->SetViewport(0.75,0.0,1.0,0.5);
  ren42->ResetCamera(-0.5,0.5,-0.5,0.5,-1,1);
  ren42->AddActor(a42);
  renWin->AddRenderer(ren42);

  //you MUST NOT call renderWindow->Render() before
  //iren->SetRenderWindow(renderWindow);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  //render and interact
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
