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
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"

// Qt includes
#include <QQuickWindow>

#include "vtkRenderWindow.h"

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::QQuickVTKRenderWindow(QQuickItem* parent)
  : Superclass(parent)
{
  // Accept mouse events
  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::AllButtons);

  this->setRenderWindow(vtkGenericOpenGLRenderWindow::New());
  this->m_interactorAdapter = new QQuickVTKInteractorAdapter(this);
  QObject::connect(
    this, &QQuickItem::windowChanged, this, &QQuickVTKRenderWindow::handleWindowChanged);
}

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::~QQuickVTKRenderWindow() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::sync()
{
  if (!this->isVisible())
  {
    return;
  }

  QSize windowSize = window()->size() * window()->devicePixelRatio();
  this->m_renderWindow->SetSize(windowSize.width(), windowSize.height());

  //  QRectF rect(this->position().x(), this->position().y(), this->width(), this->height());
  //  rect = mapRectToScene(rect);
  //  this->m_renderWindow->SetPosition(rect.topRight().x(), rect.bottomLeft().y());
  //  this->m_renderWindow->SetSize(rect.width(), rect.height());
  // this->m_renderWindow->SetSize(this->width(), this->height());
  // this->m_renderWindow->SetPosition(this->position().x(), this->position().y());

  //// Explicitly set the viewport for each render
  // QRectF rect(0, 0, this->width(), this->height());
  // rect = mapRectToScene(rect);
  // this->setViewport(rect);

  m_interactorAdapter->ProcessEvents(this->m_renderWindow->GetInteractor());
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::paint()
{
  if (!this->isVisible())
  {
    return;
  }

  // no render window set, just fill with white.
  //    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
  //    f->glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
  //    f->glClear(GL_COLOR_BUFFER_BIT);
  if (!m_initialized)
  {
    initializeOpenGLFunctions();
    vtkRenderWindowInteractor* interactor = this->m_renderWindow->GetInteractor();
    interactor->Initialize();
    this->m_renderWindow->SetMapped(1);
    this->m_renderWindow->SetIsCurrent(1);

    m_initialized = true;
  }

  glUseProgram(0);
  this->m_renderWindow->OpenGLInit();
  //   QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
  //    // QOpenGLExtraFunctions* f = this->Context->extraFunctions();
  //    f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  //    const GLenum bufs[1] = { static_cast<GLenum>(GL_COLOR_ATTACHMENT0) };
  //    f->glDrawBuffers(1, bufs);
  //    this->m_renderWindow->BlitDisplayFramebuffer(0 ? 0 : 1, 0, 0, 200, 200,
  //      100, 250, 200, 100, GL_COLOR_BUFFER_BIT,
  //      GL_LINEAR);
  this->m_renderWindow->Render();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::cleanup() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::handleWindowChanged(QQuickWindow* w)
{
  this->m_interactorAdapter->setQQuickWindow(w);
  if (w)
  {
    // Do not clear the scenegraph before the QML rendering
    // to preserve the VTK render
    w->setClearBeforeRendering(false);
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
    vtkNew<QVTKInteractor> iren;
    iren->SetRenderWindow(this->m_renderWindow);

    // now set the default style
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    iren->SetInteractorStyle(style);
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
void QQuickVTKRenderWindow::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
  m_interactorAdapter->QueueGeometryChanged(newGeometry, oldGeometry);

  Superclass::geometryChanged(newGeometry, oldGeometry);
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKRenderWindow::event(QEvent* e)
{
  return Superclass::event(e);
}

//-------------------------------------------------------------------------------------------------
QPointer<QQuickVTKInteractorAdapter> QQuickVTKRenderWindow::interactorAdapter() const
{
  return this->m_interactorAdapter;
}
