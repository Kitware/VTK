/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGaussianBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test ... TO DO
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

//#include "vtkTestUtilities.h"
//#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCameraPass.h"
#include "vtkCellArray.h"
#include "vtkElevationFilter.h"
#include "vtkInformation.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkValuePass.h"


int TestValuePass(int argc, char *argv[])
{
  bool interactive = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
    }
  }

  // 0. Prep data
  const char *fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkSmartPointer<vtkPLYReader> reader =
    vtkSmartPointer<vtkPLYReader>::New();
  reader->SetFileName(fileName);
  reader->Update();

  cerr << reader->GetOutput()->GetBounds()[0] << ", "
       << reader->GetOutput()->GetBounds()[1] << endl;

  vtkSmartPointer<vtkElevationFilter> elevation =
    vtkSmartPointer<vtkElevationFilter>::New();
  elevation->SetInputConnection(reader->GetOutputPort());
  elevation->SetLowPoint(-0.11, 0.0, 0.0);
  elevation->SetHighPoint(0.1, 0.0, 0.0);
  elevation->Update();

  vtkPolyData *polyData = vtkPolyData::SafeDownCast(elevation->GetOutput());
  vtkIntArray *array = vtkIntArray::New();
  array->SetName("TestArray");
  array->SetNumberOfComponents(1);
  for (int i = 0; i < polyData->GetNumberOfPoints(); ++i)
  {
    array->InsertNextValue(i);
  }
  polyData->GetPointData()->AddArray(array);
  array->Delete();

  vtkSmartPointer<vtkPointDataToCellData> pointsToCells =
    vtkSmartPointer<vtkPointDataToCellData>::New();
  pointsToCells->SetInputData(polyData);

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(pointsToCells->GetOutputPort());
  mapper->ScalarVisibilityOn();

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // 1. Set up renderer, window, & interactor
  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkRenderWindow> window =
    vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  window->AddRenderer(renderer);
  interactor->SetRenderWindow(window);

  renderer->AddActor(actor);

  // 2. Set up rendering passes
  vtkSmartPointer<vtkValuePass> valuePass =
    vtkSmartPointer<vtkValuePass>::New();
  valuePass->SetInputArrayToProcess(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA, 0);
  valuePass->SetInputComponentToProcess(0);
  valuePass->SetScalarRange(-0.11, 0.1);

  vtkSmartPointer<vtkRenderPassCollection> passes =
    vtkSmartPointer<vtkRenderPassCollection>::New();
  passes->AddItem(valuePass);

  vtkSmartPointer<vtkSequencePass> sequence =
    vtkSmartPointer<vtkSequencePass>::New();
  sequence->SetPasses(passes);

  vtkSmartPointer<vtkCameraPass> cameraPass =
    vtkSmartPointer<vtkCameraPass>::New();
  cameraPass->SetDelegatePass(sequence);

  vtkOpenGLRenderer *glRenderer =
    vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());
  glRenderer->SetPass(cameraPass);

  // 3. Render image and compare against baseline
  for (int i = 0; i < 3; ++i)
  {
    if (i % 2 == 0)
    {
      glRenderer->SetPass(NULL);
    }
    else
    {
      glRenderer->SetPass(cameraPass);
    }
    window->Render();
  }

  if (interactive)
  {
    interactor->Start();
  }

  return 0;
}
