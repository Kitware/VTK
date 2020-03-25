/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBlockItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlockItem.h"
#include "vtkBrush.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

static vtkSmartPointer<vtkBlockItem> AddItem(const char* label, int halign, int valign)
{
  vtkNew<vtkBlockItem> test;
  test->SetLabel(label);
  test->SetHorizontalAlignment(halign);
  test->SetVerticalAlignment(valign);
  test->SetAutoComputeDimensions(true);
  test->GetBrush()->SetColorF(0.7, 0.7, 0.7);
  return test;
}

int TestBlockItem(int argc, char* argv[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 600);

  view->GetScene()->AddItem(::AddItem("Left-Top", vtkBlockItem::LEFT, vtkBlockItem::TOP));
  view->GetScene()->AddItem(::AddItem("Left-Center", vtkBlockItem::LEFT, vtkBlockItem::CENTER));
  view->GetScene()->AddItem(::AddItem("Left-Bottom", vtkBlockItem::LEFT, vtkBlockItem::BOTTOM));

  view->GetScene()->AddItem(::AddItem("Right-Top", vtkBlockItem::RIGHT, vtkBlockItem::TOP));
  view->GetScene()->AddItem(::AddItem("Right-Center", vtkBlockItem::RIGHT, vtkBlockItem::CENTER));
  view->GetScene()->AddItem(::AddItem("Right-Bottom", vtkBlockItem::RIGHT, vtkBlockItem::BOTTOM));

  view->GetScene()->AddItem(::AddItem("Center-Top", vtkBlockItem::CENTER, vtkBlockItem::TOP));
  view->GetScene()->AddItem(::AddItem("Center-Center", vtkBlockItem::CENTER, vtkBlockItem::CENTER));
  view->GetScene()->AddItem(::AddItem("Center-Bottom", vtkBlockItem::CENTER, vtkBlockItem::BOTTOM));

  // Turn off the color buffer
  view->GetScene()->SetUseBufferId(false);
  view->GetRenderWindow()->SetMultiSamples(0);
  view->Render();
  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  switch (retVal)
  {
    case vtkTesting::DO_INTERACTOR:
      view->GetInteractor()->Initialize();
      view->GetInteractor()->Start();
      return EXIT_SUCCESS;

    case vtkTesting::NOT_RUN:
      return VTK_SKIP_RETURN_CODE;

    case vtkTesting::PASSED:
      return EXIT_SUCCESS;

    case vtkTesting::FAILED:
    default:
      return EXIT_FAILURE;
  }
}
