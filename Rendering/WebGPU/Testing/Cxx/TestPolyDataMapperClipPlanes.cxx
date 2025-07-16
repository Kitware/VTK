// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkConeSource.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLineSource.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNamedColors.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRegularPolygonSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWidgetRepresentation.h"

#include <array>

namespace
{
vtkNew<vtkPolyLineSource> MakePolyLineSource(
  vtkIdType numPts, bool closed, double offsetX, double offsetY)
{
  vtkNew<vtkPolyLineSource> polylines;
  polylines->Resize(numPts);
  polylines->SetClosed(closed);
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    double zigzag = (i % 2);
    polylines->SetPoint(i, i + offsetX + zigzag, i + offsetY, 0);
  }
  return polylines;
}

void SetupPlaneWidgetCallback(
  vtkImplicitPlaneWidget2* widget, vtkPlane* plane, double* widgetBounds)
{
  vtkNew<vtkCallbackCommand> onPlaneInteraction;
  onPlaneInteraction->SetCallback(
    [](vtkObject* sender, unsigned long, void* clientData, void*)
    {
      vtkImplicitPlaneWidget2* planeWidget = vtkImplicitPlaneWidget2::SafeDownCast(sender);
      vtkImplicitPlaneRepresentation* rep =
        vtkImplicitPlaneRepresentation::SafeDownCast(planeWidget->GetRepresentation());
      rep->GetPlane(reinterpret_cast<vtkPlane*>(clientData));
    });
  onPlaneInteraction->SetClientData(plane);
  widget->AddObserver(vtkCommand::InteractionEvent, onPlaneInteraction);

  auto* rep = vtkImplicitPlaneRepresentation::SafeDownCast(widget->GetRepresentation());
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(widgetBounds);
  rep->SetPlane(plane);
  widget->On();
}
} // namespace

int TestPolyDataMapperClipPlanes(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkAppendPolyData> append;

  auto polylinesClosed = ::MakePolyLineSource(5, true, 0, 0);
  append->AddInputConnection(polylinesClosed->GetOutputPort());

  auto polylinesOpen = ::MakePolyLineSource(5, false, 6, 0);
  append->AddInputConnection(polylinesOpen->GetOutputPort());

  vtkNew<vtkRegularPolygonSource> polygon;
  polygon->SetGeneratePolygon(true);
  polygon->SetCenter(15, 3, 0);
  polygon->SetRadius(3);
  polygon->SetNumberOfSides(12);
  append->AddInputConnection(polygon->GetOutputPort());

  vtkNew<vtkLineSource> lines;
  lines->SetPoint1(2, 10, 0);
  lines->SetPoint2(0, 12, 0);
  append->AddInputConnection(lines->GetOutputPort());

  vtkNew<vtkPointSource> points;
  points->SetNumberOfPoints(20);
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(1);
  points->SetRandomSequence(randomSequence);
  points->SetCenter(8, 10, 0);
  points->SetRadius(2);
  append->AddInputConnection(points->GetOutputPort());

  vtkNew<vtkConeSource> cone;
  cone->SetCenter(15, 10, 0);
  cone->SetRadius(2);
  cone->SetHeight(4);
  append->AddInputConnection(cone->GetOutputPort());
  append->Update();
  auto* polydata = append->GetOutput();

  vtkNew<vtkNamedColors> namedColors;
  std::array<vtkColor4ub, 4> colors;
  colors[0] = namedColors->GetColor4ub("tomato");
  colors[1] = namedColors->GetColor4ub("cyan");
  colors[2] = namedColors->GetColor4ub("green");
  colors[3] = namedColors->GetColor4ub("yellow");

  vtkNew<vtkUnsignedCharArray> colorArray;
  colorArray->SetName("color");
  const vtkIdType numCells = polydata->GetNumberOfCells();
  colorArray->SetNumberOfComponents(4);
  colorArray->SetNumberOfTuples(numCells);
  // Round-robin assignment of colors
  for (int i = 0; i < numCells; ++i)
  {
    colorArray->SetTypedTuple(i, colors[i % 4].GetData());
  }

  vtkCellData* cd = polydata->GetCellData();
  cd->SetScalars(colorArray);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(polydata);
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellData();
  mapper->SetColorModeToDirectScalars();

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetPointSize(2);
  actor->GetProperty()->SetLineWidth(2);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

  vtkNew<vtkPlane> plane1;
  mapper->AddClippingPlane(plane1);
  plane1->SetNormal(-0.024947, 0.908778, 0.416534);
  plane1->SetOrigin(8.58869, 2.77203, 0.258529);

  vtkNew<vtkPlane> plane2;
  mapper->AddClippingPlane(plane2);
  plane2->SetNormal(-0.1173493, -0.999757, -0.0135635);
  plane2->SetOrigin(8.60057, 10.4484, 1.47082);

  vtkNew<vtkImplicitPlaneWidget2> plane1Widget;
  plane1Widget->SetInteractor(iren);
  SetupPlaneWidgetCallback(plane1Widget, plane1, polydata->GetBounds());
  vtkNew<vtkImplicitPlaneWidget2> plane2Widget;
  plane2Widget->SetInteractor(iren);
  SetupPlaneWidgetCallback(plane2Widget, plane2, polydata->GetBounds());

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
