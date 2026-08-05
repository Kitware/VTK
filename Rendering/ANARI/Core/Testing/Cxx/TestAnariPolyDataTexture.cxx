// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkJPEGReader.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test covers testing of actor texturing for polydata geometry. It renders a textured cube.
 */
int TestAnariPolyDataTexture(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  // Read an image to use as texture
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");

  vtkNew<vtkJPEGReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  // Create a plane source
  vtkNew<vtkPlaneSource> plane;
  plane->SetXResolution(2);        // 3 points = 2 subdivisions
  plane->SetYResolution(4);        // 5 points = 4 subdivisions
  plane->SetOrigin(0.0, 0.0, 0.0); // Lower left corner
  plane->SetPoint1(1.0, 0.0, 0.0); // Lower right corner
  plane->SetPoint2(0.0, 1.0, 0.0); // Upper left corner
  plane->Update();

  // Get the output points
  vtkPolyData* output = plane->GetOutput();
  vtkPoints* points = output->GetPoints();
  vtkIdType num_points = points->GetNumberOfPoints();

  // Create custom texture coordinates
  vtkNew<vtkFloatArray> tex_coords;
  tex_coords->SetNumberOfComponents(2);
  tex_coords->SetNumberOfTuples(num_points);
  tex_coords->SetName("TextureCoordinates");

  for (vtkIdType i = 0; i < num_points; i++)
  {
    double p[3];
    points->GetPoint(i, p);
    float tc[2];
    tc[0] = static_cast<float>(p[0]) * 2.5 - 1.3; // U coordinate goes from -1.3 to 1.2
    tc[1] = static_cast<float>(p[1]) - 0.5;       // V coordinate goes from -0.5 to 0.5
    tex_coords->SetTuple(i, tc);
  }

  output->GetPointData()->SetTCoords(tex_coords);

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(output);

  // Texture
  vtkNew<vtkTexture> texture;
  texture->SetWrap(vtkTexture::Repeat);
  texture->InterpolateOn();
  texture->MipmapOn();
  texture->SetInputConnection(reader->GetOutputPort());

  // Actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetTexture(texture);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.1, 0.2, 0.4);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Dolly(1.5);
  renderer->ResetCameraClippingRange();

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariPolyDataTexture");
  vtkAnariRenderer* anariRenderer = vtkAnariRenderWindow::SafeDownCast(renWin)->GetAnariRenderer();
  if (anariRenderer)
  {
    anariRenderer->SetParameteri("pixelSamples", 5);
    anariRenderer->SetParameteri("ambientSamples", 1);
  }

  int retVal = vtkRegressionTestImage(renWin);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;

  return 0;
}
