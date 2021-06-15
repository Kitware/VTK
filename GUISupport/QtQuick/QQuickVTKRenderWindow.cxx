/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QQuickVTKRenderWindow.h"

// vtk includes
#include "QQuickVTKInteractorAdapter.h"
#include "QVTKInteractor.h"
#include "QVTKRenderWindowAdapter.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWindowToImageFilter.h"

// Qt includes
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::QQuickVTKRenderWindow(QQuickItem* parent)
  : Superclass(parent)
{
  vtkNew<vtkGenericOpenGLRenderWindow> renWin;
  this->setRenderWindow(renWin);
  this->m_interactorAdapter = new QQuickVTKInteractorAdapter(this);
  QObject::connect(
    this, &QQuickItem::windowChanged, this, &QQuickVTKRenderWindow::handleWindowChanged);

  // Set a standard object name
  this->setObjectName("QQuickVTKRenderWindow");
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::setupGraphicsBackend()
{
  QSurfaceFormat fmt = QVTKRenderWindowAdapter::defaultFormat(false);
  // By default QtQuick sets the alpha buffer size to 0. We follow the same thing here to prevent a
  // transparent background.
  fmt.setAlphaBufferSize(0);
  QSurfaceFormat::setDefaultFormat(fmt);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
#endif
}

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::~QQuickVTKRenderWindow()
{
  this->m_renderWindow = nullptr;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::sync()
{
  if (!this->isVisible())
  {
    return;
  }

  if (!this->m_renderWindow)
  {
    return;
  }

  QSize windowSize = window()->size() * window()->devicePixelRatio();
  this->m_renderWindow->SetSize(windowSize.width(), windowSize.height());
  if (auto iren = this->m_renderWindow->GetInteractor())
  {
    iren->SetSize(windowSize.width(), windowSize.height());
    m_interactorAdapter->ProcessEvents(iren);
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::init()
{
  if (!this->isVisible())
  {
    return;
  }

  if (!this->m_renderWindow)
  {
    // no render window set, just fill with white.
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT);
    return;
  }

  if (!this->checkGraphicsBackend())
  {
    return;
  }

  auto iren = this->m_renderWindow->GetInteractor();
  if (!this->m_initialized)
  {
    initializeOpenGLFunctions();
    if (iren)
    {
      iren->Initialize();
    }
    this->m_renderWindow->SetMapped(true);
    this->m_renderWindow->SetIsCurrent(true);

    // Since the context is being setup, call OpenGLInitContext
    this->m_renderWindow->SetForceMaximumHardwareLineWidth(1);
    this->m_renderWindow->SetOwnContext(false);
    this->m_renderWindow->OpenGLInitContext();

    // Add a dummy renderer covering the whole size of the render window as a transparent viewport.
    // Without this, the QtQuick rendering is stenciled out.
    this->m_dummyRenderer->InteractiveOff();
    this->m_dummyRenderer->SetLayer(1);
    this->m_renderWindow->AddRenderer(this->m_dummyRenderer);
    this->m_renderWindow->SetNumberOfLayers(2);

    m_initialized = true;
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::paint()
{
  if (!this->isVisible())
  {
    return;
  }

  if (!this->m_renderWindow)
  {
    // no render window set, just fill with white.
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT);
    return;
  }

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
  // Explicitly call init here if using an older Qt version with no
  // beforeRenderPassRecording API available
  this->init();
#endif

  if (!this->checkGraphicsBackend())
  {
    return;
  }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  this->window()->beginExternalCommands();
#endif
  auto iren = this->m_renderWindow->GetInteractor();
  auto ostate = this->m_renderWindow->GetState();
  ostate->Reset();
  ostate->Push();
  // By default, Qt sets the depth function to GL_LESS but VTK expects GL_LEQUAL
  ostate->vtkglDepthFunc(GL_LEQUAL);

  // auto iren = this->m_renderWindow->GetInteractor();
  this->m_renderWindow->SetReadyForRendering(true);
  if (iren)
  {
    iren->Render();
  }
  else
  {
    this->m_renderWindow->Render();
  }

  if (this->m_screenshotScheduled)
  {
    this->m_screenshotFilter->SetInput(this->m_renderWindow);
    this->m_screenshotFilter->SetReadFrontBuffer(false);
    this->m_screenshotFilter->SetInputBufferTypeToRGB();
    this->m_screenshotFilter->Update();
    this->m_screenshotScheduled = false;
  }
  this->m_renderWindow->SetReadyForRendering(false);

  ostate->Pop();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  this->window()->endExternalCommands();
#endif
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::cleanup()
{
  if (this->m_renderWindow)
  {
    this->m_renderWindow->ReleaseGraphicsResources(this->m_renderWindow);
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::handleWindowChanged(QQuickWindow* w)
{
  this->m_interactorAdapter->setQQuickWindow(w);
  if (w)
  {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Do not clear the scenegraph before the QML rendering
    // to preserve the VTK render
    w->setClearBeforeRendering(false);
#endif
    // This allows the cleanup method to be called on the render thread
    w->setPersistentSceneGraph(false);
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::setRenderWindow(vtkRenderWindow* renWin)
{
  auto gwin = vtkGenericOpenGLRenderWindow::SafeDownCast(renWin);
  if (renWin != nullptr && gwin == nullptr)
  {
    qDebug() << "QQuickVTKRenderWindow requires a `vtkGenericOpenGLRenderWindow`. `"
             << renWin->GetClassName() << "` is not supported.";
  }
  this->setRenderWindow(gwin);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::setRenderWindow(vtkGenericOpenGLRenderWindow* renWin)
{
  if (this->m_renderWindow == renWin)
  {
    return;
  }

  this->m_renderWindow = renWin;
  this->m_initialized = false;

  if (this->m_renderWindow)
  {
    this->m_renderWindow->SetMultiSamples(0);
    this->m_renderWindow->SetReadyForRendering(false);
    this->m_renderWindow->SetFrameBlitModeToBlitToHardware();
    vtkNew<QVTKInteractor> iren;
    iren->SetRenderWindow(this->m_renderWindow);

    // now set the default style
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    iren->SetInteractorStyle(style);

    this->m_renderWindow->SetReadyForRendering(false);
  }
}

//-------------------------------------------------------------------------------------------------
vtkRenderWindow* QQuickVTKRenderWindow::renderWindow() const
{
  return this->m_renderWindow;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::mapToViewport(const QRectF& rect, double viewport[4])
{
  viewport[0] = rect.topLeft().x();
  viewport[1] = rect.topLeft().y();
  viewport[2] = rect.bottomRight().x();
  viewport[3] = rect.bottomRight().y();

  if (this->m_renderWindow)
  {
    int* windowSize = this->m_renderWindow->GetSize();
    if (windowSize && windowSize[0] != 0 && windowSize[1] != 0)
    {
      viewport[0] = viewport[0] / (windowSize[0] - 1.0);
      viewport[1] = viewport[1] / (windowSize[1] - 1.0);
      viewport[2] = viewport[2] / (windowSize[0] - 1.0);
      viewport[3] = viewport[3] / (windowSize[1] - 1.0);
    }
  }

  // Change to quadrant I (vtk) from IV (Qt)
  double tmp = 1.0 - viewport[1];
  viewport[1] = 1.0 - viewport[3];
  viewport[3] = tmp;

  for (int i = 0; i < 3; ++i)
  {
    viewport[i] = viewport[i] > 0.0 ? viewport[i] : 0.0;
    viewport[i] = viewport[i] > 1.0 ? 1.0 : viewport[i];
  }
}

//-------------------------------------------------------------------------------------------------
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void QQuickVTKRenderWindow::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
#else
void QQuickVTKRenderWindow::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
#endif
{
  m_interactorAdapter->QueueGeometryChanged(newGeometry, oldGeometry);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  Superclass::geometryChanged(newGeometry, oldGeometry);
#else
  Superclass::geometryChange(newGeometry, oldGeometry);
#endif
}

//-------------------------------------------------------------------------------------------------
QPointer<QQuickVTKInteractorAdapter> QQuickVTKRenderWindow::interactorAdapter() const
{
  return this->m_interactorAdapter;
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> QQuickVTKRenderWindow::captureScreenshot()
{
  double viewport[4] = { 0, 0, 1, 1 };
  return this->captureScreenshot(viewport);
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> QQuickVTKRenderWindow::captureScreenshot(double* viewport)
{
  if (!this->window())
  {
    return nullptr;
  }
  this->m_screenshotScheduled = true;
  this->m_screenshotFilter->SetViewport(viewport);
  this->renderNow();
  return this->m_screenshotFilter->GetOutput();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::renderNow()
{
  if (!this->window())
  {
    return;
  }
  // Schedule a scenegraph update
  this->window()->update();
  // Wait for the update to complete
  QEventLoop loop;
  QObject::connect(this->window(), &QQuickWindow::afterRendering, &loop, &QEventLoop::quit);
  loop.exec();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::render()
{
  if (this->window())
  {
    this->window()->update();
  }
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKRenderWindow::isInitialized() const
{
  return this->m_initialized;
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKRenderWindow::checkGraphicsBackend()
{
  // Enforce the use of OpenGL API
  QSGRendererInterface* rif = this->window()->rendererInterface();
  auto gApi = rif->graphicsApi();
  if (!(gApi == QSGRendererInterface::OpenGL
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        || gApi == QSGRendererInterface::OpenGLRhi
#endif
        ))
  {
    qCritical(R"***(Error: QtQuick scenegraph is using an unsupported graphics API: %d.
Set the QSG_INFO environment variable to get more information.
Use QQuickVTKRenderWindow::setupGraphicsBackend() to set the right backend.)***",
      gApi);
    return false;
  }
  return true;
}
