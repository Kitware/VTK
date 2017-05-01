/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKOpenGLWidget.h"

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLTexture>
#include <QPointer>
#include <QScopedValueRollback>
#include <QtDebug>

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "vtkCommand.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

// #define DEBUG_QVTKOPENGL_WIDGET
#ifdef DEBUG_QVTKOPENGL_WIDGET
#define vtkQVTKOpenGLWidgetDebugMacro(msg)                                                         \
  cout << this << ": " msg << endl;                                                                \
  if (this->Logger)                                                                                \
  {                                                                                                \
    this->Logger->logMessage(                                                                      \
      QOpenGLDebugMessage::createApplicationMessage(QStringLiteral("QVTKOpenGLWidget::" msg)));    \
  }
#else
#define vtkQVTKOpenGLWidgetDebugMacro(x)
#endif

namespace
{
void vtkSetBackgroundAlpha(vtkRenderWindow* renWin, double value)
{
  vtkRendererCollection* renCollection = renWin->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  renCollection->InitTraversal(cookie);
  while (vtkRenderer* ren = renCollection->GetNextRenderer(cookie))
  {
    ren->SetBackgroundAlpha(value);
  }
}
}

class QVTKOpenGLWidgetObserver : public vtkCommand
{
public:
  static QVTKOpenGLWidgetObserver* New() { return new QVTKOpenGLWidgetObserver(); }
  vtkBaseTypeMacro(QVTKOpenGLWidgetObserver, vtkCommand);

  void SetTarget(QVTKOpenGLWidget* target) { this->Target = target; }

  void Execute(vtkObject*, unsigned long eventId, void* callData) VTK_OVERRIDE
  {
    if (this->Target)
    {
      switch (eventId)
      {
        case vtkCommand::WindowMakeCurrentEvent:
          {
            // We do not call QOpenGLWidget::makeCurrent() as that also makes the
            // frame buffer object used by QOpenGLWidget active. This can have
            // unintended side effects when MakeCurrent gets called in a
            // render-pass, for example. We should only be making the context
            // active. To do that, we use this trick. We rely on the
            // QOpenGLContext have been called makeCurrent() previously so we
            // can get to the surface that was used to do that. We simply
            // reactivate on that surface.
            QOpenGLContext* ctxt = this->Target->context();
            QSurface* surface = ctxt? ctxt->surface() : NULL;
            if (surface)
            {
              ctxt->makeCurrent(surface);
            }
            Q_ASSERT(ctxt == NULL || surface != NULL);
          }
          break;

        case vtkCommand::WindowIsCurrentEvent:
        {
          bool& cstatus = *reinterpret_cast<bool*>(callData);
          cstatus = (QOpenGLContext::currentContext() == this->Target->context());
        }
        break;

        case vtkCommand::WindowFrameEvent:
          this->Target->windowFrameEventCallback();
          break;

        case vtkCommand::StartEvent:
          this->Target->startEventCallback();
          break;
      }
    }
  }

protected:
  QVTKOpenGLWidgetObserver() {}
  ~QVTKOpenGLWidgetObserver() {}
  QPointer<QVTKOpenGLWidget> Target;
};

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::QVTKOpenGLWidget(QWidget* parentWdg, Qt::WindowFlags f)
  : Superclass(parentWdg, f)
  , DeferRenderInPaintEvent(false)
  , InteractorAdaptor(NULL)
  , FBO(nullptr)
  , InPaintGL(false)
  , SkipRenderInPaintGL(false)
  , Logger(nullptr)
{
  this->Observer->SetTarget(this);

  // default to strong focus
  this->setFocusPolicy(Qt::StrongFocus);

  this->InteractorAdaptor = new QVTKInteractorAdapter(this);
  this->InteractorAdaptor->SetDevicePixelRatio(this->devicePixelRatio());

  this->setMouseTracking(true);

  this->DeferedRenderTimer.setSingleShot(true);
  this->DeferedRenderTimer.setInterval(0);
  this->connect(&this->DeferedRenderTimer, SIGNAL(timeout()), SLOT(doDeferredRender()));

  // QOpenGLWidget::resized() is triggered when the default FBO in QOpenGLWidget is recreated.
  // We use the same signal to recreate our FBO.
  this->connect(this, SIGNAL(resized()), SLOT(recreateFBO()));
}

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::~QVTKOpenGLWidget()
{
  vtkQVTKOpenGLWidgetDebugMacro("~QVTKOpenGLWidget");
  // essential to cleanup context so that the render window finalizes and
  // releases any graphics resources it may have allocated.
  this->cleanupContext();
  this->SetRenderWindow(static_cast<vtkGenericOpenGLRenderWindow*>(NULL));
  this->Observer->SetTarget(NULL);
  delete this->InteractorAdaptor;
  delete this->Logger;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::SetRenderWindow(vtkRenderWindow* win)
{
  vtkGenericOpenGLRenderWindow* gwin = vtkGenericOpenGLRenderWindow::SafeDownCast(win);
  this->SetRenderWindow(gwin);
  if (gwin == NULL && win != NULL)
  {
    qDebug() << "QVTKOpenGLWidget requires a `vtkGenericOpenGLRenderWindow`. `"
             << win->GetClassName() << "` is not supported.";
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::SetRenderWindow(vtkGenericOpenGLRenderWindow* win)
{
  if (this->RenderWindow == win)
  {
    return;
  }

  if (this->RenderWindow)
  {
    this->RenderWindow->RemoveObserver(this->Observer.Get());
  }

  this->InteractorAdaptor->SetDevicePixelRatio(this->devicePixelRatio());

  this->RenderWindow = win;
  this->requireRenderWindowInitialization();
  if (this->RenderWindow)
  {
    // tell the vtk window what the size of this window is
    this->RenderWindow->SetSize(this->width() * this->devicePixelRatio(),
                                this->height() * this->devicePixelRatio());
    this->RenderWindow->SetPosition(this->x() * this->devicePixelRatio(),
                                    this->y() * this->devicePixelRatio());

    // if an interactor wasn't provided, we'll make one by default
    if (!this->RenderWindow->GetInteractor())
    {
      // create a default interactor
      vtkNew<QVTKInteractor> iren;
      // iren->SetUseTDx(this->UseTDx);
      this->RenderWindow->SetInteractor(iren.Get());
      iren->Initialize();

      // now set the default style
      vtkNew<vtkInteractorStyleTrackballCamera> style;
      iren->SetInteractorStyle(style.Get());
    }

    // tell the interactor the size of this window
    this->RenderWindow->GetInteractor()
        ->SetSize(this->width() * this->devicePixelRatio(),
                  this->height() * this->devicePixelRatio());

    this->RenderWindow->AddObserver(vtkCommand::WindowMakeCurrentEvent, this->Observer.Get());
    this->RenderWindow->AddObserver(vtkCommand::WindowIsCurrentEvent, this->Observer.Get());
    this->RenderWindow->AddObserver(vtkCommand::WindowFrameEvent, this->Observer.Get());
    this->RenderWindow->AddObserver(vtkCommand::StartEvent, this->Observer.Get());
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::startEventCallback()
{
  vtkQVTKOpenGLWidgetDebugMacro("startEventCallback");
  this->makeCurrent();
  if (this->FBO)
  {
    // ensure that before vtkRenderWindow starts to render, we activate the FBO
    // to render into. VTK code can be a bit lax with it. This just ensures that
    // we have the FBO activated.
    this->FBO->bind();
  }
}

//-----------------------------------------------------------------------------
vtkRenderWindow* QVTKOpenGLWidget::GetRenderWindow()
{
  return this->RenderWindow.Get();
}

//-----------------------------------------------------------------------------
QVTKInteractor* QVTKOpenGLWidget::GetInteractor()
{
  return QVTKInteractor::SafeDownCast(this->GetRenderWindow()->GetInteractor());
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::setDeferRenderInPaintEvent(bool val)
{
  this->DeferRenderInPaintEvent = val;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::copyFromFormat(const QSurfaceFormat& format, vtkRenderWindow* win)
{
  if (vtkOpenGLRenderWindow* oglWin = vtkOpenGLRenderWindow::SafeDownCast(win))
  {
    oglWin->SetStereoCapableWindow(format.stereo() ? 1 : 0);
    // samples may not be correct if format is obtained from
    // QOpenGLWidget::format() after the context is created. That's because
    // QOpenGLWidget always created context to samples=0.
    oglWin->SetMultiSamples(format.samples());
    oglWin->SetStencilCapable(format.stencilBufferSize() > 0);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::copyToFormat(vtkRenderWindow* win, QSurfaceFormat& format)
{
  if (vtkOpenGLRenderWindow* oglWin = vtkOpenGLRenderWindow::SafeDownCast(win))
  {
    format.setStereo(oglWin->GetStereoCapableWindow());
    format.setSamples(oglWin->GetMultiSamples());
    format.setStencilBufferSize(oglWin->GetStencilCapable() ? 8 : 0);
  }
}

//-----------------------------------------------------------------------------
QSurfaceFormat QVTKOpenGLWidget::defaultFormat()
{
  QSurfaceFormat fmt;
  fmt.setRenderableType(QSurfaceFormat::OpenGL);
  fmt.setVersion(3, 2);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setRedBufferSize(1);
  fmt.setGreenBufferSize(1);
  fmt.setBlueBufferSize(1);
  fmt.setDepthBufferSize(1);
  fmt.setStencilBufferSize(0);
  fmt.setAlphaBufferSize(1);
  fmt.setStereo(false);
  fmt.setSamples(vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples());
#ifdef DEBUG_QVTKOPENGL_WIDGET
  fmt.setOption(QSurfaceFormat::DebugContext);
#endif
  return fmt;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::recreateFBO()
{
  vtkQVTKOpenGLWidgetDebugMacro("recreateFBO");
  delete this->FBO;
  this->FBO = nullptr;
  if (!this->RenderWindow)
  {
    return;
  }

  // Since QVTKOpenGLWidget::initializeGL() cannot set multi-samples
  // state on the RenderWindow correctly, we do it here.
  QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
  GLint samples;
  f->glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
  this->RenderWindow->SetMultiSamples(static_cast<int>(samples));

  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::Depth);
  format.setSamples(samples);

  const QSize deviceSize = this->size() * this->devicePixelRatioF();
  this->FBO = new QOpenGLFramebufferObject(deviceSize, format);
  this->FBO->bind();
  this->RenderWindow->SetForceMaximumHardwareLineWidth(1);
  this->RenderWindow->SetReadyForRendering(true);
  this->RenderWindow->InitializeFromCurrentContext();

  // On OsX (if QSurfaceFormat::alphaBufferSize() > 0) or when using Mesa, we
  // end up rendering fully transparent windows (see through background)
  // unless we fill it with alpha=1.0 (See paraview/paraview#17159).
  vtkSetBackgroundAlpha(this->RenderWindow, 1.0);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::initializeGL()
{
  this->Superclass::initializeGL();

#ifdef DEBUG_QVTKOPENGL_WIDGET
  delete this->Logger;
  this->Logger = new QOpenGLDebugLogger(this);
  this->Logger->initialize(); // initializes in the current context.
#endif

  vtkQVTKOpenGLWidgetDebugMacro("initializeGL");
  if (this->RenderWindow)
  {
    // use QSurfaceFormat for the widget, update ivars on the vtkRenderWindow.
    QVTKOpenGLWidget::copyFromFormat(this->format(), this->RenderWindow);
    // When a QOpenGLWidget is told to use a QSurfaceFormat with samples > 0,
    // QOpenGLWidget doesn't actually create a context with multi-samples and
    // internally changes the QSurfaceFormat to be samples=0. Thus, we can't
    // rely on the QSurfaceFormat to indicate to us if multisampling is being
    // used. We should use glGetRenderbufferParameteriv(..) to get
    // GL_RENDERBUFFER_SAMPLES to determine the samples used. This is done by
    // in recreateFBO().
  }
  this->connect(
    this->context(), SIGNAL(aboutToBeDestroyed()), SLOT(cleanupContext()), Qt::UniqueConnection);
  this->requireRenderWindowInitialization();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::requireRenderWindowInitialization()
{
  if (this->RenderWindow)
  {
    this->RenderWindow->SetReadyForRendering(false);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::resizeGL(int w, int h)
{
  vtkQVTKOpenGLWidgetDebugMacro("resizeGL");
  vtkRenderWindowInteractor* iren = this->RenderWindow ? this->RenderWindow->GetInteractor() : nullptr;
  this->InteractorAdaptor->SetDevicePixelRatio(this->devicePixelRatio(), iren);
  if (this->RenderWindow)
  {
    this->RenderWindow->SetSize(w * this->devicePixelRatio(),
                                h * this->devicePixelRatio());
    this->SkipRenderInPaintGL = false;
  }
  this->Superclass::resizeGL(w, h);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::paintGL()
{
  if (this->InPaintGL)
  {
    return;
  }

  Q_ASSERT(this->FBO);
  Q_ASSERT(this->FBO->handle() == this->RenderWindow->GetDefaultFrameBufferId());


  QScopedValueRollback<bool> var(this->InPaintGL, true);
  this->Superclass::paintGL();

  if (this->SkipRenderInPaintGL)
  {
    vtkQVTKOpenGLWidgetDebugMacro("paintGL:skipped");
    this->SkipRenderInPaintGL = false;
  }
  else
  {
    if (this->DeferRenderInPaintEvent)
    {
      vtkQVTKOpenGLWidgetDebugMacro("paintGL:defer render");
      this->deferRender();
    }
    else
    {
      vtkQVTKOpenGLWidgetDebugMacro("paintGL:render");
      this->doDeferredRender();
    }
  }

  // If render was triggered by above calls, that may change the current context
  // due to things like progress events triggering updates on other widgets
  // (e.g. progress bar). Hence we need to make sure to call makeCurrent()
  // before proceeding with blit-ing.
  this->makeCurrent();

  // blit from this->FBO to QOpenGLWidget's FBO.
  vtkQVTKOpenGLWidgetDebugMacro("paintGL::blit-to-defaultFBO");
  QOpenGLFunctions_3_2_Core* f =
    QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
  if (f)
  {
    f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebufferObject());
    f->glDrawBuffer(GL_COLOR_ATTACHMENT0);

    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, this->FBO->handle());
    f->glReadBuffer(GL_COLOR_ATTACHMENT0);
    f->glDisable(GL_SCISSOR_TEST); // Scissor affects glBindFramebuffer.
    f->glBlitFramebuffer(0, 0, this->RenderWindow->GetSize()[0], this->RenderWindow->GetSize()[1],
      0, 0, this->RenderWindow->GetSize()[0], this->RenderWindow->GetSize()[1], GL_COLOR_BUFFER_BIT,
      GL_NEAREST);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::cleanupContext()
{
  // logger gets uninitialized when this gets called. We get errors from
  // QOpenGLDebugLogger if logMessage is called here. So we just destroy the
  // logger.
  delete this->Logger;
  this->Logger = nullptr;

  vtkQVTKOpenGLWidgetDebugMacro("cleanupContext");

  // QOpenGLWidget says when this slot is called, the context may not be current
  // and hence is a good practice to make it so.
  this->makeCurrent();
  if (this->RenderWindow)
  {
    if (this->FBO)
    {
      this->FBO->bind();
    }
    this->RenderWindow->Finalize();
    this->RenderWindow->SetReadyForRendering(false);
  }
  delete this->FBO;
  this->FBO = nullptr;
  this->requireRenderWindowInitialization();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::windowFrameEventCallback()
{
  Q_ASSERT(this->RenderWindow);
  vtkQVTKOpenGLWidgetDebugMacro("frame");

  // Render happened. If we have requested a render to happen, it has happened,
  // so no need to request another render. Stop the timer.
  this->DeferedRenderTimer.stop();

  if (!this->InPaintGL)
  {
    // Handing vtkOpenGLRenderWindow::Frame is tricky. VTK code traditionally
    // calls `Frame` to indicate that VTK is done rendering 1 frame. Now, when
    // that happens, should we tell Qt to update the widget -- that's the
    // question? In general, yes, but sometimes VTK does things in the
    // background i.e. back buffer without wanting to update the front buffer
    // e.g. when making selections. In that case, we don't want to update Qt
    // widget either, since whatever it did was not meant to be visible.
    // To handle that, we check if vtkOpenGLRenderWindow::SwapBuffers is true,
    // and request an update only when it is.
    if (this->RenderWindow->GetSwapBuffers() || this->RenderWindow->GetDoubleBuffer() == 0)
    {
      // Means that the vtkRenderWindow rendered outside a paintGL call. That can
      // happen when application code call vtkRenderWindow::Render() directly,
      // instead of calling QVTKOpenGLWidget::update() or letting Qt update the
      // widget. In that case, since QOpenGLWidget rendering into an offscreen
      // FBO, the result still needs to be composed by Qt widget stack. We request
      // that using `update()`.
      vtkQVTKOpenGLWidgetDebugMacro("update");
      this->update();

      this->SkipRenderInPaintGL = true;
    }
    else
    {
      this->SkipRenderInPaintGL = false;
    }

  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::deferRender()
{
  this->DeferedRenderTimer.start();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::doDeferredRender()
{
  vtkRenderWindowInteractor* iren = this->RenderWindow ? this->RenderWindow->GetInteractor() : NULL;
  if (iren && this->FBO)
  {
    // Bind the FBO we'll be rendering into (is this needed? VTK will bind it anyways).
    this->FBO->bind();
    iren->Render();
    this->DeferedRenderTimer.stop(); // not necessary, but no harm.
  }
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWidget::event(QEvent* evt)
{
  switch (evt->type())
  {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
      // skip events that are explicitly handled by overrides to avoid duplicate
      // calls to InteractorAdaptor->ProcessEvent().
      break;

    default:
      if (this->RenderWindow && this->RenderWindow->GetInteractor())
      {
        this->InteractorAdaptor->ProcessEvent(evt, this->RenderWindow->GetInteractor());
      }
  }
  return this->Superclass::event(evt);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::moveEvent(QMoveEvent* evt)
{
  this->Superclass::moveEvent(evt);
  if (this->RenderWindow)
  {
    this->RenderWindow->SetPosition(this->x(), this->y());
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::mousePressEvent(QMouseEvent* event)
{
  emit mouseEvent(event);

  if (this->RenderWindow && this->RenderWindow->GetInteractor())
  {
    this->InteractorAdaptor->ProcessEvent(event,
                                          this->RenderWindow->GetInteractor());
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
  emit mouseEvent(event);

  if (this->RenderWindow && this->RenderWindow->GetInteractor())
  {
    this->InteractorAdaptor->ProcessEvent(event,
                                          this->RenderWindow->GetInteractor());
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
  emit mouseEvent(event);

  if (this->RenderWindow && this->RenderWindow->GetInteractor())
  {
    this->InteractorAdaptor->ProcessEvent(event,
                                          this->RenderWindow->GetInteractor());
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  emit mouseEvent(event);

  if (this->RenderWindow && this->RenderWindow->GetInteractor())
  {
    this->InteractorAdaptor->ProcessEvent(event,
                                          this->RenderWindow->GetInteractor());
  }
}
