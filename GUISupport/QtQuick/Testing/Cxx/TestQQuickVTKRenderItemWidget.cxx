// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Tests QQuickVTKRenderItem

// VTK_DEPRECATED_IN_9_3_0 applies to the classes tested here
#define VTK_DEPRECATION_LEVEL 0

#include "QQuickVTKInteractiveWidget.h"
#include "QQuickVTKRenderItem.h"
#include "QQuickVTKRenderWindow.h"
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
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

class TestQQuickVTKRenderItemWidgetCallback : public vtkCommand
{
public:
  static TestQQuickVTKRenderItemWidgetCallback* New()
  {
    return new TestQQuickVTKRenderItemWidgetCallback;
  }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitPlaneWidget2* planeWidget = reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
    vtkImplicitPlaneRepresentation* rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }
  TestQQuickVTKRenderItemWidgetCallback()
    : Plane(nullptr)
    , Actor(nullptr)
  {
  }
  vtkPlane* Plane;
  vtkActor* Actor;
};

int TestQQuickVTKRenderItemWidget(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  QQuickVTKRenderWindow::setupGraphicsBackend();
  QApplication app(argc, argv);

  QQmlApplicationEngine engine;
  qDebug() << "QML2_IMPORT_PATH:" << engine.importPathList();
  engine.load(QUrl("qrc:///TestQQuickVTKRenderItemWidget.qml"));

  QObject* topLevel = engine.rootObjects().value(0);
  QQuickWindow* window = qobject_cast<QQuickWindow*>(topLevel);

  window->show();

  // Fetch the QQuick window using the standard object name set up in the constructor
  QQuickVTKRenderWindow* qquickvtkWindow =
    topLevel->findChild<QQuickVTKRenderWindow*>("QQuickVTKRenderWindow");

  // Fetch the QQuick item using the object name set up in the qml file
  QQuickVTKRenderItem* coneItem = topLevel->findChild<QQuickVTKRenderItem*>("ConeView");
  // Create a cone pipeline and add it to the view
  vtkNew<vtkActor> actor;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkConeSource> cone;
  mapper->SetInputConnection(cone->GetOutputPort());
  actor->SetMapper(mapper);
  coneItem->renderer()->AddActor(actor);
  coneItem->renderer()->SetBackground(0.5, 0.5, 0.7);
  coneItem->renderer()->SetBackground2(0.7, 0.7, 0.7);
  coneItem->renderer()->SetGradientBackground(true);
  coneItem->update();

  // Fetch the QQuick item using the object name set up in the qml file
  QQuickVTKRenderItem* widgetItem = topLevel->findChild<QQuickVTKRenderItem*>("WidgetView");
  // Create a mace out of filters.
  //
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkGlyph3D> glyph;
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

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<TestQQuickVTKRenderItemWidgetCallback> myCallback;
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkNew<vtkImplicitPlaneRepresentation> rep;
  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetRepresentation(rep);
  planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  QQuickVTKInteractiveWidget* qquickVTKWidget = new QQuickVTKInteractiveWidget(window);
  qquickVTKWidget->setWidget(planeWidget);
  qquickVTKWidget->setEnabled(true);

  widgetItem->renderer()->AddActor(maceActor);
  widgetItem->renderer()->AddActor(selectActor);
  widgetItem->addWidget(qquickVTKWidget);
  widgetItem->update();

  // Wait a little for the application and window to be set up properly
  QEventLoop loop;
  QTimer::singleShot(100, &loop, SLOT(quit()));
  loop.exec();

  // Once the application is up, adjust the camera, widget reps, etc.
  widgetItem->renderer()->ResetCamera();
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());
  widgetItem->renderer()->GetActiveCamera()->Azimuth(20);

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);
  if (vtktesting->IsInteractiveModeSpecified())
  {
    return QApplication::exec();
  }

  // Capture a screenshot of the item
  vtkSmartPointer<vtkImageData> im = qquickvtkWindow->captureScreenshot();

  std::string validName = std::string(vtktesting->GetValidImageFileName());
  std::string::size_type slashPos = validName.rfind('/');
  if (slashPos != std::string::npos)
  {
    validName = validName.substr(slashPos + 1);
  }
  std::string tmpDir = vtktesting->GetTempDirectory();
  std::string vImage = tmpDir + "/" + validName;
  vtkNew<vtkPNGWriter> w;
  w->SetInputData(im);
  w->SetFileName(vImage.c_str());
  w->Write();

  int retVal = vtktesting->RegressionTest(vImage, 10);

  switch (retVal)
  {
    case vtkTesting::FAILED:
    case vtkTesting::NOT_RUN:
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
