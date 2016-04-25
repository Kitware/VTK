#ifndef TestAxisActorInternal_h
#define TestAxisActorInternal_h

#include "vtkAxisActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"

inline int TestAxisActorInternal(int use2dMode, int use3dProp)
{
  vtkNew<vtkStringArray> labels;
  labels->SetNumberOfTuples(6);
  labels->SetValue(0, "0");
  labels->SetValue(1, "2");
  labels->SetValue(2, "4");
  labels->SetValue(3, "6");
  labels->SetValue(4, "8");
  labels->SetValue(5, "10");

  vtkNew<vtkTextProperty> textProp1;
  textProp1->SetColor(0., 0., 1.);
  textProp1->SetOpacity(0.9);

  vtkNew<vtkTextProperty> textProp2;
  textProp2->SetColor(1., 0., 0.);
  textProp2->SetOpacity(0.6);

  vtkNew<vtkTextProperty> textProp3;
  textProp3->SetColor(0., 1., 0.);
  textProp3->SetOpacity(1);

  vtkNew<vtkProperty> prop1;
  prop1->SetColor(1., 0., 1.);

  vtkNew<vtkProperty> prop2;
  prop2->SetColor(1., 1., 0.);

  vtkNew<vtkProperty> prop3;
  prop3->SetColor(0., 1., 1.);

  //-------------  X Axis -------------
  vtkNew<vtkAxisActor> axisXActor;
  axisXActor->SetUse2DMode(use2dMode);
  axisXActor->SetUseTextActor3D(use3dProp);
  axisXActor->GetProperty()->SetAmbient(1);
  axisXActor->GetProperty()->SetDiffuse(0);
  axisXActor->SetPoint1(0, 0, 0);
  axisXActor->SetPoint2(10, 0, 0);
  axisXActor->SetTitle("X Axis");
  axisXActor->SetBounds(0, 10, 0, 0, 0, 0);
  axisXActor->SetTickLocationToBoth();
  axisXActor->SetAxisTypeToX();
  axisXActor->SetRange(0, 10);
  axisXActor->SetLabels(labels.Get());
  axisXActor->SetDeltaRangeMajor(2);
  axisXActor->SetDeltaRangeMinor(0.5);
  axisXActor->SetExponent("+00");
  axisXActor->SetExponentVisibility(true);
  axisXActor->SetTitleScale(0.8);
  axisXActor->SetLabelScale(0.5);
  axisXActor->SetTitleOffset(3);
  axisXActor->SetExponentOffset(3);
  axisXActor->SetLabelOffset(5);
  axisXActor->SetTitleTextProperty(textProp1.Get());
  axisXActor->SetLabelTextProperty(textProp2.Get());
  axisXActor->SetAxisMainLineProperty(prop1.Get());
  axisXActor->SetAxisMajorTicksProperty(prop2.Get());
  axisXActor->SetAxisMinorTicksProperty(prop3.Get());

  //-------------  Y Axis -------------
  vtkNew<vtkAxisActor> axisYActor;
  axisYActor->SetUse2DMode(use2dMode);
  axisYActor->SetUseTextActor3D(use3dProp);
  axisYActor->GetProperty()->SetAmbient(1);
  axisYActor->GetProperty()->SetDiffuse(0);
  axisYActor->SetPoint1(0, 0, 0);
  axisYActor->SetPoint2(0, 10, 0);
  axisYActor->SetTitle("Y Axis");
  axisYActor->SetBounds(0, 0, 0, 10, 0, 0);
  axisYActor->SetTickLocationToInside();
  axisYActor->SetAxisTypeToY();
  axisYActor->SetRange(0.1, 500);
  axisYActor->SetMajorRangeStart(0.1);
  axisYActor->SetMinorRangeStart(0.1);
  axisYActor->SetMinorTicksVisible(true);
  axisYActor->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_TOP);
  axisYActor->SetExponent("+00");
  axisYActor->SetExponentVisibility(true);
  axisYActor->SetExponentLocation(vtkAxisActor::VTK_ALIGN_TOP);
  axisYActor->SetTitleScale(0.8);
  axisYActor->SetLabelScale(0.5);
  axisYActor->SetTitleOffset(3);
  axisYActor->SetExponentOffset(5);
  axisYActor->SetLabelOffset(5);
  axisYActor->SetTitleTextProperty(textProp2.Get());
  axisYActor->SetLog(true);
  axisYActor->SetAxisLinesProperty(prop1.Get());

  //-------------  Z Axis -------------
  vtkNew<vtkAxisActor> axisZActor;
  axisZActor->SetUse2DMode(use2dMode);
  axisZActor->SetUseTextActor3D(use3dProp);
  axisZActor->GetProperty()->SetAmbient(1);
  axisZActor->GetProperty()->SetDiffuse(0);
  axisZActor->SetPoint1(0, 0, 0);
  axisZActor->SetPoint2(0, 0, 10);
  axisZActor->SetTitle("Z Axis");
  axisZActor->SetBounds(0, 0, 0, 0, 0, 10);
  axisZActor->SetTickLocationToOutside();
  axisZActor->SetAxisTypeToZ();
  axisZActor->SetRange(0, 10);
  axisZActor->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_POINT2);
  axisZActor->SetExponent("+00");
  axisZActor->SetExponentVisibility(true);
  axisZActor->SetExponentLocation(vtkAxisActor::VTK_ALIGN_POINT1);
  axisZActor->SetTitleScale(0.8);
  axisZActor->SetLabelScale(0.5);
  axisZActor->SetTitleOffset(3);
  axisZActor->SetExponentOffset(3);
  axisZActor->SetLabelOffset(5);
  axisZActor->SetTitleTextProperty(textProp3.Get());
  axisZActor->SetMajorTickSize(3);
  axisZActor->SetMinorTickSize(1);
  axisZActor->SetDeltaRangeMajor(2);
  axisZActor->SetDeltaRangeMinor(0.1);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.Get());
  renderer->AddActor(axisXActor.Get());
  renderer->AddActor(axisYActor.Get());
  renderer->AddActor(axisZActor.Get());
  renderer->SetBackground(.5, .5, .5);

  vtkCamera* camera = renderer->GetActiveCamera();
  axisXActor->SetCamera(camera);
  axisYActor->SetCamera(camera);
  axisZActor->SetCamera(camera);
  renderWindow->SetSize(300, 300);

  camera->SetPosition(-10.0, 22.0, -29);
  camera->SetFocalPoint(-2, 8.5, -9.);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}

#endif
