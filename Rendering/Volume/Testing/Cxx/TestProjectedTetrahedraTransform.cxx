/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProjectedTetrahedraTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkConeSource.h>
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProjectedTetrahedraMapper.h>
#include <vtkProp3D.h>
#include <vtkProperty.h>
#include <vtkRectilinearGrid.h>
#include <vtkRectilinearGridToTetrahedra.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTesting.h>
#include <vtkTransform.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVolumeProperty.h>

// Creates a cube volume
vtkSmartPointer<vtkVolume> CubeVolume(double r, double g, double b)
{
  // Create the coordinates
  vtkNew<vtkDoubleArray> xArray;
  xArray->InsertNextValue(0.0);
  xArray->InsertNextValue(1.0);
  vtkNew<vtkDoubleArray> yArray;
  yArray->InsertNextValue(0.0);
  yArray->InsertNextValue(1.0);
  vtkNew<vtkDoubleArray> zArray;
  zArray->InsertNextValue(0.0);
  zArray->InsertNextValue(1.0);

  // Create the RectilinearGrid
  vtkNew<vtkRectilinearGrid> grid;
  grid->SetDimensions(2, 2, 2);
  grid->SetXCoordinates(xArray.GetPointer());
  grid->SetYCoordinates(yArray.GetPointer());
  grid->SetZCoordinates(zArray.GetPointer());

  // Obtain an UnstructuredGrid made of tetrahedras
  vtkNew<vtkRectilinearGridToTetrahedra> rectilinearGridToTetrahedra;
  rectilinearGridToTetrahedra->SetInputData(grid.GetPointer());
  rectilinearGridToTetrahedra->Update();

  vtkSmartPointer<vtkUnstructuredGrid> ugrid
    = rectilinearGridToTetrahedra->GetOutput();

  // Add scalars to the grid
  vtkNew<vtkDoubleArray> scalars;
  for (int i = 0; i < 8; i++)
  {
    scalars->InsertNextValue(0);
  }
  ugrid->GetPointData()->SetScalars(scalars.GetPointer());

  // Volume Rendering Mapper
  vtkNew<vtkProjectedTetrahedraMapper> mapper;
  mapper->SetInputData(ugrid);
  mapper->Update();

  // Create the volume
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(mapper.GetPointer());

  // Apply a ColorTransferFunction to the volume
  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(0.0, r, g, b);
  volume->GetProperty()->SetColor(colorTransferFunction.GetPointer());

  return volume;
}

// Creates a cone actor
vtkSmartPointer<vtkActor> ConeActor(double r, double g, double b)
{
  // Simple cone mapper
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkConeSource> coneSource;
  coneSource->SetCenter(0.0, 0.0, 0.0);
  mapper->SetInputConnection(coneSource->GetOutputPort());

  // Create the actor
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->GetProperty()->SetColor(r, g, b);
  actor->SetMapper(mapper.GetPointer());

  return actor;
}


int TestProjectedTetrahedraTransform(int argc, char *argv[])
{
  // Create the props

  // The red cube volume
  vtkSmartPointer<vtkProp3D> volume1 = CubeVolume(1, 0, 0);

  // The blue cube volume
  vtkSmartPointer<vtkProp3D> volume2 = CubeVolume(0, 0, 1);

  // The red cone actor
  vtkSmartPointer<vtkProp3D> actor1 = ConeActor(1, 0, 0);

  // The blue cone actor
  vtkSmartPointer<vtkProp3D> actor2 = ConeActor(0, 0, 1);

  // Translate the blue props by (2,2)
  vtkNew<vtkTransform> transform;
  transform->Translate(2, 2, 0);
  volume2->SetUserTransform(transform.GetPointer());
  actor2->SetUserTransform(transform.GetPointer());

  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  // Add the props to the scene
  renderer->AddVolume(volume1);
  renderer->AddVolume(volume2);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  renderer->SetBackground(1, 1, 1);

  // Render and interact
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkTesting::Test(argc, argv, renderWindow.GetPointer(), 20);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
