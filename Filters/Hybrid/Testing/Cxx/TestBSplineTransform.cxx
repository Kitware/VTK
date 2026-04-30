// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

#include <vtkActor.h>
#include <vtkBSplineTransform.h>
#include <vtkImageBSplineCoefficients.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformToGrid.h>

int TestBSplineTransform(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);

  // Make a sphere
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);
  sphere->Update();

  // Make another sphere, with no normals
  vtkNew<vtkPolyData> sphereData;
  sphereData->SetPoints(sphere->GetOutput()->GetPoints());
  sphereData->SetPolys(sphere->GetOutput()->GetPolys());

  // ---------------------------
  // start with a thin plate spline transform
  vtkNew<vtkPoints> spoints;
  spoints->SetNumberOfPoints(10);
  spoints->SetPoint(0, 0.000, 0.000, 0.500);
  spoints->SetPoint(1, 0.000, 0.000, -0.500);
  spoints->SetPoint(2, 0.433, 0.000, 0.250);
  spoints->SetPoint(3, 0.433, 0.000, -0.250);
  spoints->SetPoint(4, -0.000, 0.433, 0.250);
  spoints->SetPoint(5, -0.000, 0.433, -0.250);
  spoints->SetPoint(6, -0.433, -0.000, 0.250);
  spoints->SetPoint(7, -0.433, -0.000, -0.250);
  spoints->SetPoint(8, 0.000, -0.433, 0.250);
  spoints->SetPoint(9, 0.000, -0.433, -0.250);

  vtkNew<vtkPoints> tpoints;
  tpoints->SetNumberOfPoints(10);
  tpoints->SetPoint(0, 0.000, 0.000, 0.800);
  tpoints->SetPoint(1, 0.000, 0.000, -0.200);
  tpoints->SetPoint(2, 0.433, 0.000, 0.350);
  tpoints->SetPoint(3, 0.433, 0.000, -0.150);
  tpoints->SetPoint(4, -0.000, 0.233, 0.350);
  tpoints->SetPoint(5, -0.000, 0.433, -0.150);
  tpoints->SetPoint(6, -0.433, -0.000, 0.350);
  tpoints->SetPoint(7, -0.433, -0.000, -0.150);
  tpoints->SetPoint(8, 0.000, -0.233, 0.350);
  tpoints->SetPoint(9, 0.000, -0.433, -0.150);

  vtkNew<vtkThinPlateSplineTransform> thin;
  thin->SetSourceLandmarks(spoints);
  thin->SetTargetLandmarks(tpoints);
  thin->SetBasisToR2LogR();

  // First pane: thin-plate, no normals
  vtkNew<vtkTransformFilter> f11;
  f11->SetInputData(sphereData);
  f11->SetTransform(thin);

  vtkNew<vtkPolyDataMapper> m11;
  m11->SetInputConnection(f11->GetOutputPort());

  vtkNew<vtkActor> a11;
  a11->SetMapper(m11);
  a11->RotateY(90);
  a11->GetProperty()->SetColor(1, 0, 0);

  vtkNew<vtkRenderer> ren11;
  ren11->SetViewport(0.0, 0.5, 0.25, 1.0);
  ren11->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren11->AddActor(a11);
  renWin->AddRenderer(ren11);

  // Invert the transform
  vtkNew<vtkTransformFilter> f12;
  f12->SetInputData(sphereData);
  f12->SetTransform(thin->GetInverse());

  vtkNew<vtkPolyDataMapper> m12;
  m12->SetInputConnection(f12->GetOutputPort());

  vtkNew<vtkActor> a12;
  a12->SetMapper(m12);
  a12->RotateY(90);
  a12->GetProperty()->SetColor(0.9, 0.9, 0);

  vtkNew<vtkRenderer> ren12;
  ren12->SetViewport(0.0, 0.0, 0.25, 0.5);
  ren12->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren12->AddActor(a12);
  renWin->AddRenderer(ren12);

  // Second pane: b-spline transform, no normals
  vtkNew<vtkTransformToGrid> transformToGrid;
  transformToGrid->SetInput(thin);
  transformToGrid->SetGridOrigin(-1.5, -1.5, -1.5);
  transformToGrid->SetGridExtent(0, 60, 0, 60, 0, 60);
  transformToGrid->SetGridSpacing(0.05, 0.05, 0.05);

  vtkNew<vtkImageBSplineCoefficients> coeffs;
  coeffs->SetInputConnection(transformToGrid->GetOutputPort());

  vtkNew<vtkBSplineTransform> t2;
  t2->SetCoefficientConnection(coeffs->GetOutputPort());

  vtkNew<vtkTransformFilter> f21;
  f21->SetInputData(sphereData);
  f21->SetTransform(t2);

  vtkNew<vtkPolyDataMapper> m21;
  m21->SetInputConnection(f21->GetOutputPort());

  vtkNew<vtkActor> a21;
  a21->SetMapper(m21);
  a21->RotateY(90);
  a21->GetProperty()->SetColor(1, 0, 0);

  vtkNew<vtkRenderer> ren21;
  ren21->SetViewport(0.25, 0.5, 0.50, 1.0);
  ren21->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren21->AddActor(a21);
  renWin->AddRenderer(ren21);

  // Invert the transform
  vtkNew<vtkTransformFilter> f22;
  f22->SetInputData(sphereData);
  f22->SetTransform(t2->GetInverse());

  vtkNew<vtkPolyDataMapper> m22;
  m22->SetInputConnection(f22->GetOutputPort());

  vtkNew<vtkActor> a22;
  a22->SetMapper(m22);
  a22->RotateY(90);
  a22->GetProperty()->SetColor(0.9, 0.9, 0);

  vtkNew<vtkRenderer> ren22;
  ren22->SetViewport(0.25, 0.0, 0.50, 0.5);
  ren22->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren22->AddActor(a22);
  renWin->AddRenderer(ren22);

  // Third pane: thin-plate, no normals
  vtkNew<vtkTransformFilter> f31;
  f31->SetInputConnection(sphere->GetOutputPort());
  f31->SetTransform(thin);

  vtkNew<vtkPolyDataMapper> m31;
  m31->SetInputConnection(f31->GetOutputPort());

  vtkNew<vtkActor> a31;
  a31->SetMapper(m31);
  a31->RotateY(90);
  a31->GetProperty()->SetColor(1, 0, 0);

  vtkNew<vtkRenderer> ren31;
  ren31->SetViewport(0.50, 0.5, 0.75, 1.0);
  ren31->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren31->AddActor(a31);
  renWin->AddRenderer(ren31);

  // Invert the transform
  vtkNew<vtkTransformFilter> f32;
  f32->SetInputConnection(sphere->GetOutputPort());
  f32->SetTransform(thin->GetInverse());

  vtkNew<vtkPolyDataMapper> m32;
  m32->SetInputConnection(f32->GetOutputPort());

  vtkNew<vtkActor> a32;
  a32->SetMapper(m32);
  a32->RotateY(90);
  a32->GetProperty()->SetColor(0.9, 0.9, 0);

  vtkNew<vtkRenderer> ren32;
  ren32->SetViewport(0.5, 0.0, 0.75, 0.5);
  ren32->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren32->AddActor(a32);
  renWin->AddRenderer(ren32);

  // Third pane: b-spline, normals
  vtkNew<vtkBSplineTransform> t4;
  t4->SetCoefficientConnection(coeffs->GetOutputPort());

  vtkNew<vtkTransformFilter> f41;
  f41->SetInputConnection(sphere->GetOutputPort());
  f41->SetTransform(t4);

  vtkNew<vtkPolyDataMapper> m41;
  m41->SetInputConnection(f41->GetOutputPort());

  vtkNew<vtkActor> a41;
  a41->SetMapper(m41);
  a41->RotateY(90);
  a41->GetProperty()->SetColor(1, 0, 0);

  vtkNew<vtkRenderer> ren41;
  ren41->SetViewport(0.75, 0.5, 1.0, 1.0);
  ren41->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren41->AddActor(a41);
  renWin->AddRenderer(ren41);

  // Invert the transform
  vtkNew<vtkTransformFilter> f42;
  f42->SetInputConnection(sphere->GetOutputPort());
  f42->SetTransform(t4->GetInverse());

  vtkNew<vtkPolyDataMapper> m42;
  m42->SetInputConnection(f42->GetOutputPort());

  vtkNew<vtkActor> a42;
  a42->SetMapper(m42);
  a42->RotateY(90);
  a42->GetProperty()->SetColor(0.9, 0.9, 0);

  vtkNew<vtkRenderer> ren42;
  ren42->SetViewport(0.75, 0.0, 1.0, 0.5);
  ren42->ResetCamera(-0.5, 0.5, -0.5, 0.5, -1, 1);
  ren42->AddActor(a42);
  renWin->AddRenderer(ren42);

  // you MUST NOT call renderWindow->Render() before
  // iren->SetRenderWindow(renderWindow);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // render and interact
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
