/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHeatmapItem.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestHeatmapItem(int argc, char* argv[])
{
  vtkNew<vtkTable> table;
  vtkNew<vtkStringArray> tableNames;
  vtkNew<vtkDoubleArray> m1;
  vtkNew<vtkDoubleArray> m2;
  vtkNew<vtkDoubleArray> m3;
  vtkNew<vtkStringArray> m4;

  tableNames->SetNumberOfTuples(3);
  tableNames->SetValue(0, "c");
  tableNames->SetValue(1, "b");
  tableNames->SetValue(2, "a");
  tableNames->SetName("name");

  m1->SetNumberOfTuples(3);
  m2->SetNumberOfTuples(3);
  m3->SetNumberOfTuples(3);
  m4->SetNumberOfTuples(3);

  m1->SetName("m1");
  m2->SetName("m2");
  m3->SetName("m3");
  m4->SetName("m4");

  m1->SetValue(0, 1.0f);
  m1->SetValue(1, 3.0f);
  m1->SetValue(2, 1.0f);

  m2->SetValue(0, 2.0f);
  m2->SetValue(1, 2.0f);
  m2->SetValue(2, 2.0f);

  m3->SetValue(0, 3.0f);
  m3->SetValue(1, 1.0f);
  m3->SetValue(2, 3.0f);

  m4->SetValue(0, "a");
  m4->SetValue(1, "b");
  m4->SetValue(2, "c");

  table->AddColumn(tableNames.GetPointer());
  table->AddColumn(m1.GetPointer());
  table->AddColumn(m2.GetPointer());
  table->AddColumn(m3.GetPointer());
  table->AddColumn(m4.GetPointer());

  vtkNew<vtkContextActor> actor;

  vtkNew<vtkHeatmapItem> heatmap;
  heatmap->SetTable(table.GetPointer());
  heatmap->SetPosition(20, 5);

  vtkNew<vtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(heatmap.GetPointer());
  trans->Scale(2, 2);
  actor->GetScene()->AddItem(trans.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 200);
  renderWindow->AddRenderer(renderer.GetPointer());
  renderer->AddActor(actor.GetPointer());
  actor->GetScene()->SetRenderer(renderer.GetPointer());

  vtkNew<vtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(actor->GetScene());

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetInteractorStyle(interactorStyle.GetPointer());
  interactor->SetRenderWindow(renderWindow.GetPointer());
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return !retVal;
}
