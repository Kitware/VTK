/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarBarAboveBelow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"

int TestScalarBarAboveBelow(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int resolution = 3;
  vtkNew<vtkPlaneSource> plane;
  plane->SetXResolution(resolution);
  plane->SetYResolution(resolution);

  vtkNew<vtkDoubleArray> cellData;
  for (int i = 0; i < resolution * resolution; i++)
  {
    cellData->InsertNextValue(i);
  }

  plane->Update(); // Force an update so we can set cell data
  plane->GetOutput()->GetCellData()->SetScalars(cellData.Get());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());
  mapper->SetScalarRange(1, 7);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());

  vtkScalarsToColors* stc = mapper->GetLookupTable();
  vtkLookupTable* lut = vtkLookupTable::SafeDownCast(stc);
  lut->SetUseBelowRangeColor(true);
  lut->SetUseAboveRangeColor(true);
  lut->SetNumberOfColors(7);

  vtkNew<vtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(stc);
  scalarBar->SetDrawBelowRangeSwatch(true);
  scalarBar->SetDrawAboveRangeSwatch(true);

  vtkNew<vtkScalarBarActor> scalarBar2;
  scalarBar2->SetLookupTable(stc);
  scalarBar2->SetDrawBelowRangeSwatch(true);
  scalarBar2->SetOrientationToHorizontal();
  scalarBar2->SetWidth(0.5);
  scalarBar2->SetHeight(0.15);
  scalarBar2->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar2->GetPositionCoordinate()->SetValue(.05, .8);

  vtkNew<vtkScalarBarActor> scalarBar3;
  scalarBar3->SetLookupTable(stc);
  scalarBar3->SetDrawAboveRangeSwatch(true);
  scalarBar3->SetOrientationToHorizontal();
  scalarBar3->SetWidth(0.5);
  scalarBar3->SetHeight(0.15);
  scalarBar3->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar3->GetPositionCoordinate()->SetValue(.05, .2);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.Get());
  renderer->AddActor(actor.Get());
  renderer->AddActor(scalarBar.Get());
  renderer->AddActor(scalarBar2.Get());
  renderer->AddActor(scalarBar3.Get());
  renderer->SetBackground(.5, .5, .5);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
