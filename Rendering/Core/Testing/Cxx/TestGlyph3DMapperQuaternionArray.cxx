/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkGlyph3DMapper.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestGlyph3DMapperQuaternionArray(int argc, char* argv[])
{
  int res = 30;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res, res);

  vtkNew<vtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);
  colors->Update();

  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(colors->GetOutputPort());

  vtkPointData* pointData = vtkDataSet::SafeDownCast(colors->GetOutput())->GetPointData();
  pointData->SetActiveScalars("Elevation");

  vtkFloatArray* elevData = vtkFloatArray::SafeDownCast(pointData->GetArray("Elevation"));

  vtkIdType nbTuples = elevData->GetNumberOfTuples();

  vtkNew<vtkFloatArray> quatData;
  quatData->SetNumberOfComponents(4);
  quatData->SetNumberOfTuples(nbTuples);
  quatData->SetName("Quaternion");

  float* elevPtr = elevData->GetPointer(0);
  float* quatPtr = quatData->GetPointer(0);

  for (vtkIdType i = 0; i < nbTuples; i++)
  {
    float angle = (*elevPtr++) * vtkMath::Pi();
    float s = sin(0.5 * angle);
    float c = cos(0.5 * angle);

    *quatPtr++ = c * c * c + s * s * s;
    *quatPtr++ = s * c * c - c * s * s;
    *quatPtr++ = c * s * c + s * c * s;
    *quatPtr++ = c * c * s - s * s * c;
  }

  pointData->AddArray(quatData);

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkConeSource> squad;
  squad->SetHeight(10.0);
  squad->SetRadius(1.0);
  squad->SetResolution(50);
  squad->SetDirection(0.0, 0.0, 1.0);

  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetOrientationArray("Quaternion");
  glypher->SetOrientationModeToQuaternion();
  glypher->SetScaleFactor(0.01);

  glypher->SetSourceConnection(squad->GetOutputPort());

  vtkNew<vtkActor> glyphActor;

  glyphActor->SetMapper(glypher);

  // Create the rendering stuff

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();

  iren->SetRenderWindow(win);

  ren->AddActor(planeActor);
  ren->AddActor(glyphActor);
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.5);

  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
