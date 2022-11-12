/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRenderWindowDifferentThread.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include <future>
#include <sstream>
#include <string>
#include <thread>

int Start(int argc, char* argv[])
{
  vtkLogger::SetThreadName("Render Thread");
  std::hash<std::thread::id> tid_hash{};
  auto tid = tid_hash(std::this_thread::get_id());
  vtkLog(INFO, << "Rendering on " << tid);
  // Create a cylinder
  vtkNew<vtkCylinderSource> cylinderSource;
  cylinderSource->SetCenter(0.0, 0.0, 0.0);
  cylinderSource->SetRadius(5.0);
  cylinderSource->SetHeight(10.0);
  cylinderSource->SetResolution(100);

  // Create a mapper and actor
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cylinderSource->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetColor(1.0, 0.38, 0.278);
  actor->SetMapper(mapper);
  actor->RotateX(30.0);
  actor->RotateY(-45.0);

  // Create a renderer
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.1, 0.2, 0.4);
  renderer->ResetCamera();

  // Create a render window initialized for offscreen rendering. you won't see it.
  vtkNew<vtkRenderWindow> renWin;
  renWin->OffScreenRenderingOn();
  renWin->AddRenderer(renderer);
  renWin->SetSize(1920, 1080);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  return !retVal;
}

int TestRenderWindowDifferentThread(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_INFO);
  std::future<int> fut = std::async(std::launch::async, &Start, argc, argv);
  std::hash<std::thread::id> tid_hash{};
  auto tid = tid_hash(std::this_thread::get_id());
  vtkLog(INFO, << "Main thread " << tid);
  int result = vtkRegressionTester::FAILED;
  result = fut.get();
  vtkLog(INFO, << "result=" << result);
  return result;
}
