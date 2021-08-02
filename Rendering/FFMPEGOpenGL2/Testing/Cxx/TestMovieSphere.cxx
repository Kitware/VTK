/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFFMPEGVideoSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkOpenGLMovieSphere.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"

#include "vtkFFMPEGVideoSource.h"

int TestMovieSphere(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkOpenGLMovieSphere> actor;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tracktor.webm");

  vtkNew<vtkFFMPEGVideoSource> video;
  video->SetFileName(fileName);
  delete[] fileName;

  actor->SetVideoSource(video);
  actor->SetProjectionToSphere();

  video->Record();

  renderWindow->Render();

  vtkNew<vtkTimerLog> timer;
  auto startTime = timer->GetUniversalTime();
  while (timer->GetUniversalTime() - startTime < 12.0 && !video->GetEndOfFile())
  {
    renderWindow->Render();
  }

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // last frame may differ as it is a real time playback so we just test
  // execution to get runtime errors, asan, ubsan etc
  return EXIT_SUCCESS;
}
