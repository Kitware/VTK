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
#include <vtkSmartPointer.h>
#include <vtkConeSource.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>

//This tests if the offscreen rendering works
static bool TestWin32OpenGLRenderWindowOffScreen(vtkWin32OpenGLRenderWindow* renderWindow)
{
  //Create a cone
  vtkSmartPointer<vtkConeSource> coneSource =
    vtkSmartPointer<vtkConeSource>::New();
  coneSource->Update();
 
  //Create a mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(coneSource->GetOutputPort());
 
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
 
  //Create a renderer, render window, and interactor
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  renderWindow->AddRenderer(renderer);
 
  //Add the actors to the scene
  renderer->AddActor(actor);
  renderer->SetBackground(.3, .2, .1); // Background color dark red
 
  //Render and interact
  renderWindow->SetOffScreenRendering(1);
  renderWindow->SetSize(100,100);
  renderWindow->Render(); 
  return 1;
}
int TestWin32OpenGLRenderWindow(int argc, char* argv[])
{
  vtkWin32OpenGLRenderWindow* renderWindow(NULL);

  vtkSmartPointer<vtkRenderWindow> renderWindowBase = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow = vtkWin32OpenGLRenderWindow::SafeDownCast(renderWindowBase);

  TestWin32OpenGLRenderWindowOffScreen(renderWindow);

  return EXIT_SUCCESS;
}
