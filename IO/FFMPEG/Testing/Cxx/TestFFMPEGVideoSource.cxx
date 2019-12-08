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
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"

#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"

#include "vtkLookupTable.h"

#include "vtkFFMPEGVideoSource.h"

int TestFFMPEGVideoSource(int argc, char* argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->SetBackground(0.2, 0.3, 0.4);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tracktor.webm");

  vtkNew<vtkFFMPEGVideoSource> video;
  video->SetFileName(fileName);
  delete[] fileName;

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(video->GetOutputPort());
  actor->SetTexture(texture);

  vtkNew<vtkPlaneSource> plane;
  mapper->SetInputConnection(plane->GetOutputPort());
  actor->SetMapper(mapper);

  video->Initialize();
  int fsize[3];
  video->GetFrameSize(fsize);
  plane->SetOrigin(0, 0, 0);
  plane->SetPoint1(fsize[0], 0, 0);
  plane->SetPoint2(0, fsize[1], 0);
  renderWindow->Render();

  for (int i = 0; i < 10; ++i)
  {
    video->Grab();
    renderWindow->Render();
  }

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
