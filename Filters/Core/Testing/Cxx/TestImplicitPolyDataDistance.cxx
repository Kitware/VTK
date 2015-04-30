/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPolyDataDistance.cxx

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
#include "vtkImplicitPolyDataDistance.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkXMLPolyDataReader.h"

#include <vector>

int TestImplicitPolyDataDistance(int argc, char* argv[])
{
  vtkSmartPointer<vtkTesting> testHelper =
    vtkSmartPointer<vtkTesting>::New();
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
    {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
    }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string fileName = dataRoot + "/Data/CuspySurface.vtp";
  std::cout << fileName << std::endl;

  // Set up reader
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();

  // Set up distance calculator
  vtkNew<vtkImplicitPolyDataDistance> implicitDistance;
  implicitDistance->SetInput(reader->GetOutput());

  // Compute distances to test points, saving those within the cuspy surface for display
  vtkNew<vtkPoints> points;
  double xRange[2] = {-47.6, 46.9};
  double yRange[2] = {-18.2, 82.1};
  double zRange[2] = {1.63, 102};
  const double spacing = 10.0;
  for (double z = zRange[0]; z < zRange[1]; z += spacing)
    {
    for (double y = yRange[0]; y < yRange[1]; y += spacing)
      {
      for (double x = xRange[0]; x < xRange[1]; x += spacing)
        {
        double pt[3] = {x, y, z};
        double distance = implicitDistance->EvaluateFunction(pt);
        if (distance <= 0.0)
          {
          points->InsertNextPoint(pt);
          }
        }
      }
    }

  // Set up the point data structure
  vtkNew<vtkPolyData> insidePoints;
  insidePoints->SetPoints(points.GetPointer());

  // Glyph the points
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(3);
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetInputData(insidePoints.GetPointer());
  glypher->SetSourceConnection(sphere->GetOutputPort());

  // Display the glyphs
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(glypher->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Display the bounding surface
  vtkNew<vtkPolyDataMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper.GetPointer());
  surfaceActor->GetProperty()->FrontfaceCullingOn();

  // Standard rendering classes
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renderer->AddActor(actor.GetPointer());
  renderer->AddActor(surfaceActor.GetPointer());

  // Standard testing code.
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(300,300);

  vtkCamera *camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Azimuth(30);
  camera->Elevation(-20);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
