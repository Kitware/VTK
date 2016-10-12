/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestWin32OpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"

int TestWin32OpenGLRenderWindow(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  if (!vtkWin32OpenGLRenderWindow::SafeDownCast(renWin.GetPointer()))
  {
    std::cout << "Expected vtkRenderWindow to be a vtkWin32OpenGLRenderWindow"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Set multisamples to 0 to allow using
  // vtkOpenGLRenderWindow::CreateHardwareOffScreenWindow() implementation
  // (see check near top of function)
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkConeSource> coneSource;
  coneSource->Update();

  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper.GetPointer());

  renderer->AddActor(coneActor.GetPointer());

  int width = 100;
  int height = 75;

  float scale = 4.0f;
  int scaledWidth = width * scale;
  int scaledHeight = height * scale;

  renderer->SetGradientBackground(1);
  renderer->SetBackground(0.0, 0.37, 0.62);
  renderer->SetBackground2(0.0, 0.62, 0.29);
  renWin->SetSize(width, height);
  renWin->Render();

  // Render offscreen at a larger size
  renWin->SetOffScreenRendering(1);
  renWin->SetSize(scaledWidth, scaledHeight);

  int* screenSize = renWin->GetScreenSize();
  if (screenSize[0] != scaledWidth || screenSize[1] != scaledHeight)
  {
    std::cout << "Expected calling vtkWin32OpenGLRenderWindow::GetScreenSize()"
                 " not to change render window size"
              << std::endl;
    return EXIT_FAILURE;
  }

  int* windowSize = renWin->GetSize();
  if (windowSize[0] != scaledWidth || windowSize[1] != scaledHeight)
  {
    std::cout << "Expected calling vtkWin32OpenGLRenderWindow::GetScreenSize()"
                 " not to change render window size"
              << std::endl;
    return EXIT_FAILURE;
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
