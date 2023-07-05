// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Tests QQuickVTKItem

#include "QQuickVTKItem.h"
#include "TestQQuickCommon.h"
#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPNGWriter.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"

#include <QApplication>

namespace
{
struct MyConeItem : QQuickVTKItem
{
  struct Data : vtkObject
  {
    static Data* New();
    vtkTypeMacro(Data, vtkObject);
  };

  void onEndEvent(vtkObject* caller, unsigned long, void*)
  {
    vtkRenderWindow* renderWindow = vtkRenderWindow::SafeDownCast(caller);
    renderWindow->GetRenderers()->GetFirstRenderer()->ResetCamera();
    renderWindow->RemoveObserver(this->endEventTag);
    this->scheduleRender();
  }

  vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override
  {
    auto vtk = vtkNew<Data>();

    // Create a cone pipeline and add it to the view
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkActor> actor;
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkConeSource> cone;
    renderWindow->AddRenderer(renderer);
    mapper->SetInputConnection(cone->GetOutputPort());
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
    renderer->ResetCamera();
    renderer->SetBackground2(0.7, 0.7, 0.7);
    renderer->SetGradientBackground(true);

    endEventTag = renderWindow->AddObserver(vtkCommand::EndEvent, this, &MyConeItem::onEndEvent);

    return vtk;
  }

  unsigned long endEventTag;
};
vtkStandardNewMacro(MyConeItem::Data);

/*=========================================================================*/

struct MyWidgetItem : QQuickVTKItem
{
  struct Data : vtkObject
  {
    static Data* New();
    vtkTypeMacro(Data, vtkObject);

    vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  };

  struct Callback
  {
    void Execute(vtkObject*, unsigned long evt, void*)
    {
      if (evt == vtkCommand::InteractionEvent)
      {
        this->Rep->GetPlane(this->Plane);
        this->Actor->VisibilityOn();
      }

      if (evt == vtkCommand::EndEvent)
      {
        // Once the application is up, adjust the camera, widget reps, etc.
        this->Renderer->ResetCamera();
        this->Rep->SetPlaceFactor(1.25);
        this->Rep->PlaceWidget(this->Glyph->GetOutput()->GetBounds());
        this->Renderer->GetActiveCamera()->Azimuth(20);
        this->Renderer->GetRenderWindow()->RemoveObserver(this->EndEventTag);
        this->pThis->scheduleRender();
      }
    }
    Callback()
      : Plane(nullptr)
      , Actor(nullptr)
    {
    }
    vtkPlane* Plane;
    vtkActor* Actor;
    vtkGlyph3D* Glyph;
    vtkRenderer* Renderer;
    vtkImplicitPlaneRepresentation* Rep;
    MyWidgetItem* pThis;
    unsigned long EndEventTag;
  };

  vtkUserData initializeVTK(vtkRenderWindow* renderWindow) override
  {
    auto vtk = vtkNew<Data>();

    vtkNew<vtkRenderer> renderer;
    renderWindow->AddRenderer(renderer);

    // Create a mace out of filters.
    //
    vtkNew<vtkSphereSource> sphere;
    vtkNew<vtkGlyph3D> glyph;
    vtkNew<vtkConeSource> cone;
    glyph->SetInputConnection(sphere->GetOutputPort());
    glyph->SetSourceConnection(cone->GetOutputPort());
    glyph->SetVectorModeToUseNormal();
    glyph->SetScaleModeToScaleByVector();
    glyph->SetScaleFactor(0.25);

    // The sphere and spikes are appended into a single polydata.
    // This just makes things simpler to manage.
    vtkNew<vtkAppendPolyData> apd;
    apd->AddInputConnection(glyph->GetOutputPort());
    apd->AddInputConnection(sphere->GetOutputPort());

    vtkNew<vtkPolyDataMapper> maceMapper;
    maceMapper->SetInputConnection(apd->GetOutputPort());

    vtkNew<vtkActor> maceActor;
    maceActor->SetMapper(maceMapper);
    maceActor->VisibilityOn();

    // This portion of the code clips the mace with the vtkPlanes
    // implicit function. The clipped region is colored green.
    vtkNew<vtkPlane> plane;
    vtkNew<vtkClipPolyData> clipper;
    clipper->SetInputConnection(apd->GetOutputPort());
    clipper->SetClipFunction(plane);
    clipper->InsideOutOn();

    vtkNew<vtkPolyDataMapper> selectMapper;
    selectMapper->SetInputConnection(clipper->GetOutputPort());

    vtkNew<vtkActor> selectActor;
    selectActor->SetMapper(selectMapper);
    selectActor->GetProperty()->SetColor(0, 1, 0);
    selectActor->VisibilityOff();
    selectActor->SetScale(1.01, 1.01, 1.01);

    vtkNew<vtkImplicitPlaneRepresentation> rep;

    // The SetInteractor method is how 3D widgets are associated with the render
    // window interactor. Internally, SetInteractor sets up a bunch of callbacks
    // using the Command/Observer mechanism (AddObserver()).
    myCallback.Plane = plane;
    myCallback.Actor = selectActor;
    myCallback.Glyph = glyph;
    myCallback.Rep = rep;
    myCallback.Renderer = renderer;
    myCallback.pThis = this;

    vtk->planeWidget->SetRepresentation(rep);
    vtk->planeWidget->AddObserver(vtkCommand::InteractionEvent, &myCallback, &Callback::Execute);
    myCallback.EndEventTag = renderer->GetRenderWindow()->AddObserver(
      vtkCommand::EndEvent, &myCallback, &Callback::Execute);
    auto iren = renderWindow->GetInteractor();
    vtk->planeWidget->SetInteractor(iren);
    vtk->planeWidget->SetCurrentRenderer(renderer);
    vtk->planeWidget->SetEnabled(1);
    vtk->planeWidget->SetProcessEvents(1);

    renderer->AddActor(maceActor);
    renderer->AddActor(selectActor);

    return vtk;
  }

  Callback myCallback;
};
vtkStandardNewMacro(MyWidgetItem::Data);
}

int TestQQuickVTKItem_2(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  QQuickVTKItem::setGraphicsApi();
  QApplication app(argc, argv);

  qmlRegisterType<MyConeItem>("Vtk", 1, 0, "MyConeItem");
  qmlRegisterType<MyWidgetItem>("Vtk", 1, 0, "MyWidgetItem");

  return detail::performTest(argc, argv, "qrc:///TestQQuickVTKItem_2.qml");
}
