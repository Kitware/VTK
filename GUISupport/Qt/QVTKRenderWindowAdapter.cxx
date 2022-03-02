/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKRenderWindowAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKRenderWindowAdapter.h"

#include <QVTKInteractorAdapter.h>
#include <vtkCommand.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLogger.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLState.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

#include <QApplication>
#include <QCursor>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QPointer>
#include <QScopedValueRollback>
#include <QScreen>
#include <QWidget>
#include <QWindow>

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
#include <QDesktopWidget>
#endif

#include <sstream>

#define QVTKInternalsDebugMacro(msg)                                                               \
  if (this->Logger)                                                                                \
  {                                                                                                \
    std::ostringstream str;                                                                        \
    str << "QVTKRenderWindowAdapter(" << this << "): " << msg;                                     \
    cout << str.str() << endl;                                                                     \
    this->Logger->logMessage(                                                                      \
      QOpenGLDebugMessage::createApplicationMessage(QString(str.str().c_str())));                  \
  }

class QVTKRenderWindowAdapter::QVTKInternals
{
  void renderWindowEventHandler(vtkObject*, unsigned long eventid, void* callData);
  void updateDPI() const;

  QWidget* ParentWidget;
  QWindow* ParentWindow;

public:
  QVTKRenderWindowAdapter* Self;
  QVTKInteractorAdapter InteractorAdapter;

  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;
  std::vector<unsigned long> RenderWindowObserverIds;

  // This flag is used as an indicate that `QVTKInteractorAdapter::render`
  // should request the vtkGenericOpenGLRenderWindow to render.
  // We need this to avoid re-rendering when the app directly triggers a render
  // by calling renderWindow->Render();
  bool DoVTKRenderInPaintGL = true;

  bool InPaint = false;

  int UnscaledDPI = 72; // same default as vtkWindow::DPI

  bool EnableHiDPI = true; // defaulting to enabling DPI scaling.

  double CustomDevicePixelRatio = 0.0;

  QPointer<QOpenGLContext> Context;
  QSurface* Surface;

  QScopedPointer<QOpenGLDebugLogger> Logger;

  QVTKInternals(QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* renWin,
    QObject* widgetOrWindow, QVTKRenderWindowAdapter* self)
    : ParentWidget(qobject_cast<QWidget*>(widgetOrWindow))
    , ParentWindow(qobject_cast<QWindow*>(widgetOrWindow))
    , Self(self)
    , InteractorAdapter(widgetOrWindow)
    , RenderWindow(renWin)
    , Context(cntxt)
    , Surface(nullptr)
  {
    Q_ASSERT(renWin != nullptr && cntxt != nullptr && widgetOrWindow != nullptr);

    const auto fmt = cntxt->format();
    if (fmt.testOption(QSurfaceFormat::DebugContext))
    {
      this->Logger.reset(new QOpenGLDebugLogger());
      if (!this->Logger->initialize())
      {
        // initialize failure means OpenGL doesn't have appropriate extension so
        // just don't log.
        this->Logger.reset(nullptr);
      }
    }

    QVTKInternalsDebugMacro("constructor");

    // It is unclear if we're better off creating a new QOpenGLContext with shared resources
    // or use the context pass in to this method. In the end, we decided to use the context pass in.
    // That way, if needed, the calling code can itself create new shared context and then pass
    // that to this method.
    this->Surface = this->Context->surface();

    this->RenderWindowObserverIds.push_back(renWin->AddObserver(
      vtkCommand::WindowMakeCurrentEvent, this, &QVTKInternals::renderWindowEventHandler));
    this->RenderWindowObserverIds.push_back(renWin->AddObserver(
      vtkCommand::WindowIsCurrentEvent, this, &QVTKInternals::renderWindowEventHandler));
    this->RenderWindowObserverIds.push_back(renWin->AddObserver(
      vtkCommand::WindowFrameEvent, this, &QVTKInternals::renderWindowEventHandler));
    this->RenderWindowObserverIds.push_back(
      renWin->AddObserver(vtkCommand::StartEvent, this, &QVTKInternals::renderWindowEventHandler));
    this->RenderWindowObserverIds.push_back(
      renWin->AddObserver(vtkCommand::EndEvent, this, &QVTKInternals::renderWindowEventHandler));
    this->RenderWindowObserverIds.push_back(renWin->AddObserver(
      vtkCommand::CursorChangedEvent, this, &QVTKInternals::renderWindowEventHandler));

    // First and foremost, make sure vtkRenderWindow is not using offscreen
    // buffers as that throws off all logic to render in the buffers we're
    // building and frankly unnecessary.
    if (this->RenderWindow->GetUseOffScreenBuffers())
    {
      vtkGenericWarningMacro(
        "disabling `UseOffScreenBuffers` since QVTKRenderWindowAdapter already "
        "uses offscreen buffers (FBO) for rendering");
      this->RenderWindow->SetUseOffScreenBuffers(false);
    }

    // since new context is being setup, call `OpenGLInitContext` which is stuff
    // to do when new context is created.
    this->RenderWindow->SetForceMaximumHardwareLineWidth(1);
    this->RenderWindow->SetReadyForRendering(true);
    this->RenderWindow->SetOwnContext(0);
    this->RenderWindow->OpenGLInitContext();

    // since the context is just being setup, we know that paint should indeed
    // request VTK to do a render.
    this->DoVTKRenderInPaintGL = true;

    // update current dpi and devicePixelRatio settings.
    this->InteractorAdapter.SetDevicePixelRatio(
      static_cast<float>(this->effectiveDevicePixelRatio()));
  }

  ~QVTKInternals()
  {
    QVTKInternalsDebugMacro("destructor");
    Q_ASSERT(this->RenderWindow);
    Q_ASSERT(this->Context);
    this->Logger.reset(nullptr);
    for (const auto& id : this->RenderWindowObserverIds)
    {
      this->RenderWindow->RemoveObserver(id);
    }
    this->RenderWindowObserverIds.clear();

    this->RenderWindow->Finalize();
    this->RenderWindow->SetReadyForRendering(false);
    this->Context = nullptr;
    this->Surface = nullptr;
  }

  double effectiveDevicePixelRatio() const
  {
    if (this->CustomDevicePixelRatio <= 0.0)
    {
      return this->ParentWindow ? this->ParentWindow->devicePixelRatio()
                                : this->ParentWidget->devicePixelRatioF();
    }
    return this->CustomDevicePixelRatio;
  }

  QSize screenSize() const
  {
    if (this->ParentWidget)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
      return this->ParentWidget->screen()->size();
#else
      return QApplication::desktop()->screenGeometry(this->ParentWidget).size();
#endif
    }
    else if (this->ParentWindow)
    {
      return this->ParentWindow->screen()->size();
    }
    return QSize();
  }

  bool makeCurrent()
  {
    Q_ASSERT(this->Context && this->Surface);
    return this->Context->makeCurrent(this->Surface);
  }

  bool isCurrent() const
  {
    Q_ASSERT(this->Context && this->Surface);
    auto currentContext = QOpenGLContext::currentContext();
    return (currentContext == this->Context && currentContext->surface() == this->Surface);
  }

  void resize(int w, int h)
  {
    QVTKInternalsDebugMacro("resize (" << w << ", " << h << ")");
    vtkLogF(TRACE, "resize(%d, %d)", w, h);
    const double dpr = this->effectiveDevicePixelRatio();
    const QSize deviceSize = QSize(w, h) * dpr;
    vtkLogF(TRACE, "resize(%d, %d), dpr=%f, scaledSize(%d, %d)", w, h, dpr, deviceSize.width(),
      deviceSize.height());
    this->InteractorAdapter.SetDevicePixelRatio(dpr);

    if (auto iren = this->RenderWindow->GetInteractor())
    {
      iren->UpdateSize(deviceSize.width(), deviceSize.height());
    }
    else
    {
      this->RenderWindow->SetSize(deviceSize.width(), deviceSize.height());
    }

    const QSize screen_size = this->screenSize();
    const QSize screen_deviceSize = screen_size * dpr;
    this->RenderWindow->SetScreenSize(screen_deviceSize.width(), screen_deviceSize.height());

    // since we've resize, we request a vtkRenderWindow::Render in `paintGL`
    // so we render an updated rendering.
    this->DoVTKRenderInPaintGL = true;

    // update render window DPI, if needed, since this method gets called on
    // devicePixelRatio changes as well.
    this->updateDPI();
  }

  void paint()
  {
    if (this->InPaint)
    {
      vtkLogF(TRACE, "recursive paint ?");
      return;
    }

    vtkLogScopeFunction(TRACE);
    QVTKInternalsDebugMacro("paint");
    QScopedValueRollback<bool> var(this->InPaint, true);
    if (this->DoVTKRenderInPaintGL)
    {
      vtkLogScopeF(TRACE, "requesting render");
      // TODO: check some flag to determine we auto render is enabled
      if (auto iren = this->RenderWindow->GetInteractor())
      {
        iren->Render();
      }
      else
      {
        this->RenderWindow->Render();
      }
    }
    this->DoVTKRenderInPaintGL = false;
  }

  void frame()
  {
    QVTKInternalsDebugMacro("frame");
    bool using_double_buffer = this->RenderWindow->GetDoubleBuffer();
    bool swap_buffers = this->RenderWindow->GetSwapBuffers();

    if (using_double_buffer && !swap_buffers)
    {
      // if we're using double buffer, but explicitly rendering to back buffer,
      // means that we don't want the thing we rendered displayed on the screen.
      // in which case, we ignore this frame result.
      vtkLogF(TRACE, "frame using_double_buffer=%d, swap_buffers=%d -- ignored",
        using_double_buffer, this->RenderWindow->GetSwapBuffers());
      return;
    }

    vtkLogF(TRACE, "frame using_double_buffer=%d, swap_buffers=%d", using_double_buffer,
      this->RenderWindow->GetSwapBuffers());

    this->DoVTKRenderInPaintGL = false;
    if (!this->InPaint)
    {
      if (this->ParentWidget)
      {
        this->ParentWidget->update();
      }
      else if (this->ParentWindow)
      {
        this->ParentWindow->requestUpdate();
      }
    }
  }

  bool blit(unsigned int targetId, int targetAttachment, const QRect& targetRect, bool left)
  {
    QVTKInternalsDebugMacro("blit");
    if (!this->Context)
    {
      return false;
    }
    QOpenGLExtraFunctions* f = this->Context->extraFunctions();
    if (!f)
    {
      return false;
    }

    f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetId);
    const GLenum bufs[1] = { static_cast<GLenum>(targetAttachment) };
    f->glDrawBuffers(1, bufs);

    GLboolean scissorTest = f->glIsEnabled(GL_SCISSOR_TEST);
    if (scissorTest == GL_TRUE)
    {
      this->RenderWindow->GetState()->vtkglDisable(GL_SCISSOR_TEST);
      f->glDisable(GL_SCISSOR_TEST); // Scissor affects glBindFramebuffer.
    }

    int* rbsize = this->RenderWindow->GetRenderFramebuffer()->GetLastSize();
    this->RenderWindow->BlitDisplayFramebuffer(left ? 0 : 1, 0, 0, rbsize[0], rbsize[1],
      targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), GL_COLOR_BUFFER_BIT,
      GL_LINEAR);

    this->clearAlpha(targetRect);

    if (scissorTest == GL_TRUE)
    {
      this->RenderWindow->GetState()->vtkglEnable(GL_SCISSOR_TEST);
      f->glEnable(GL_SCISSOR_TEST);
    }
    return true;
  }

  void setCursor(int vtk_cursor)
  {
    switch (vtk_cursor)
    {
      case VTK_CURSOR_CROSSHAIR:
        this->setCursor(QCursor(Qt::CrossCursor));
        break;
      case VTK_CURSOR_SIZEALL:
        this->setCursor(QCursor(Qt::SizeAllCursor));
        break;
      case VTK_CURSOR_SIZENS:
        this->setCursor(QCursor(Qt::SizeVerCursor));
        break;
      case VTK_CURSOR_SIZEWE:
        this->setCursor(QCursor(Qt::SizeHorCursor));
        break;
      case VTK_CURSOR_SIZENE:
        this->setCursor(QCursor(Qt::SizeBDiagCursor));
        break;
      case VTK_CURSOR_SIZENW:
        this->setCursor(QCursor(Qt::SizeFDiagCursor));
        break;
      case VTK_CURSOR_SIZESE:
        this->setCursor(QCursor(Qt::SizeFDiagCursor));
        break;
      case VTK_CURSOR_SIZESW:
        this->setCursor(QCursor(Qt::SizeBDiagCursor));
        break;
      case VTK_CURSOR_HAND:
        this->setCursor(QCursor(Qt::PointingHandCursor));
        break;
      case VTK_CURSOR_ARROW:
        this->setCursor(QCursor(Qt::ArrowCursor));
        break;
      default:
        this->setCursor(this->Self->defaultCursor());
        break;
    }
  }

  void setCursor(const QCursor& cursor)
  {
    this->ParentWindow ? this->ParentWindow->setCursor(cursor)
                       : this->ParentWidget->setCursor(cursor);
  }

  void setEnableHiDPI(bool val)
  {
    if (this->EnableHiDPI != val)
    {
      this->EnableHiDPI = val;
      this->updateDPI();
    }
  }

  void setUnscaledDPI(int val)
  {
    if (this->UnscaledDPI != val)
    {
      this->UnscaledDPI = val;
      this->updateDPI();
    }
  }

  void setCustomDevicePixelRatio(double sf)
  {
    if (this->CustomDevicePixelRatio != sf)
    {
      this->CustomDevicePixelRatio = sf;
      this->updateDPI();
    }
  }

  void clearAlpha(const QRect& targetRect) const
  {
    Q_ASSERT(this->Context);

    QOpenGLFunctions* f = this->Context->functions();
    if (f)
    {
      // now clear alpha otherwise we end up blending the rendering with
      // background windows in certain cases. It happens on OsX
      // (if QSurfaceFormat::alphaBufferSize() > 0) or when using Mesa on Linux
      // (see paraview/paraview#17159).
      GLboolean colorMask[4];
      f->glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
      f->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

      GLfloat clearColor[4];
      f->glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
      f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

      GLint viewport[4];
      f->glGetIntegerv(GL_VIEWPORT, viewport);
      f->glViewport(targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height());

      f->glClear(GL_COLOR_BUFFER_BIT);

      f->glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
      f->glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
      f->glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    }
  }
};

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::QVTKInternals::renderWindowEventHandler(
  vtkObject*, unsigned long eventid, void* callData)
{
  switch (eventid)
  {
    case vtkCommand::WindowMakeCurrentEvent:
      this->makeCurrent();
      break;

    case vtkCommand::WindowIsCurrentEvent:
    {
      bool& cstatus = *reinterpret_cast<bool*>(callData);
      cstatus = this->isCurrent();
    }
    break;

    case vtkCommand::WindowFrameEvent:
      this->frame();
      break;

    case vtkCommand::StartEvent:
      VTK_FALLTHROUGH;
    case vtkCommand::StartPickEvent:
      break;

    case vtkCommand::EndEvent:
      break;

    case vtkCommand::CursorChangedEvent:
    {
      int cShape = *reinterpret_cast<int*>(callData);
      this->setCursor(cShape);
    }
    break;
  }
}

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::QVTKInternals::updateDPI() const
{
  assert(this->RenderWindow != nullptr);
  this->RenderWindow->SetDPI(
    this->EnableHiDPI ? this->effectiveDevicePixelRatio() * this->UnscaledDPI : this->UnscaledDPI);
}

//------------------------------------------------------------------------------
QVTKRenderWindowAdapter::QVTKRenderWindowAdapter(
  QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* renWin, QWidget* widget)
  : QVTKRenderWindowAdapter(cntxt, renWin, static_cast<QObject*>(widget))
{
}

//------------------------------------------------------------------------------
QVTKRenderWindowAdapter::QVTKRenderWindowAdapter(
  QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* renWin, QWindow* window)
  : QVTKRenderWindowAdapter(cntxt, renWin, static_cast<QObject*>(window))
{
}

//------------------------------------------------------------------------------
QVTKRenderWindowAdapter::QVTKRenderWindowAdapter(
  QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* renWin, QObject* widgetOrWindow)
  : Superclass(widgetOrWindow)
  , Internals(new QVTKRenderWindowAdapter::QVTKInternals(cntxt, renWin, widgetOrWindow, this))
  , DefaultCursor(Qt::ArrowCursor)
{
  Q_ASSERT(renWin != nullptr && cntxt != nullptr && widgetOrWindow != nullptr);

  // need to makes sure that when the context is getting destroyed, we release
  // all OpenGL resources.
  this->connect(cntxt, SIGNAL(aboutToBeDestroyed()), SLOT(contextAboutToBeDestroyed()));
}

//------------------------------------------------------------------------------
QVTKRenderWindowAdapter::~QVTKRenderWindowAdapter()
{
  this->Internals.reset(nullptr);
}

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::contextAboutToBeDestroyed()
{
  this->Internals.reset(nullptr);
}

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::paint()
{
  if (this->Internals)
  {
    this->Internals->paint();
  }
}

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::resize(int width, int height)
{
  if (this->Internals)
  {
    this->Internals->resize(width, height);
  }
}

//------------------------------------------------------------------------------
bool QVTKRenderWindowAdapter::blit(
  unsigned int targetId, int targetAttachement, const QRect& targetRect, bool left)
{
  if (this->Internals)
  {
    return this->Internals->blit(targetId, targetAttachement, targetRect, left);
  }
  return false;
}

//------------------------------------------------------------------------------
bool QVTKRenderWindowAdapter::handleEvent(QEvent* evt)
{
  return this->Internals ? this->Internals->InteractorAdapter.ProcessEvent(
                             evt, this->Internals->RenderWindow->GetInteractor())
                         : false;
}

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::setEnableHiDPI(bool value)
{
  if (this->Internals)
  {
    this->Internals->setEnableHiDPI(value);
  }
}

//------------------------------------------------------------------------------
void QVTKRenderWindowAdapter::setUnscaledDPI(int unscaledDPI)
{
  if (this->Internals)
  {
    this->Internals->setUnscaledDPI(unscaledDPI);
  }
}
//-----------------------------------------------------------------------------
void QVTKRenderWindowAdapter::setCustomDevicePixelRatio(double sf)
{
  if (this->Internals)
  {
    this->Internals->setCustomDevicePixelRatio(sf);
  }
}
//------------------------------------------------------------------------------
QSurfaceFormat QVTKRenderWindowAdapter::defaultFormat(bool stereo_capable)
{
  QSurfaceFormat fmt;
  fmt.setRenderableType(QSurfaceFormat::OpenGL);
  fmt.setVersion(3, 2);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setRedBufferSize(8);
  fmt.setGreenBufferSize(8);
  fmt.setBlueBufferSize(8);
  fmt.setDepthBufferSize(8);
  fmt.setAlphaBufferSize(8);
  fmt.setStencilBufferSize(0);
  fmt.setStereo(stereo_capable);
  fmt.setSamples(0); // we never need multisampling in the context since the FBO can support
                     // multisamples independently
  return fmt;
}
