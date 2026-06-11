// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verifies that vtkBatchedLabeledDataMapper renders text labels correctly
// when using display-coordinate mode. Exercises CoordinateSystemDisplay(),
// DisplayOffset, TextAnchor, and 5 different text styles, then does a
// regression image comparison. Also exercises the ReleaseGraphicsResources /
// re-render path.

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkBatchedLabeledDataMapper.h"
#include "vtkCamera.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTextProperty.h"

namespace
{

void AddTextProperty(vtkBatchedLabeledDataMapper* mapper, int idx, int fontFamily, int fontSize,
  int frameWidth, std::array<double, 4> color, std::array<double, 4> bgColor,
  std::array<double, 4> frameColor)
{
  vtkNew<vtkTextProperty> tp;
  tp->SetFontFamily(fontFamily);
  tp->SetFontSize(fontSize);
  tp->SetColor(color.data());
  tp->SetOpacity(color[3]);
  tp->SetBackgroundColor(bgColor.data());
  tp->SetBackgroundOpacity(bgColor[3]);
  tp->SetFrame(frameWidth > 0);
  tp->SetFrameWidth(frameWidth);
  tp->SetFrameColor(frameColor.data());
  mapper->SetLabelTextProperty(tp, idx);
}

} // namespace

int TestBatchedLabeledDataMapperDisplayCoords(int argc, char* argv[])
{
  // ---- build a small grid of labeled points -----------------------------------
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(5, 5);
  plane->Update();

  auto* dataset = plane->GetOutput();
  int numPoints = dataset->GetNumberOfPoints();

  vtkNew<vtkStringArray> labels;
  labels->SetName("labels");
  vtkNew<vtkIntArray> types;
  types->SetName("types");
  types->SetNumberOfComponents(1);

  for (int i = 0; i < numPoints; i++)
  {
    labels->InsertNextValue(vtk::format("L{:d}", i));
    types->InsertNextValue(i % 5);
  }

  dataset->GetPointData()->AddArray(labels);
  dataset->GetPointData()->AddArray(types);

  // ---- label mapper -----------------------------------------------------------
  vtkNew<vtkBatchedLabeledDataMapper> labelMapper;
  labelMapper->CoordinateSystemDisplay();
  labelMapper->SetLabelModeToLabelFieldData();
  labelMapper->SetFieldDataName("labels");
  labelMapper->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "types");
  labelMapper->SetInputData(dataset);

  // Five distinct text styles
  AddTextProperty(
    labelMapper, 0, VTK_ARIAL, 14, 0, { 1., 1., 1., 1. }, { 0., 0., 0., 0.8 }, { 0., 0., 0., 1. });
  AddTextProperty(
    labelMapper, 1, VTK_TIMES, 16, 2, { 1., 1., 0., 1. }, { 0., 0., 0.5, 0.9 }, { 1., 1., 0., 1. });
  AddTextProperty(labelMapper, 2, VTK_COURIER, 14, 3, { 0., 1., 1., 1. }, { 0.3, 0., 0.3, 0.9 },
    { 0., 1., 1., 1. });
  AddTextProperty(labelMapper, 3, VTK_ARIAL, 12, 1, { 1., 0.5, 0., 1. }, { 0., 0.2, 0., 0.8 },
    { 1., 0.5, 0., 1. });
  AddTextProperty(labelMapper, 4, VTK_TIMES, 18, 4, { 0.8, 0.8, 1., 1. }, { 0.1, 0.1, 0., 0.9 },
    { 0.5, 0.5, 1., 1. });

  // Nudge labels 4 pixels right and 4 pixels up from anchor
  int offset[2] = { 4, 4 };
  labelMapper->SetDisplayOffset(offset);
  labelMapper->SetTextAnchor(vtkBatchedLabeledDataMapper::LowerLeft);

  vtkNew<vtkActor2D> labelActor;
  labelActor->SetMapper(labelMapper);

  // ---- geometry actor (so we can see where the points are) --------------------
  vtkNew<vtkPolyDataMapper> geomMapper;
  geomMapper->SetInputData(dataset);
  vtkNew<vtkActor> geomActor;
  geomActor->SetMapper(geomMapper);
  geomActor->GetProperty()->SetRepresentationToPoints();
  geomActor->GetProperty()->SetPointSize(4);
  geomActor->GetProperty()->SetColor(0.6, 0.6, 0.6);

  // ---- renderer ---------------------------------------------------------------
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.15, 0.15, 0.2);
  ren->AddActor(geomActor);
  ren->AddViewProp(labelActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();
  // Exercise ReleaseGraphicsResources / re-render path
  labelMapper->ReleaseGraphicsResources(renWin);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
