// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderingOpenGLConfigure.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include <iostream>
#include <thread>

namespace
{
void AddSphere(vtkRenderWindow* window, double background)
{
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(background, background, background);
  window->AddRenderer(ren);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort(0));
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  ren->ResetCamera();
}
}

//------------------------------------------------------------------------------
// Test one can create an resize offscreen render windows.
int TestOffscreenRenderingResize(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetShowWindow(false);
  window->SetUseOffScreenBuffers(true);
  window->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(window);

  AddSphere(window, 0.3);
  window->Render();

  window->SetSize(400, 300);
  window->Render();
  int retVal = vtkRegressionTestImage(window);

#ifdef VTK_USE_COCOA
  // An offscreen window renders without an NSView, so it also renders on a
  // thread other than the main thread, where AppKit refuses to make one.
  vtkNew<vtkRenderWindow> threaded;
  threaded->SetShowWindow(false);
  threaded->SetUseOffScreenBuffers(true);
  threaded->SetSize(64, 64);
  AddSphere(threaded, 0.0);

  bool sphereDrawn = false;
  std::thread worker(
    [&]
    {
      threaded->Render();
      unsigned char* center = threaded->GetPixelData(32, 32, 32, 32, 0);
      sphereDrawn = center[0] != 0 || center[1] != 0 || center[2] != 0;
      delete[] center;
    });
  worker.join();
  if (!sphereDrawn)
  {
    std::cerr << "The sphere was not rendered on a thread." << std::endl;
    return EXIT_FAILURE;
  }

  // Showing such a window gives it an NSWindow and NSView, which AppKit allows
  // creating only here, on the main thread.
  threaded->SetShowWindow(true);
  if (!threaded->GetGenericWindowId())
  {
    std::cerr << "The window was not given an NSView when shown." << std::endl;
    return EXIT_FAILURE;
  }
  threaded->Render();
#endif

  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
