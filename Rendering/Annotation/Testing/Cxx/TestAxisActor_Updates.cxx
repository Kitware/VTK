// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * vtkAxisActor has different modes that have huge impact
 * on underlying text objects: `Use2DMode` and `UseTextActor3D`.
 *
 * Following tests ensure that switching between modes does not fails
 * due to internal state.
 */

#include "TestAxisActorInternal.h"
#include "vtkAxisActor.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkRendererCollection.h"

#include <array>

namespace
{
//------------------------------------------------------------------------------
void UpdateTextProperties()
{
  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkAxisActor> axis;
  ::InitializeXAxis(axis);
  ::AddToWindow(window, axis);

  const std::array<int, 4> sizes{ 8, 20, 12, 40 };
  for (int fontSize : sizes)
  {
    vtkNew<vtkTextProperty> titleProp;
    titleProp->SetFontSize(fontSize * 2);
    axis->SetTitleTextProperty(titleProp);

    vtkNew<vtkTextProperty> labelsProp;
    labelsProp->SetFontSize(fontSize);
    axis->SetLabelTextProperty(labelsProp);
    window->Render();
  }
}

//------------------------------------------------------------------------------
void UpdateMode()
{
  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkAxisActor> axis;
  ::InitializeXAxis(axis);
  ::AddToWindow(window, axis);

  for (int i = 0; i < 4; i++)
  {
    axis->SetUseTextActor3D(i < 2);
    axis->SetUse2DMode(i % 2 == 0);

    window->Render();
  }
}

//------------------------------------------------------------------------------
void TestLifeTime()
{
  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkTextProperty> externProperty;

  {
    vtkNew<vtkAxisActor> axis;
    ::InitializeXAxis(axis);
    ::AddToWindow(window, axis);

    axis->SetTitleTextProperty(externProperty);
    window->Render();

    vtkNew<vtkTextProperty> labelsProp;
    labelsProp->SetFontSize(externProperty->GetFontSize() * 2);
    axis->SetLabelTextProperty(labelsProp);

    // clean any references to `axis`: remove it from view
    auto r = window->GetRenderers();
    vtkRenderer* renderer = r->GetFirstRenderer();
    renderer->RemoveActor(axis);
  }

  externProperty->SetFontSize(6);
  window->Render();
}
}

//------------------------------------------------------------------------------
int TestAxisActor_Updates(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  ::UpdateTextProperties();

  ::UpdateMode();

  ::TestLifeTime();

  return EXIT_SUCCESS;
}
