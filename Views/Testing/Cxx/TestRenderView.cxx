/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRenderView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkCommand.h"
#include "vtkCubeSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkRenderView.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelectionLink.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <vtksys/stl/vector>
using vtksys_stl::vector;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

class TestRenderViewUpdater : public vtkCommand
{
public:
  static TestRenderViewUpdater* New()
  { return new TestRenderViewUpdater; }
  
  void AddView(vtkView* view)
  {
    this->Views.push_back(view);
    view->AddObserver(vtkCommand::SelectionChangedEvent, this);
  }
  
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    for (vector<vtkView*>::size_type i = 0; i < this->Views.size(); i++)
      {
      this->Views[i]->Update();
      }
  }
private:
  TestRenderViewUpdater() { }  
  ~TestRenderViewUpdater() { }
  vector<vtkView*> Views;
};

int TestRenderView(int argc, char* argv[])
{
  VTK_CREATE(vtkSelectionLink, link);
  VTK_CREATE(TestRenderViewUpdater, updater);
  
  VTK_CREATE(vtkSphereSource, sphere);
  VTK_CREATE(vtkCubeSource, cube);
  cube->SetCenter(2, 0, 0);
  
  VTK_CREATE(vtkTransformFilter, transform);
  VTK_CREATE(vtkTransform, trans);
  trans->Translate(0, 2, 0);
  transform->SetTransform(trans);
  transform->SetInputConnection(sphere->GetOutputPort());
  
  // Render view 1
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  VTK_CREATE(vtkRenderView, view);
  view->SetupRenderWindow(win);
  updater->AddView(view);
  
  // Sphere 1
  VTK_CREATE(vtkSurfaceRepresentation, sphereRep1);
  sphereRep1->SetInputConnection(sphere->GetOutputPort());
  sphereRep1->SetSelectionLink(link);
  view->AddRepresentation(sphereRep1);
  view->Update();
  
  // Cube 1
  VTK_CREATE(vtkSurfaceRepresentation, cubeRep1);
  cubeRep1->SetInputConnection(cube->GetOutputPort());
  view->AddRepresentation(cubeRep1);
  view->Update();

  view->GetRenderer()->ResetCamera();
  view->Update();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    // If interactive, make a second view to play with.
    
    // Render view 2
    VTK_CREATE(vtkRenderWindow, win2);
    VTK_CREATE(vtkRenderWindowInteractor, iren2);
    iren2->SetRenderWindow(win2);
    VTK_CREATE(vtkRenderView, view2);
    view2->SetupRenderWindow(win2);
    updater->AddView(view2);
    
    // Sphere 2
    VTK_CREATE(vtkSurfaceRepresentation, sphereRep2);
    sphereRep2->SetInputConnection(sphere->GetOutputPort());
    sphereRep2->SetSelectionLink(link);
    view2->AddRepresentation(sphereRep2);
    view2->Update();
    
    // Sphere 3
    VTK_CREATE(vtkSurfaceRepresentation, sphereRep3);
    sphereRep3->SetInputConnection(transform->GetOutputPort());
    sphereRep3->SetSelectionLink(link);
    view2->AddRepresentation(sphereRep3);
    view2->Update();
    
    view2->GetRenderer()->ResetCamera();
    view2->Update();
    
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }
  
  return !retVal;
}
