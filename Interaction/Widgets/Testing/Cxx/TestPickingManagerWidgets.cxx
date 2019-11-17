/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPickingManagerWidgets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*==============================================================================

  Library: MSVTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

//
// This example tests the PickingManager using different widgets and associated
// pickers:
// * vtkBalloonWidget
// * vtkBoxWidget
// * vtkImplicitPlaneWidget2
// By default the Picking Manager is enabled.
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to enable/disable the Optimization on render events.

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkBalloonRepresentation.h"
#include "vtkBalloonWidget.h"
#include "vtkBoxWidget.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropPicker.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

//------------------------------------------------------------------------------
class vtkBalloonPickCallback : public vtkCommand
{
public:
  static vtkBalloonPickCallback* New() { return new vtkBalloonPickCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkPropPicker* picker = reinterpret_cast<vtkPropPicker*>(caller);
    vtkProp* prop = picker->GetViewProp();
    if (prop != nullptr)
    {
      this->BalloonWidget->UpdateBalloonString(prop, "Picked");
    }
  }

  vtkBalloonWidget* BalloonWidget;
};

//------------------------------------------------------------------------------
// Updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPW2Callback : public vtkCommand
{
public:
  static vtkTIPW2Callback* New() { return new vtkTIPW2Callback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitPlaneWidget2* planeWidget = reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
    vtkImplicitPlaneRepresentation* rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }

  vtkTIPW2Callback()
    : Plane(nullptr)
    , Actor(nullptr)
  {
  }
  vtkPlane* Plane;
  vtkActor* Actor;
};

//------------------------------------------------------------------------------
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to switch the activation of the optimization based on the render
// events.
class vtkEnableManagerCallback : public vtkCommand
{
public:
  static vtkEnableManagerCallback* New() { return new vtkEnableManagerCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);

    if ((vtkStdString(iren->GetKeySym()) == "Control_L" ||
          vtkStdString(iren->GetKeySym()) == "Control_R") &&
      iren->GetPickingManager())
    {
      if (!iren->GetPickingManager()->GetEnabled())
      {
        std::cout << "PickingManager ON !" << std::endl;
        iren->GetPickingManager()->EnabledOn();
      }
      else
      {
        std::cout << "PickingManager OFF !" << std::endl;
        iren->GetPickingManager()->EnabledOff();
      }
    }
    // Enable/Disable the Optimization on render events.
    else if (vtkStdString(iren->GetKeySym()) == "o" && iren->GetPickingManager())
    {
      if (!iren->GetPickingManager()->GetOptimizeOnInteractorEvents())
      {
        std::cout << "Optimization on Interactor events ON !" << std::endl;
        iren->GetPickingManager()->SetOptimizeOnInteractorEvents(1);
      }
      else
      {
        std::cout << "Optimization on Interactor events OFF !" << std::endl;
        iren->GetPickingManager()->SetOptimizeOnInteractorEvents(0);
      }
    }
  }

  vtkEnableManagerCallback() = default;
};

//------------------------------------------------------------------------------
// Test Picking Manager with a several widgets
//------------------------------------------------------------------------------
int TestPickingManagerWidgets(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleTrackballCamera> irenStyle;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(irenStyle);

  // Instantiate a picker and link it to the ballonWidgetCallback
  vtkNew<vtkPropPicker> picker;
  vtkNew<vtkBalloonPickCallback> pcbk;
  picker->AddObserver(vtkCommand::PickEvent, pcbk);
  iren->SetPicker(picker);

  /*--------------------------------------------------------------------------*/
  // PICKING MANAGER
  /*--------------------------------------------------------------------------*/
  // Callback to switch between the managed and non-managed mode of the
  // Picking Manager
  vtkNew<vtkEnableManagerCallback> callMode;
  iren->AddObserver(vtkCommand::KeyPressEvent, callMode);

  /*--------------------------------------------------------------------------*/
  // BALLOON WIDGET
  /*--------------------------------------------------------------------------*/
  // Create a test pipeline
  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> sph;
  sph->SetMapper(mapper);

  vtkNew<vtkCylinderSource> cs;
  vtkNew<vtkPolyDataMapper> csMapper;
  csMapper->SetInputConnection(cs->GetOutputPort());
  vtkNew<vtkActor> cyl;
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5, 0, 0);

  vtkNew<vtkConeSource> coneSource;
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  vtkNew<vtkActor> cone;
  cone->SetMapper(coneMapper);
  cone->AddPosition(0, 5, 0);

  // Create the widget
  vtkNew<vtkBalloonRepresentation> rep;
  rep->SetBalloonLayoutToImageRight();

  vtkNew<vtkBalloonWidget> widget;
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  widget->AddBalloon(sph, "This is a sphere", nullptr);
  widget->AddBalloon(cyl, "This is a\ncylinder", nullptr);
  widget->AddBalloon(cone, "This is a\ncone,\na really big.", nullptr);
  pcbk->BalloonWidget = widget;

  /*--------------------------------------------------------------------------*/
  // BOX WIDGET
  /*--------------------------------------------------------------------------*/
  vtkNew<vtkBoxWidget> boxWidget;
  boxWidget->SetInteractor(iren);
  boxWidget->SetPlaceFactor(1.25);

  // Create the mass actor
  vtkNew<vtkConeSource> cone1;
  cone1->SetResolution(6);
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  sphere->SetCenter(5, 5, 0);
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceData(cone1->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkNew<vtkAppendPolyData> append;
  append->AddInputData(glyph->GetOutput());
  append->AddInputData(sphere->GetOutput());

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(append->GetOutputPort());

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper);

  /*--------------------------------------------------------------------------*/
  // Multiple ImplicitPlane Widgets
  /*--------------------------------------------------------------------------*/
  // Create a mace out of filters.
  //
  vtkNew<vtkSphereSource> sphereImpPlane;
  vtkNew<vtkConeSource> coneImpPlane;
  vtkNew<vtkGlyph3D> glyphImpPlane;
  glyphImpPlane->SetInputConnection(sphereImpPlane->GetOutputPort());
  glyphImpPlane->SetSourceConnection(coneImpPlane->GetOutputPort());
  glyphImpPlane->SetVectorModeToUseNormal();
  glyphImpPlane->SetScaleModeToScaleByVector();
  glyphImpPlane->SetScaleFactor(0.25);
  glyphImpPlane->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apdImpPlane;
  apdImpPlane->AddInputData(glyphImpPlane->GetOutput());
  apdImpPlane->AddInputData(sphereImpPlane->GetOutput());

  vtkNew<vtkPolyDataMapper> maceMapperImpPlane;
  maceMapperImpPlane->SetInputConnection(apdImpPlane->GetOutputPort());

  vtkNew<vtkActor> maceActorImpPlane;
  maceActorImpPlane->SetMapper(maceMapperImpPlane);
  maceActorImpPlane->AddPosition(0, 0, 0);
  maceActorImpPlane->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkNew<vtkPlane> plane;
  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(apdImpPlane->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkActor> selectActor;
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0, 1, 0);
  selectActor->VisibilityOff();
  selectActor->AddPosition(0, 0, 0);
  selectActor->SetScale(1.01, 1.01, 1.01);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkTIPW2Callback> impPlaneCallback;
  impPlaneCallback->Plane = plane;
  impPlaneCallback->Actor = selectActor;

  // First ImplicitPlaneWidget (Green)
  vtkNew<vtkImplicitPlaneRepresentation> impPlaneRep;
  impPlaneRep->SetPlaceFactor(1.);
  impPlaneRep->SetOutlineTranslation(0);
  impPlaneRep->SetScaleEnabled(0);
  impPlaneRep->PlaceWidget(glyphImpPlane->GetOutput()->GetBounds());
  impPlaneRep->SetEdgeColor(0., 1., 0.);
  impPlaneRep->SetNormal(1, 0, 1);

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(impPlaneRep);
  planeWidget->On();

  planeWidget->AddObserver(vtkCommand::InteractionEvent, impPlaneCallback);
  planeWidget->AddObserver(vtkCommand::UpdateEvent, impPlaneCallback);

  // Second ImplicitPlaneWidget (Red)
  vtkNew<vtkImplicitPlaneRepresentation> impPlaneRep2;
  impPlaneRep2->SetOutlineTranslation(0);
  impPlaneRep2->SetScaleEnabled(0);
  impPlaneRep2->SetPlaceFactor(1.);
  impPlaneRep2->PlaceWidget(glyphImpPlane->GetOutput()->GetBounds());
  impPlaneRep2->SetEdgeColor(1., 0., 0.);

  vtkNew<vtkImplicitPlaneWidget2> planeWidget2;
  planeWidget2->SetInteractor(iren);
  planeWidget2->SetRepresentation(impPlaneRep2);
  planeWidget2->On();

  /*--------------------------------------------------------------------------*/
  // Rendering
  /*--------------------------------------------------------------------------*/
  // Add the actors to the renderer, set the background and size
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->AddActor(maceActorImpPlane);
  ren1->AddActor(selectActor);
  ren1->AddActor(maceActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);

  // Configure the box widget
  boxWidget->SetProp3D(maceActor);
  boxWidget->PlaceWidget();

  // render the image
  iren->Initialize();
  double extent[6] = { -2, 7, -2, 7, -1, 1 };
  ren1->ResetCamera(extent);
  renWin->Render();
  widget->On();
  boxWidget->On();
  iren->Start();

  return EXIT_SUCCESS;
}
