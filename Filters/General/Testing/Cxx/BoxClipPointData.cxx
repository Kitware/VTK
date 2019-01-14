/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>
#include <vtkBoxClipDataSet.h>
#include <vtkLookupTable.h>

#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>

#include <vtkDataSetSurfaceFilter.h>
#include <vtkDataSetMapper.h>
#include <vtkCamera.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

#include "vtkTestUtilities.h"

int BoxClipPointData(int argc, char *argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/hexa.vtk");

  // Read the data
  vtkSmartPointer<vtkUnstructuredGridReader> reader =
    vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName (fileName);
  reader->Update();
  delete [] fileName;

  double bounds[6];
  reader->GetOutput()->GetBounds(bounds);

  double range[2];
  reader->GetOutput()->GetScalarRange(range);

  double minBoxPoint[3];
  double maxBoxPoint[3];
  minBoxPoint[0] = (bounds[1] - bounds[0]) / 2.0 + bounds[0];
  minBoxPoint[1] = (bounds[3] - bounds[2]) / 2.0 + bounds[2];
  minBoxPoint[2] = (bounds[5] - bounds[4]) / 2.0 + bounds[4];
  maxBoxPoint[0] = bounds[1];
  maxBoxPoint[1] = bounds[3];
  maxBoxPoint[2] = bounds[5];

  vtkSmartPointer<vtkBoxClipDataSet> boxClip =
    vtkSmartPointer<vtkBoxClipDataSet>::New();
  boxClip->SetInputConnection (reader->GetOutputPort());
  boxClip->SetBoxClip(minBoxPoint[0], maxBoxPoint[0],
                      minBoxPoint[1], maxBoxPoint[1],
                      minBoxPoint[2], maxBoxPoint[2]);
  boxClip->GenerateClippedOutputOn();

  // Define a lut
  vtkSmartPointer<vtkLookupTable> lut1 =
    vtkSmartPointer<vtkLookupTable>::New();
  lut1->SetHueRange (.667, 0);
  lut1->Build();

  vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceIn =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfaceIn->SetInputConnection (boxClip->GetOutputPort(0));

  vtkSmartPointer<vtkDataSetMapper> mapperIn =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapperIn->SetInputConnection(surfaceIn->GetOutputPort());
  mapperIn->SetScalarRange(reader->GetOutput()->GetScalarRange());
  mapperIn->SetLookupTable(lut1);

  vtkSmartPointer<vtkActor> actorIn =
    vtkSmartPointer<vtkActor>::New();
  actorIn->SetMapper(mapperIn);

  vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceOut =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfaceOut->SetInputConnection (boxClip->GetOutputPort(1));

  vtkSmartPointer<vtkDataSetMapper> mapperOut =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapperOut->SetInputConnection(surfaceOut->GetOutputPort());
  mapperOut->SetScalarRange(reader->GetOutput()->GetScalarRange());
  mapperOut->SetLookupTable(lut1);

  vtkSmartPointer<vtkActor> actorOut =
    vtkSmartPointer<vtkActor>::New();
  actorOut->SetMapper(mapperOut);
  actorOut->AddPosition(-.5 * (maxBoxPoint[0] - minBoxPoint[0]),
                        -.5 * (maxBoxPoint[1] - minBoxPoint[1]),
                        -.5 * (maxBoxPoint[2] - minBoxPoint[2]));

  // Create a renderer, render window, and interactor
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground (.5, .5, .5);
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->AddActor(actorIn);
  renderer->AddActor(actorOut);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(120);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->GetActiveCamera()->Dolly(1.0);
  renderer->ResetCameraClippingRange();

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
