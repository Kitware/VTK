/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVTKMClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkmClip.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDelaunay3D.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageToPoints.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

namespace {

template <typename DataSetT>
void GenerateScalars(DataSetT *dataset, bool negate)
{
  vtkIdType numPoints = dataset->GetNumberOfPoints();

  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("x+y");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(numPoints);

  double point[3];
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    dataset->GetPoint(i, point);
    scalars->SetTypedComponent(i, 0, (negate ?-point[0] - point[1]
                                             : point[0] + point[1]));
  }
  dataset->GetPointData()->SetScalars(scalars.Get());
}

} // end anon namespace

int TestVTKMClip(int, char*[])
{
  vtkNew<vtkRenderer> renderer;

  // First input is a polydata with 2D cells. This should produce a polydata
  // output from vtkmClip.
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(50);
  sphereSource->SetPhiResolution(50);
  sphereSource->Update();
  vtkPolyData *sphere = sphereSource->GetOutput();
  GenerateScalars(sphere, false);

  // Clip at zero:
  vtkNew<vtkmClip> sphereClipper;
  sphereClipper->SetInputData(sphere);
  sphereClipper->SetComputeScalars(true);
  sphereClipper->SetClipValue(0.);

  vtkNew<vtkDataSetSurfaceFilter> sphSurface;
  sphSurface->SetInputConnection(sphereClipper->GetOutputPort());

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphSurface->GetOutputPort());
  sphereMapper->SetScalarVisibility(1);
  sphereMapper->SetScalarModeToUsePointFieldData();
  sphereMapper->SelectColorArray("x+y");
  sphereMapper->SetScalarRange(0, 1);

  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());
  sphereActor->SetPosition(0.5, 0.5, 0.);
  sphereActor->RotateWXYZ(90., 0., 0., 1.);
  renderer->AddActor(sphereActor.Get());

  // Second input is an unstructured grid with 3D cells. This should produce an
  // unstructured grid output from vtkmClip.
  vtkNew<vtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(-5, 5, -5, 5, -5, 5);

  // Convert image to pointset
  vtkNew<vtkImageToPoints> imageToPoints;
  imageToPoints->SetInputConnection(imageSource->GetOutputPort());

  // Convert point set to tets:
  vtkNew<vtkDelaunay3D> tetrahedralizer;
  tetrahedralizer->SetInputConnection(imageToPoints->GetOutputPort());
  tetrahedralizer->Update();
  vtkUnstructuredGrid *tets = tetrahedralizer->GetOutput();
  GenerateScalars(tets, true);

  // Clip at zero:
  vtkNew<vtkmClip> tetClipper;
  tetClipper->SetInputData(tets);
  tetClipper->SetComputeScalars(true);
  tetClipper->SetClipValue(0.);

  vtkNew<vtkDataSetSurfaceFilter> tetSurface;
  tetSurface->SetInputConnection(tetClipper->GetOutputPort());

  vtkNew<vtkPolyDataMapper> tetMapper;
  tetMapper->SetInputConnection(tetSurface->GetOutputPort());
  tetMapper->SetScalarVisibility(1);
  tetMapper->SetScalarModeToUsePointFieldData();
  tetMapper->SelectColorArray("x+y");
  tetMapper->SetScalarRange(0, 10);

  vtkNew<vtkActor> tetActor;
  tetActor->SetMapper(tetMapper.Get());
  tetActor->SetScale(1. / 5.);
  renderer->AddActor(tetActor.Get());

  // Third dataset tests imagedata. This should produce an unstructured grid:
  vtkImageData *image = imageSource->GetOutput();
  GenerateScalars(image, false);

  vtkNew<vtkmClip> imageClipper;
  imageClipper->SetInputData(image);
  imageClipper->SetComputeScalars(true);
  imageClipper->SetClipValue(0.);

  vtkNew<vtkDataSetSurfaceFilter> imageSurface;
  imageSurface->SetInputConnection(imageClipper->GetOutputPort());

  vtkNew<vtkPolyDataMapper> imageMapper;
  imageMapper->SetInputConnection(imageSurface->GetOutputPort());
  imageMapper->SetScalarVisibility(1);
  imageMapper->SetScalarModeToUsePointFieldData();
  imageMapper->SelectColorArray("x+y");
  imageMapper->SetScalarRange(0, 10);

  vtkNew<vtkActor> imageActor;
  imageActor->SetMapper(imageMapper.Get());
  imageActor->SetScale(1. / 5.);
  imageActor->SetPosition(1.0, 1.0, 0.);
  renderer->AddActor(imageActor.Get());

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin.Get());
  renWin->AddRenderer(renderer.Get());

  renWin->SetSize(500,500);
  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
