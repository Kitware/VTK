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

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestContextScene( int argc, char * argv [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 400);

  vtkSmartPointer<vtkBlockItem> test = vtkSmartPointer<vtkBlockItem>::New();
  test->SetDimensions(20, 20, 30, 40);
  vtkSmartPointer<vtkBlockItem> test2 = vtkSmartPointer<vtkBlockItem>::New();
  test2->SetDimensions(80, 20, 30, 40);

  vtkSmartPointer<vtkBlockItem> parent = vtkSmartPointer<vtkBlockItem>::New();
  parent->SetDimensions(20, 200, 80, 40);
  parent->SetLabel("Parent");
  vtkSmartPointer<vtkBlockItem> child = vtkSmartPointer<vtkBlockItem>::New();
  child->SetDimensions(120, 200, 80, 46);
  child->SetLabel("Child");
  vtkSmartPointer<vtkBlockItem> child2 = vtkSmartPointer<vtkBlockItem>::New();
  child2->SetDimensions(150, 250, 86, 46);
  child2->SetLabel("Child2");

  vtkSmartPointer<vtkContextTransform> transform =
      vtkSmartPointer<vtkContextTransform>::New();
  transform->AddItem(parent);
  transform->Translate(50, -190);

  // Build up our multi-level scene
  view->GetScene()->AddItem(test);   // scene
  view->GetScene()->AddItem(test2);  // scene
  view->GetScene()->AddItem(parent); // scene
  parent->AddItem(child);            // scene->parent
  child->AddItem(child2);            // scene->parent->child

  // Add our transformed item
  view->GetScene()->AddItem(transform);

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
