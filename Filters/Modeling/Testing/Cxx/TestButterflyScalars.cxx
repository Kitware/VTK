/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButterflyScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test that no scalar overflow occurs with ButterflySubdivision

#include "vtkSmartPointer.h"

#include "vtkButterflySubdivisionFilter.h"
#include "vtkCylinderSource.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLoopSubdivisionFilter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTriangleFilter.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
int TestButterflyScalars(int argc, char* argv[])
{
  // Defining a cylinder source.
  vtkSmartPointer<vtkCylinderSource> cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
  cylinderSource->Update();

  vtkSmartPointer<vtkTriangleFilter> triangles = vtkSmartPointer<vtkTriangleFilter>::New();
  triangles->SetInputConnection(cylinderSource->GetOutputPort());
  triangles->Update();

  vtkSmartPointer<vtkPolyData> originalMesh;
  originalMesh = triangles->GetOutput();

  vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(originalMesh->GetNumberOfPoints());
  colors->SetName("Colors");

  // Loop to select colors for each of the points in the polydata.
  for (int i = 0; i < originalMesh->GetNumberOfPoints(); i++)
  {
    if (i > 0 && i < 5)
    {
      // Black
      colors->InsertTuple3(i, 255, 255, 0);
    }
    else if (i > 4 && i < 10)
    {
      // Blue
      colors->InsertTuple3(i, 0, 0, 255);
    }
    else if (i > 9 && i < 300)
    {
      // Red
      colors->InsertTuple3(i, 255, 0, 0);
    }
    else
    {
      colors->InsertTuple3(i, 255, 0, 0);
    }
  }

  originalMesh->GetPointData()->SetScalars(colors);

  // Subdivision.
  int numberOfSubdivisions = 4;
  vtkSmartPointer<vtkButterflySubdivisionFilter> subdivisionFilter =
    vtkSmartPointer<vtkButterflySubdivisionFilter>::New();

  subdivisionFilter->SetNumberOfSubdivisions(numberOfSubdivisions);
  subdivisionFilter->SetInputData(originalMesh);
  subdivisionFilter->Update();

  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Create a mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(subdivisionFilter->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(0, 0, 0);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);
  renderWindow->Render();

  int testStatus = vtkRegressionTestImage(renderWindow);
  if (testStatus == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return (testStatus ? EXIT_SUCCESS : EXIT_FAILURE);
}
