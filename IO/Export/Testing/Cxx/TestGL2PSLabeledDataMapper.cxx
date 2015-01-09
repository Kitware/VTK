/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSLabeledDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkGL2PSExporter.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellCenters.h"
#include "vtkIdFilter.h"
#include "vtkLabeledDataMapper.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelectVisiblePoints.h"
#include "vtkSphereSource.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

// This test is adapted from labeledMesh.py to test GL2PS exporting of selection
// labels.
int TestGL2PSLabeledDataMapper(int, char *[] )
{
  // Selection rectangle:
  double xmin = 100.;
  double xmax = 400.;
  double ymin = 100.;
  double ymax = 400.;

  vtkNew<vtkPoints> pts;
  pts->InsertPoint(0, xmin, ymin, 0.);
  pts->InsertPoint(1, xmax, ymin, 0.);
  pts->InsertPoint(2, xmax, ymax, 0.);
  pts->InsertPoint(3, xmin, ymax, 0.);

  vtkNew<vtkCellArray> rect;
  rect->InsertNextCell(5);
  rect->InsertCellPoint(0);
  rect->InsertCellPoint(1);
  rect->InsertCellPoint(2);
  rect->InsertCellPoint(3);
  rect->InsertCellPoint(0);

  vtkNew<vtkPolyData> selectRect;
  selectRect->SetPoints(pts.GetPointer());
  selectRect->SetLines(rect.GetPointer());

  vtkNew<vtkPolyDataMapper2D> rectMapper;
  vtkNew<vtkActor2D> rectActor;
  rectMapper->SetInputData(selectRect.GetPointer());
  rectActor->SetMapper(rectMapper.GetPointer());

  // Create sphere
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> sphereMapper;
  vtkNew<vtkActor> sphereActor;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereActor->SetMapper(sphereMapper.GetPointer());

  // Generate ids for labeling
  vtkNew<vtkIdFilter> ids;
  ids->SetInputConnection(sphere->GetOutputPort());
  ids->PointIdsOn();
  ids->CellIdsOn();
  ids->FieldDataOn();

  // Create labels for points
  vtkNew<vtkSelectVisiblePoints> visPts;
  visPts->SetInputConnection(ids->GetOutputPort());
  visPts->SelectionWindowOn();
  visPts->SetSelection(static_cast<int>(xmin), static_cast<int>(xmax), static_cast<int>(ymin), static_cast<int>(ymax));

  vtkNew<vtkLabeledDataMapper> ldm;
  ldm->SetInputConnection(visPts->GetOutputPort());
  ldm->SetLabelModeToLabelFieldData();

  vtkNew<vtkActor2D> pointLabels;
  pointLabels->SetMapper(ldm.GetPointer());

  // Create labels for cells:
  vtkNew<vtkCellCenters> cc;
  cc->SetInputConnection(ids->GetOutputPort());

  vtkNew<vtkSelectVisiblePoints> visCells;
  visCells->SetInputConnection(cc->GetOutputPort());
  visCells->SelectionWindowOn();
  visCells->SetSelection(static_cast<int>(xmin), static_cast<int>(xmax), static_cast<int>(ymin), static_cast<int>(ymax));

  vtkNew<vtkLabeledDataMapper> cellMapper;
  cellMapper->SetInputConnection(visCells->GetOutputPort());
  cellMapper->SetLabelModeToLabelFieldData();
  cellMapper->GetLabelTextProperty()->SetColor(0., 1., 0.);

  vtkNew<vtkActor2D> cellLabels;
  cellLabels->SetMapper(cellMapper.GetPointer());

  // Rendering setup
  vtkNew<vtkRenderer> ren;
  visPts->SetRenderer(ren.GetPointer());
  visCells->SetRenderer(ren.GetPointer());
  ren->AddActor(sphereActor.GetPointer());
  ren->AddActor2D(rectActor.GetPointer());
  ren->AddActor2D(pointLabels.GetPointer());
  ren->AddActor2D(cellLabels.GetPointer());
  ren->SetBackground(1., 1., 1.);
  ren->GetActiveCamera()->Zoom(.55);

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  renWin->AddRenderer(ren.GetPointer());
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);
  renWin->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(renWin.GetPointer());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetPS3Shading(0);
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOff();
  exp->SetTextAsPath(true);

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSLabeledDataMapper");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
