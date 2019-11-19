/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitProjectOnPlaneDistance

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitProjectOnPlaneDistance.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkType.h"
#include "vtkXMLPolyDataReader.h"

#include <vector>

int TestImplicitProjectOnPlaneDistance(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CuspySurface.vtp");
  std::cout << fileName << std::endl;

  // Set up reader
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;
  reader->Update();
  vtkPolyData* pd = vtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());

  vtkNew<vtkPlaneSource> plane;
  plane->SetOrigin(0, 0, -1);
  plane->SetPoint1(-30, -10, -1);
  plane->SetPoint2(30, 50, -1);
  plane->Update();

  // Set up distance calculator
  vtkNew<vtkImplicitProjectOnPlaneDistance> implicitDistance;
  implicitDistance->SetInput(plane->GetOutput());

  // Compute distances to test points, saving those below the cuspy surface for display
  vtkNew<vtkPoints> insidePoints;
  const vtkIdType pdNbPoints = pd->GetNumberOfPoints();
  for (vtkIdType i = 0; i < pdNbPoints; i++)
  {
    double point[3];
    pd->GetPoint(i, point);
    double distance = implicitDistance->EvaluateFunction(point);
    if (distance <= 0.0)
    {
      insidePoints->InsertNextPoint(point);
    }
  }

  // Set up inside points data structure
  vtkNew<vtkPolyData> insidePointsPolyData;
  insidePointsPolyData->SetPoints(insidePoints);

  // Glyph the points
  vtkNew<vtkSphereSource> insidePointSphere;
  insidePointSphere->SetRadius(3);
  vtkNew<vtkGlyph3D> insidePointsGlypher;
  insidePointsGlypher->SetInputData(insidePointsPolyData);
  insidePointsGlypher->SetSourceConnection(insidePointSphere->GetOutputPort());

  // Display the glyphs
  vtkNew<vtkPolyDataMapper> insidePointMapper;
  insidePointMapper->SetInputConnection(insidePointsGlypher->GetOutputPort());

  vtkNew<vtkActor> insidePointActor;
  insidePointActor->SetMapper(insidePointMapper);
  insidePointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Display the glyphs
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetColor(0.0, 0.0, 1.0);

  // Display the bounding surface
  vtkNew<vtkPolyDataMapper> surfaceMapper;
  surfaceMapper->SetInputData(pd);

  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->FrontfaceCullingOn();

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(insidePointActor);
  renderer->AddActor(planeActor);
  renderer->AddActor(surfaceActor);

  // Standard testing code.
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  vtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Azimuth(60);
  camera->Elevation(-10);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
