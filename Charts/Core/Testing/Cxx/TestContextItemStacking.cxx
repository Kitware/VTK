/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContextItemStacking.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBlockItem.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
int TestContextItemStacking(int, char*[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(0.32, 0.40, 0.47);
  view->GetRenderWindow()->SetSize(400, 400);

  vtkNew<vtkBlockItem> rootItem;
  rootItem->SetDimensions(0, 350, 50, 50);

  int i = 0;
  int s = 120;
  int step = s / 3;
  vtkNew<vtkBlockItem> test1;
  test1->SetDimensions(i, i, s, s);
  test1->SetLabel("1");
  i += step;
  vtkNew<vtkBlockItem> test2;
  test2->SetDimensions(i, i, s, s);
  test2->SetLabel("2");
  i += step;
  vtkNew<vtkBlockItem> test3;
  test3->SetDimensions(i, i, s, s);
  test3->SetLabel("3");
  i += step;
  vtkNew<vtkBlockItem> test4;
  test4->SetDimensions(i, i, s, s);
  test4->SetLabel("4");
  i += step;
  vtkNew<vtkBlockItem> test41;
  test41->SetDimensions(i, i, s, s);
  test41->SetLabel("4.1");
  i += step;
  vtkNew<vtkBlockItem> test411;
  test411->SetDimensions(i, i, s, s);
  test411->SetLabel("4.1.1");
  i += step;
  vtkNew<vtkBlockItem> test42;
  test42->SetDimensions(i, i, s, s);
  test42->SetLabel("4.2");
  i += step;
  vtkNew<vtkBlockItem> test5;
  test5->SetDimensions(i, i, s, s);
  test5->SetLabel("5");

  // Build up our multi-level scene
  vtkIdType index1 = rootItem->AddItem(test1);
  vtkIdType index2 = rootItem->AddItem(test2);
  vtkIdType index3 = rootItem->AddItem(test3);
  vtkIdType index4 = rootItem->AddItem(test4);
  vtkIdType index41 = test4->AddItem(test41);
  vtkIdType index411 = test41->AddItem(test411);
  vtkIdType index42 = test4->AddItem(test42);
  vtkIdType index5 = rootItem->AddItem(test5);
  view->GetScene()->AddItem(rootItem);

  // Check indexes
  if (index1 != 0 || index2 != 1 || index3 != 2 || index4 != 3 || index41 != 0 || index411 != 0 ||
    index42 != 1 || index5 != 4)
  {
    std::cerr << "AddItem, bad indexes: " << index1 << ", " << index2 << ", " << index3 << ", "
              << index4 << ", " << index41 << ", " << index411 << ", " << index42 << ", " << index5
              << std::endl;
    return EXIT_FAILURE;
  }
  // Restack item 3 under all items
  int res = rootItem->Lower(rootItem->GetItemIndex(test3));
  index1 = rootItem->GetItemIndex(test1);
  index2 = rootItem->GetItemIndex(test2);
  index3 = rootItem->GetItemIndex(test3);
  index4 = rootItem->GetItemIndex(test4);
  if (res != 0 || index1 != 1 || index2 != 2 || index3 != 0 || index4 != 3)
  {
    std::cerr << "Lower, bad indexes: " << res << "->" << index1 << ", " << index2 << ", " << index3
              << ", " << index4 << ", " << index41 << ", " << index411 << ", " << index42 << ", "
              << index5 << std::endl;
    return EXIT_FAILURE;
  }
  // Restack item 1 above 4
  res = rootItem->StackAbove(index1, index4);
  index1 = rootItem->GetItemIndex(test1);
  index2 = rootItem->GetItemIndex(test2);
  index3 = rootItem->GetItemIndex(test3);
  index4 = rootItem->GetItemIndex(test4);
  index41 = test4->GetItemIndex(test41);
  index42 = test4->GetItemIndex(test42);
  index5 = rootItem->GetItemIndex(test5);
  if (res != 3 || index1 != 3 || index2 != 1 || index3 != 0 || index4 != 2 || index41 != 0 ||
    index411 != 0 || index42 != 1 || index5 != 4)
  {
    std::cerr << "StackAbove, bad indexes: " << res << "->" << index1 << ", " << index2 << ", "
              << index3 << ", " << index4 << ", " << index41 << ", " << index411 << ", " << index42
              << ", " << index5 << std::endl;
    return EXIT_FAILURE;
  }
  // Restack item 41 above 42
  res = test4->Raise(index41);
  index1 = rootItem->GetItemIndex(test1);
  index2 = rootItem->GetItemIndex(test2);
  index3 = rootItem->GetItemIndex(test3);
  index4 = rootItem->GetItemIndex(test4);
  index41 = test4->GetItemIndex(test41);
  index42 = test4->GetItemIndex(test42);
  index5 = rootItem->GetItemIndex(test5);
  if (res != 1 || index1 != 3 || index2 != 1 || index3 != 0 || index4 != 2 || index41 != 1 ||
    index411 != 0 || index42 != 0 || index5 != 4)
  {
    std::cerr << "Raise, bad indexes: " << res << "->" << index1 << ", " << index2 << ", " << index3
              << ", " << index4 << ", " << index41 << ", " << index411 << ", " << index42 << ", "
              << index5 << std::endl;
    return EXIT_FAILURE;
  }
  // Restack item 1 above 4
  res = rootItem->StackUnder(index2, index3);
  index1 = rootItem->GetItemIndex(test1);
  index2 = rootItem->GetItemIndex(test2);
  index3 = rootItem->GetItemIndex(test3);
  index4 = rootItem->GetItemIndex(test4);
  index41 = test4->GetItemIndex(test41);
  index42 = test4->GetItemIndex(test42);
  index5 = rootItem->GetItemIndex(test5);
  if (res != 0 || index1 != 3 || index2 != 0 || index3 != 1 || index4 != 2 || index41 != 1 ||
    index411 != 0 || index42 != 0 || index5 != 4)
  {
    std::cerr << "StackUnder, bad indexes: " << res << "->" << index1 << ", " << index2 << ", "
              << index3 << ", " << index4 << ", " << index41 << ", " << index411 << ", " << index42
              << ", " << index5 << std::endl;
    return EXIT_FAILURE;
  }

  // Turn off the color buffer
  view->GetScene()->SetUseBufferId(false);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
