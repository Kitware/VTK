/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkBlockItem.h"
#include "vtkContextTransform.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestContextScene( int argc, char * argv [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 400);

  vtkNew<vtkBlockItem> test;
  test->SetDimensions(20, 20, 30, 40);
  vtkNew<vtkBlockItem> test2;
  test2->SetDimensions(80, 20, 30, 40);

  vtkNew<vtkBlockItem> parent;
  parent->SetDimensions(20, 200, 80, 40);
  parent->SetLabel("Parent");
  vtkNew<vtkBlockItem> child;
  child->SetDimensions(120, 200, 80, 46);
  child->SetLabel("Child");
  vtkNew<vtkBlockItem> child2;
  child2->SetDimensions(150, 250, 86, 46);
  child2->SetLabel("Child2");

  vtkNew<vtkContextTransform> transform;
  transform->AddItem(parent.GetPointer());
  transform->Translate(50, -190);

  // Build up our multi-level scene
  view->GetScene()->AddItem(test.GetPointer());   // scene
  view->GetScene()->AddItem(test2.GetPointer());  // scene
  view->GetScene()->AddItem(parent.GetPointer()); // scene
  parent->AddItem(child.GetPointer());            // scene->parent
  child->AddItem(child2.GetPointer());            // scene->parent->child

  // Add our transformed item
  view->GetScene()->AddItem(transform.GetPointer());

  // Turn off the color buffer
  view->GetScene()->SetUseBufferId(false);

  view->GetRenderWindow()->SetMultiSamples(0);

  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }
  return 0;
  //return !retVal;
}
