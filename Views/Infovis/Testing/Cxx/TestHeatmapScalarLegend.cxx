/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHeatmapScalarLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHeatmapItem.h"
#include "vtkNew.h"
#include "vtkIntArray.h"
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
int TestHeatmapScalarLegend(int argc, char* argv[])
{
  vtkNew<vtkTable> table;
  vtkNew<vtkStringArray> tableNames;
  vtkNew<vtkIntArray> column;

  tableNames->SetNumberOfTuples(3);
  tableNames->SetValue(0, "3");
  tableNames->SetValue(1, "2");
  tableNames->SetValue(2, "1");
  tableNames->SetName("names");

  column->SetNumberOfTuples(3);
  column->SetName("values");
  column->SetValue(0, 3);
  column->SetValue(1, 2);
  column->SetValue(2, 1);

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

  // double click to display the color legend
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
