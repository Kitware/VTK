/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHeatmapCategoryLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHeatmapItem.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkContextView.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestHeatmapCategoryLegend(int argc, char* argv[])
{
  vtkNew<vtkTable> table;
  vtkNew<vtkStringArray> tableNames;
  vtkNew<vtkStringArray> column;

  tableNames->SetNumberOfTuples(4);
  tableNames->SetValue(0, "c");
  tableNames->SetValue(1, "b");
  tableNames->SetValue(2, "a");
  tableNames->SetValue(3, "a");
  tableNames->SetName("names");

  column->SetNumberOfTuples(4);
  column->SetName("values");
  column->SetValue(0, "c");
  column->SetValue(1, "b");
  column->SetValue(2, "a");
  column->SetValue(3, "a");

  table->AddColumn(tableNames.GetPointer());
  table->AddColumn(column.GetPointer());

  vtkNew<vtkHeatmapItem> heatmap;
  heatmap->SetTable(table.GetPointer());

  vtkNew<vtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(heatmap.GetPointer());
  trans->Translate(125, 125);

  vtkNew<vtkContextView> contextView;
  contextView->GetScene()->AddItem(trans.GetPointer());

  contextView->GetRenderWindow()->SetMultiSamples(0);
  contextView->GetRenderWindow()->Render();

  // double click to display the category legend
  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(contextView->GetInteractor());
  vtkVector2f pos;
  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  pos.Set(16, 38);
  mouseEvent.SetPos(pos);
  heatmap->MouseDoubleClickEvent(mouseEvent);
  contextView->GetRenderWindow()->Render();

  int retVal = vtkRegressionTestImage(contextView->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    contextView->GetRenderWindow()->Render();
    contextView->GetInteractor()->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return !retVal;
}
