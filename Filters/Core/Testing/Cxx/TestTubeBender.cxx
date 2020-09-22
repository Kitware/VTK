/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTubeBender.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include <sstream>

#include "vtkDebugLeaks.h"

#include "vtkRegressionTestImage.h"
#include "vtkTubeBender.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFeatureEdges.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLine.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTubeFilter.h"
#include "vtkVersion.h"

int TestTubeBender(int argc, char* argv[])
{
  std::ostringstream strm;
  strm << "TestTubeBender Start" << endl;

  vtkNew<vtkPolyData> line;
  {
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;

    points->InsertNextPoint(0.0, 0.0, 0.0);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    points->InsertNextPoint(0.5, 0.0, 0.0);
    points->InsertNextPoint(0.5, 1.0, 0.0);

    points->InsertNextPoint(0.0, 2.0, 0.0);
    points->InsertNextPoint(0.0, 3.0, 0.0);
    points->InsertNextPoint(0.5, 2.0, 0.0);
    points->InsertNextPoint(0.5, 3.0, 0.0);
    points->InsertNextPoint(1.5, 3.0, 0.0);
    points->InsertNextPoint(2.5, 2.2, 0.0);

    std::vector<int> cell1 = { 0, 1, 2, 3 };
    std::vector<int> cell2 = { 4, 5, 6, 7, 8, 9 };
    std::vector<std::vector<int>> cellMap{ cell1, cell2 };

    for (size_t i = 0; i < cellMap.size(); i++)
    {
      auto cellN = cellMap[i];
      vtkNew<vtkPolyLine> polyline;
      polyline->GetPointIds()->SetNumberOfIds(static_cast<vtkIdType>(cellN.size()));
      for (size_t j = 0; j < cellN.size(); j++)
      {
        polyline->GetPointIds()->SetId(static_cast<vtkIdType>(j), static_cast<vtkIdType>(cellN[j]));
      }
      cells->InsertNextCell(polyline);
    }

    line->SetPoints(points);
    line->SetLines(cells);
  }

  vtkNew<vtkActor> lineActor;
  {
    vtkNew<vtkPolyDataMapper> lineMapper;
    lineMapper->SetInputData(line);

    lineActor->GetProperty()->SetColor(0.0, 0.0, 0.1);
    lineActor->SetMapper(lineMapper);
  }

  vtkNew<vtkActor> tubeActor;
  {
    double radius = 0.1;

    vtkNew<vtkTubeBender> tubeBender;
    tubeBender->SetInputData(line);
    tubeBender->SetRadius(radius);

    vtkNew<vtkTubeFilter> tubeFilter;
    tubeFilter->SetInputConnection(tubeBender->GetOutputPort());
    // tubeFilter->SetInputData(line);
    tubeFilter->SetRadius(radius);
    tubeFilter->SetNumberOfSides(50);

    vtkNew<vtkPolyDataMapper> lineMapper;
    lineMapper->SetInputConnection(tubeFilter->GetOutputPort());

    tubeActor->GetProperty()->SetColor(0.0, 1.0, 0.1);
    tubeActor->SetMapper(lineMapper);
    tubeActor->GetProperty()->SetOpacity(0.5);
  }

  strm << "TestTubeBender End" << endl;

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(lineActor);
  renderer->AddActor(tubeActor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(500, 500);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}
