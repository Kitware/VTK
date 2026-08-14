// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises the scalar bars the view maintains: one per array being rendered,
// shared by every representation that renders it, spanning the union of their
// ranges, and coming and going with the scene.

#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkLookupTableManager.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPropCollection.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkScalarBarWidget.h"
#include "vtkScalarsToColors.h"
#include "vtkScivisScalarBars.h"
#include "vtkScivisView.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkTrivialProducer.h"
#include "vtkVolumeRepresentation.h"

#include <cmath>
#include <cstring>
#include <iostream>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << "\n";                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

namespace
{

bool Near(double a, double b)
{
  return std::abs(a - b) < 1e-9;
}

bool RangeIs(const double range[2], double low, double high)
{
  if (Near(range[0], low) && Near(range[1], high))
  {
    return true;
  }
  std::cerr << "  range is [" << range[0] << ", " << range[1] << "], expected [" << low << ", "
            << high << "]\n";
  return false;
}

// A sphere whose active scalars, "Height", run from `offset` upwards, so that
// two of them at different offsets have plainly different ranges.
vtkSmartPointer<vtkPolyData> MakeSurface(double offset)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  auto surface = vtkSmartPointer<vtkPolyData>::New();
  surface->DeepCopy(sphere->GetOutput());

  const vtkIdType numberOfPoints = surface->GetNumberOfPoints();
  vtkNew<vtkDoubleArray> heights;
  heights->SetName("Height");
  heights->SetNumberOfTuples(numberOfPoints);
  for (vtkIdType i = 0; i < numberOfPoints; ++i)
  {
    heights->SetValue(i, offset + i);
  }
  surface->GetPointData()->AddArray(heights);
  surface->GetPointData()->SetActiveScalars("Height");

  return surface;
}

vtkSmartPointer<vtkTrivialProducer> Produce(vtkDataObject* data)
{
  auto producer = vtkSmartPointer<vtkTrivialProducer>::New();
  producer->SetOutput(data);
  return producer;
}

vtkSmartPointer<vtkScivisView> MakeView()
{
  auto view = vtkSmartPointer<vtkScivisView>::New();
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);
  return view;
}

// Two surfaces coloring by one array share a bar, a table and a range.
int TestOneBarPerArray()
{
  auto view = MakeView();

  auto first = MakeSurface(0.0);
  auto second = MakeSurface(100.0);
  const double lastPoint = first->GetNumberOfPoints() - 1;

  auto firstProducer = Produce(first);
  auto secondProducer = Produce(second);
  vtkNew<vtkSurfaceRepresentation> firstRep;
  firstRep->SetInputConnection(firstProducer->GetOutputPort());
  vtkNew<vtkSurfaceRepresentation> secondRep;
  secondRep->SetInputConnection(secondProducer->GetOutputPort());
  view->AddRepresentation(firstRep);
  view->AddRepresentation(secondRep);

  CHECK(view->GetScalarBars()->GetNumberOfBars() == 0,
    "there are bars before anything has been rendered");

  view->Render();

  CHECK(view->GetScalarBars()->GetNumberOfBars() == 1,
    "two surfaces of one array did not share a single bar");
  CHECK(view->GetScalarBars()->GetArrayName(0) != nullptr &&
      strcmp(view->GetScalarBars()->GetArrayName(0), "Height") == 0,
    "the bar is not labelled with the array being rendered");
  CHECK(view->GetScalarBars()->GetFieldAssociation(0) == vtkDataObject::FIELD_ASSOCIATION_POINTS,
    "a point array is not reported as point associated");

  vtkScalarBarActor* bar =
    view->GetScalarBars()->GetActor("Height", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  CHECK(bar != nullptr, "the bar cannot be found by the array it is labelled with");
  CHECK(bar == view->GetScalarBars()->GetActor(0),
    "the bar found by name is not the one found by index");
  CHECK(bar->GetLookupTable() != nullptr, "the bar shows no table");

  // Both representations color through the manager's table for the array, and
  // that table spans both of their data.
  vtkScalarsToColors* shared = view->GetLookupTableManager()->GetLookupTable("Height");
  CHECK(firstRep->GetColorMap() == shared && secondRep->GetColorMap() == shared,
    "the representations were not put on the shared table");
  CHECK(bar->GetLookupTable() == shared, "the bar does not show the table the surfaces color by");
  CHECK(RangeIs(shared->GetRange(), 0.0, 100.0 + lastPoint),
    "the shared range does not cover both representations");

  // What is out of the scene loses its bar; what is left keeps the range the
  // two of them reached, because a live bar's range only grows.
  secondRep->SetVisibility(false);
  view->Render();
  CHECK(view->GetScalarBars()->GetNumberOfBars() == 1,
    "hiding one of two contributors retired the bar");
  CHECK(RangeIs(shared->GetRange(), 0.0, 100.0 + lastPoint),
    "the range shrank when a contributor left");

  firstRep->SetVisibility(false);
  view->Render();
  CHECK(view->GetScalarBars()->GetNumberOfBars() == 0, "a bar outlived everything that fed it");
  CHECK(
    view->GetScalarBars()->GetActor("Height", vtkDataObject::FIELD_ASSOCIATION_POINTS) == nullptr,
    "a retired bar is still reachable by name");

  return EXIT_SUCCESS;
}

// Different arrays get different bars.
int TestOneBarPerDistinctArray()
{
  auto view = MakeView();

  auto surface = MakeSurface(0.0);
  auto producer = Produce(surface);
  vtkNew<vtkSurfaceRepresentation> surfaceRep;
  surfaceRep->SetInputConnection(producer->GetOutputPort());
  view->AddRepresentation(surfaceRep);

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-5, 5, -5, 5, -5, 5);
  vtkNew<vtkVolumeRepresentation> volumeRep;
  volumeRep->SetInputConnection(wavelet->GetOutputPort());
  view->AddRepresentation(volumeRep);

  view->Render();

  CHECK(view->GetScalarBars()->GetNumberOfBars() == 2, "two different arrays did not get two bars");
  // Bars are ordered by array name, so "Height" comes before "RTData".
  CHECK(strcmp(view->GetScalarBars()->GetArrayName(0), "Height") == 0,
    "the bars are not in name order");
  CHECK(strcmp(view->GetScalarBars()->GetArrayName(1), "RTData") == 0,
    "the bars are not in name order");
  CHECK(view->GetScalarBars()->GetActor(2) == nullptr, "an index past the end returned a bar");
  CHECK(view->GetScalarBars()->GetArrayName(-1) == nullptr, "a negative index returned a name");

  // A volume colors through transfer functions of its own, so its bar shows
  // them rather than a table from the manager.
  vtkScalarBarActor* volumeBar =
    view->GetScalarBars()->GetActor("RTData", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  CHECK(volumeBar != nullptr, "the volume's array got no bar");
  CHECK(volumeBar->GetLookupTable() ==
      static_cast<vtkScalarsToColors*>(volumeRep->GetColorTransferFunction()),
    "the volume's bar does not show the volume's transfer function");
  CHECK(!view->GetLookupTableManager()->HasLookupTable("RTData"),
    "the volume's array was given a table in the manager");

  return EXIT_SUCCESS;
}

// A table registered with the manager wins, and keeps the range it was given.
int TestSuppliedLookupTable()
{
  auto view = MakeView();

  vtkNew<vtkLookupTable> supplied;
  supplied->SetRange(-5.0, 5.0);
  view->GetLookupTableManager()->SetLookupTable("Height", supplied);

  auto surface = MakeSurface(0.0);
  auto producer = Produce(surface);
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(producer->GetOutputPort());
  view->AddRepresentation(rep);

  view->Render();

  CHECK(rep->GetColorMap() == supplied.Get(), "the supplied table is not what the surface uses");
  CHECK(view->GetScalarBars()->GetActor(0)->GetLookupTable() == supplied.Get(),
    "the supplied table is not what the bar shows");
  CHECK(RangeIs(supplied->GetRange(), -5.0, 5.0), "the supplied range was written over");

  return EXIT_SUCCESS;
}

// Field data describes a block rather than an element, so it gets no bar.
int TestFieldDataHasNoBar()
{
  auto view = MakeView();

  auto surface = MakeSurface(0.0);
  vtkNew<vtkDoubleArray> metadata;
  metadata->SetName("Metadata");
  metadata->SetNumberOfTuples(2);
  metadata->SetValue(0, 3.0);
  metadata->SetValue(1, 7.0);
  surface->GetFieldData()->AddArray(metadata);

  auto producer = Produce(surface);
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(producer->GetOutputPort());
  rep->ColorByFieldArray("Metadata");
  view->AddRepresentation(rep);

  view->Render();

  CHECK(
    view->GetScalarBars()->GetNumberOfBars() == 0, "an array in field data was given a scalar bar");

  return EXIT_SUCCESS;
}

// Turning the bars off takes them away and stops them coming back.
int TestAutoVisibility()
{
  auto view = MakeView();

  auto surface = MakeSurface(0.0);
  auto producer = Produce(surface);
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(producer->GetOutputPort());
  view->AddRepresentation(rep);

  CHECK(view->GetScalarBars()->GetAutoVisibility(), "the bars are not on by default");
  view->Render();
  CHECK(view->GetScalarBars()->GetNumberOfBars() == 1, "the rendered array got no bar");

  view->GetScalarBars()->SetAutoVisibility(false);
  view->Render();
  CHECK(view->GetScalarBars()->GetNumberOfBars() == 0, "turning the bars off left one behind");

  view->GetScalarBars()->SetAutoVisibility(true);
  view->Render();
  CHECK(view->GetScalarBars()->GetNumberOfBars() == 1,
    "turning the bars back on did not bring one back");

  return EXIT_SUCCESS;
}

// A manager set on two views keeps their coloring in step.
int TestSharedManager()
{
  auto first = MakeView();
  auto second = MakeView();

  vtkNew<vtkLookupTableManager> manager;
  first->SetLookupTableManager(manager);
  second->SetLookupTableManager(manager);
  CHECK(first->GetLookupTableManager() == manager.Get(),
    "the manager that was set is not the one held");

  auto firstSurface = MakeSurface(0.0);
  auto firstProducer = Produce(firstSurface);
  vtkNew<vtkSurfaceRepresentation> firstRep;
  firstRep->SetInputConnection(firstProducer->GetOutputPort());
  first->AddRepresentation(firstRep);

  auto secondSurface = MakeSurface(0.0);
  auto secondProducer = Produce(secondSurface);
  vtkNew<vtkSurfaceRepresentation> secondRep;
  secondRep->SetInputConnection(secondProducer->GetOutputPort());
  second->AddRepresentation(secondRep);

  first->Render();
  second->Render();

  CHECK(firstRep->GetColorMap() == secondRep->GetColorMap(),
    "views sharing a manager did not share a table");

  // Being handed no manager is being handed a fresh one, not none at all.
  second->SetLookupTableManager(nullptr);
  CHECK(
    second->GetLookupTableManager() != nullptr, "clearing the manager left the view without one");
  CHECK(second->GetLookupTableManager() != manager.Get(), "clearing the manager kept the old one");

  return EXIT_SUCCESS;
}

// Whether the renderer is drawing `bar` itself, as opposed to a widget doing it.
bool DrawnByTheRenderer(vtkScivisView* view, vtkScalarBarActor* bar)
{
  vtkPropCollection* props = view->GetRenderer()->GetViewProps();
  props->InitTraversal();
  while (vtkProp* prop = props->GetNextProp())
  {
    if (prop == bar)
    {
      return true;
    }
  }
  return false;
}

bool SameGeometry(vtkScalarBarActor* bar, const double position[2], double width, double height)
{
  double* current = bar->GetPositionCoordinate()->GetValue();
  return std::abs(current[0] - position[0]) < 1e-6 && std::abs(current[1] - position[1]) < 1e-6 &&
    std::abs(bar->GetWidth() - width) < 1e-6 && std::abs(bar->GetHeight() - height) < 1e-6;
}
// Draggable puts each bar on a widget the user can pick up, and taking it away
// puts the bars back the way they were.
int TestDraggable()
{
  auto view = MakeView();
  auto surface = MakeSurface(0.0);
  auto producer = Produce(surface);
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(producer->GetOutputPort());
  view->AddRepresentation(rep);
  view->Render();

  vtkScivisScalarBars* bars = view->GetScalarBars();
  CHECK(bars->GetNumberOfBars() == 1, "no bar to drag");
  CHECK(!bars->GetDraggable(), "the bars start out draggable");
  CHECK(bars->GetWidget(0) == nullptr, "there is a widget before anything is draggable");

  // Where the bar sits before anything is draggable, which becoming draggable
  // must not change.
  vtkScalarBarActor* bar = bars->GetActor(0);
  CHECK(DrawnByTheRenderer(view, bar), "the bar is not being drawn to begin with");
  const double position[2] = { bar->GetPositionCoordinate()->GetValue()[0],
    bar->GetPositionCoordinate()->GetValue()[1] };
  const double width = bar->GetWidth();
  const double height = bar->GetHeight();

  bars->DraggableOn();
  view->Render();
  vtkScalarBarWidget* widget = bars->GetWidget(0);
  CHECK(widget != nullptr, "no widget once the bars are draggable");
  CHECK(widget->GetEnabled() != 0, "the widget is not enabled");
  CHECK(widget->GetScalarBarActor() == bars->GetActor(0),
    "the widget does not carry the bar it is for");
  CHECK(SameGeometry(bars->GetActor(0), position, width, height),
    "becoming draggable moved or resized the bar");
  // The widget draws it now, so the renderer must not as well.
  CHECK(!DrawnByTheRenderer(view, bars->GetActor(0)),
    "the bar is drawn by both the renderer and the widget");

  bars->DraggableOff();
  view->Render();
  CHECK(bars->GetWidget(0) == nullptr, "the widget outlived draggability");
  CHECK(bars->GetNumberOfBars() == 1, "turning dragging off took the bar with it");
  CHECK(bars->GetActor(0) != nullptr, "the bar itself is gone");
  CHECK(DrawnByTheRenderer(view, bars->GetActor(0)),
    "the bar was left with nothing drawing it after dragging was turned off");
  CHECK(SameGeometry(bars->GetActor(0), position, width, height),
    "the bar did not come back the size it was");

  return EXIT_SUCCESS;
}

// A bar the user has moved stays where it was put, whether or not the bars are
// draggable at the moment.
int TestAMovedBarStaysPut()
{
  auto view = MakeView();
  auto surface = MakeSurface(0.0);
  auto producer = Produce(surface);
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(producer->GetOutputPort());
  view->AddRepresentation(rep);
  view->Render();

  vtkScivisScalarBars* bars = view->GetScalarBars();
  bars->DraggableOn();
  view->Render();

  // Move it, the way dragging it with the mouse would.
  const double moved[2] = { 0.05, 0.05 };
  vtkScalarBarRepresentation* representation = bars->GetWidget(0)->GetScalarBarRepresentation();
  CHECK(representation != nullptr, "the widget has no representation to move");
  representation->SetPosition(moved[0], moved[1]);
  bars->GetWidget(0)->InvokeEvent(vtkCommand::InteractionEvent);
  view->Render();

  const double width = bars->GetActor(0)->GetWidth();
  const double height = bars->GetActor(0)->GetHeight();
  CHECK(SameGeometry(bars->GetActor(0), moved, width, height),
    "stacking took the bar back off the user");

  // Handing the bar back to the renderer must not take it back to the stack.
  bars->DraggableOff();
  view->Render();
  CHECK(SameGeometry(bars->GetActor(0), moved, width, height),
    "the bar snapped back to the stack when dragging was turned off");

  // Nor should picking it up again move it.
  bars->DraggableOn();
  view->Render();
  CHECK(SameGeometry(bars->GetActor(0), moved, width, height),
    "the bar moved when it became draggable a second time");

  bars->DraggableOff();
  view->Render();
  CHECK(SameGeometry(bars->GetActor(0), moved, width, height),
    "the bar did not stay put across a second round trip");

  return EXIT_SUCCESS;
}
}

int TestScivisScalarBars(int, char*[])
{
  if (TestOneBarPerArray() != EXIT_SUCCESS || TestOneBarPerDistinctArray() != EXIT_SUCCESS ||
    TestSuppliedLookupTable() != EXIT_SUCCESS || TestFieldDataHasNoBar() != EXIT_SUCCESS ||
    TestAutoVisibility() != EXIT_SUCCESS || TestSharedManager() != EXIT_SUCCESS ||
    TestDraggable() != EXIT_SUCCESS || TestAMovedBarStaysPut() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
