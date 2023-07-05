// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Tests QQuickVTKRenderWindow/QQuickVTKRenderItem

// VTK_DEPRECATED_IN_9_3_0 applies to the classes tested here
#define VTK_DEPRECATION_LEVEL 0

#include "QQuickVTKRenderItem.h"
#include "QQuickVTKRenderWindow.h"
#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h"
#include "vtkPNGWriter.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkWindowToImageFilter.h"
#include "vtkXMLImageDataReader.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

int TestQQuickVTKRenderWindow(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  QQuickVTKRenderWindow::setupGraphicsBackend();
  QApplication app(argc, argv);

  QQmlApplicationEngine engine;
  qDebug() << "QML2_IMPORT_PATH:" << engine.importPathList();
  engine.load(QUrl("qrc:///TestQQuickVTKRenderWindow.qml"));

  QObject* topLevel = engine.rootObjects().value(0);
  QQuickWindow* window = qobject_cast<QQuickWindow*>(topLevel);

  window->show();

  // Fetch the QQuick window using the standard object name set up in the constructor
  QQuickVTKRenderItem* geomItem = topLevel->findChild<QQuickVTKRenderItem*>("GeomView");

  // Create a cone pipeline and add it to the view
  vtkNew<vtkActor> actor;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkConeSource> cone;
  mapper->SetInputConnection(cone->GetOutputPort());
  actor->SetMapper(mapper);
  geomItem->renderer()->AddActor(actor);
  geomItem->renderer()->ResetCamera();
  // geomItem->renderer()->SetBackground(0.5, 0.5, 0.7);
  geomItem->renderer()->SetBackground2(0.7, 0.7, 0.7);
  // geomItem->renderer()->SetGradientBackground(true);
  geomItem->update();

  // Now the volume view
  QQuickVTKRenderItem* volumeItem = topLevel->findChild<QQuickVTKRenderItem*>("VolumeView");

  // Create a volume pipeline and add it to the view
  vtkNew<vtkSmartVolumeMapper> volumeMapper;
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  delete[] volumeFile;
  double scalarRange[2];
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetAutoAdjustSampleDistances(1);
  volumeMapper->SetBlendModeToComposite();
  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 0.09);
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);
  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);
  // colorTransferFunction->AddRGBPoint(scalarRange[1], 0.2, 0.1, 0.3);
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  volumeItem->renderer()->AddVolume(volume);
  volumeItem->renderer()->ResetCamera();
  // volumeItem->renderer()->SetBackground(0.5, 0.5, 0.7);
  volumeItem->renderer()->SetBackground(0, 0, 1);
  // volumeItem->renderer()->SetBackground2(0.7, 0.7, 0.7);
  volumeItem->update();

  // Now the glyph view
  QQuickVTKRenderItem* glyphItem = topLevel->findChild<QQuickVTKRenderItem*>("GlyphView");

  // Create the glyph pipeline
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkGlyph3DMapper> glyphMapper;
  vtkNew<vtkConeSource> squad;
  glyphMapper->SetInputConnection(sphere->GetOutputPort());
  glyphMapper->SetSourceConnection(squad->GetOutputPort());
  glyphMapper->SetOrientationArray("Normals");
  vtkNew<vtkActor> glyphActor;
  glyphActor->SetMapper(glyphMapper);
  glyphActor->GetProperty()->SetDiffuseColor(0.5, 1.0, 0.8);
  glyphItem->renderer()->AddActor(glyphActor);
  glyphItem->renderer()->SetBackground(0.5, 0.5, 0.7);
  glyphItem->renderer()->ResetCamera();
  glyphItem->update();

  // Now the testing
  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);
  if (vtktesting->IsInteractiveModeSpecified())
  {
    return QApplication::exec();
  }

  // Wait a little for the application and window to be set up properly
  QEventLoop loop;
  QTimer::singleShot(100, &loop, SLOT(quit()));
  loop.exec();

  // Capture a screenshot of the window
  // Fetch the QQuick window using the standard object name set up in the constructor
  QQuickVTKRenderWindow* qquickvtkWindow =
    topLevel->findChild<QQuickVTKRenderWindow*>("QQuickVTKRenderWindow");
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
