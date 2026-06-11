// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verifies that vtkBatchedLabeledDataMapper renders integer ID labels correctly
// when using world-coordinate mode. Uses a sphere source so that labels track
// 3D positions as the camera orbits the geometry. Exercises CoordinateSystemWorld(),
// SetLabelModeToLabelIds(), two text property styles, and the vtkTransform path
// inherited from vtkLabeledDataMapper. Also exercises the ReleaseGraphicsResources /
// re-render path.

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkBatchedLabeledDataMapper.h"
#include "vtkCamera.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

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

int TestBatchedLabeledDataMapperWorldCoords(int argc, char* argv[])
{
  // ---- build a sphere with 20 visible labeled points --------------------------
  // ThetaResolution=5, PhiResolution=4 gives exactly 20 surface points.
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetThetaResolution(5);
  sphere->SetPhiResolution(4);
  sphere->Update();

  auto* dataset = sphere->GetOutput();
  int numPoints = dataset->GetNumberOfPoints();

  // Assign a type array so we can exercise two text property styles
  vtkNew<vtkIntArray> types;
  types->SetName("types");
  types->SetNumberOfComponents(1);
  for (int i = 0; i < numPoints; i++)
  {
    types->InsertNextValue(i % 2);
  }
  dataset->GetPointData()->AddArray(types);

  // ---- label mapper -----------------------------------------------------------
  vtkNew<vtkBatchedLabeledDataMapper> labelMapper;
  labelMapper->CoordinateSystemWorld();
  labelMapper->SetLabelModeToLabelIds();
  labelMapper->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "types");
  labelMapper->SetInputData(dataset);

  // Two text property styles
  AddTextProperty(labelMapper, 0, VTK_ARIAL, 14, 2, { 1., 1., 1., 1. }, { 0., 0., 0., 0.8 },
    { 0.8, 0.8, 0.8, 1. });
  AddTextProperty(
    labelMapper, 1, VTK_TIMES, 14, 0, { 1., 1., 0., 1. }, { 0., 0., 0.4, 0.9 }, { 1., 1., 0., 1. });

  // Center anchor with a small upward nudge
  int offset[2] = { 0, 3 };
  labelMapper->SetDisplayOffset(offset);
  labelMapper->SetTextAnchor(vtkBatchedLabeledDataMapper::Center);

  // Exercise the vtkTransform path: apply a mild rotation to label positions
  vtkNew<vtkTransform> labelTransform;
  labelTransform->RotateZ(15.0);
  labelMapper->SetTransform(labelTransform);

  vtkNew<vtkActor2D> labelActor;
  labelActor->SetMapper(labelMapper);

  // ---- geometry actor ---------------------------------------------------------
  vtkNew<vtkPolyDataMapper> geomMapper;
  geomMapper->SetInputData(dataset);
  vtkNew<vtkActor> geomActor;
  geomActor->SetMapper(geomMapper);
  geomActor->GetProperty()->SetColor(0.4, 0.5, 0.7);
  geomActor->GetProperty()->SetOpacity(0.6);

  // ---- renderer ---------------------------------------------------------------
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.1, 0.15);
  ren->AddActor(geomActor);
  ren->AddViewProp(labelActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  // Orbit slightly so labels at different 3D positions are clearly separated
  ren->GetActiveCamera()->Azimuth(30);
  ren->GetActiveCamera()->Elevation(20);
  ren->ResetCameraClippingRange();

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
