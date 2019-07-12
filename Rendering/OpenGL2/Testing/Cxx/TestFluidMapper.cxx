/*=========================================================================

 Program:   Visualization Toolkit
 Module:    TestSprites.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLFluidMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSkybox.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"


int TestFluidMapper(int argc, char *argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete [] fileName;

  vtkNew<vtkPolyDataMapper> dragonMapper;
  dragonMapper->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkActor> dragon;
  dragon->SetMapper(dragonMapper);
  dragon->SetScale(20,20,20);
  dragon->SetPosition(2, -0.5, 3);
  renderer->AddActor(dragon);

  vtkNew<vtkTexture> texture;
  texture->InterpolateOn();
  const char * fName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wintersun.jpg");
  vtkNew<vtkJPEGReader> imgReader;
  imgReader->SetFileName(fName);
  texture->SetInputConnection(imgReader->GetOutputPort());
  delete [] fName;

  vtkNew<vtkSkybox> world;
  world->SetProjectionToSphere();
  world->SetTexture(texture);
  world->ForceOpaqueOn();
  renderer->AddActor(world);

  vtkNew<vtkPolyData> pd;
  vtkNew<vtkPoints> points;
  pd->SetPoints(points);
  for (int z = 0; z < 50; ++z)
  {
    for (int y = 0; y < 20; ++y)
    {
      for (int x = 0; x < 50; ++x)
      {
        points->InsertNextPoint(x*0.1, y*0.1, z*0.1);
      }
    }
  }

  vtkNew<vtkOpenGLFluidMapper> mapper;
  mapper->SetInputData(pd);
  mapper->SetRadius(0.12);

  vtkNew<vtkVolume> vol;
  vol->SetMapper(mapper);
  renderer->AddVolume(vol);

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetViewAngle(70.0);
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 1;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(1);
    renderer->GetActiveCamera()->Elevation(1);
    renderWindow->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();

  cerr << "interactive render time: " << elapsed / numRenders << endl;

  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetViewAngle(70.0);
  renderer->GetActiveCamera()->Dolly(3.0);
  renderer->ResetCameraClippingRange();

  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
