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
#include <QOpenGLFunctions>
#include <QPointer>

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "vtkCommand.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkShaderProgram.h"
#include "vtkUnsignedCharArray.h"

// #define DEBUG_QVTKOPENGL_WIDGET
#ifdef DEBUG_QVTKOPENGL_WIDGET
#define vtkQVTKOpenGLWidgetDebugMacro(msg) cout << this << ": " msg << endl;
#else
#define vtkQVTKOpenGLWidgetDebugMacro(x)
#endif

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
          this->Target->makeCurrent();
          break;

        case vtkCommand::WindowIsCurrentEvent:
        {
          bool& cstatus = *reinterpret_cast<bool*>(callData);
          cstatus = (QOpenGLContext::currentContext() == this->Target->context());
        }
        break;

        case vtkCommand::WindowFrameEvent:
          vtkQVTKOpenGLWidgetDebugMacro("frame");
          this->Target->windowFrameEventCallback();
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
  , AutomaticImageCacheEnabled(false)
  , MaxRenderRateForImageCache(1.0)
  , DeferRenderInPaintEvent(false)
  , InteractorAdaptor(NULL)
  , InPaintGL(false)
  , NeedToReinitializeWindow(false)
  , SkipRenderInPaintGL(false)
{
  this->Observer->SetTarget(this);

  // default to strong focus
  this->setFocusPolicy(Qt::StrongFocus);

  this->setAttribute(Qt::WA_NoSystemBackground, false);

  // Currently, we rely on QOpenGLWidget::PartialUpdate. This allows to easily
  // handle the case where vtkRenderWindow::Render() is called from outside
  // QVTKOpenGLWidget::paintGL(). When that happens, we simply skip calling
  // vtkRenderWindow::Render() when `painGL` gets called.
  this->setUpdateBehavior(QOpenGLWidget::PartialUpdate);

  this->InteractorAdaptor = new QVTKInteractorAdapter(this);

  this->setMouseTracking(true);

  this->DeferedRenderTimer.setSingleShot(true);
  this->DeferedRenderTimer.setInterval(0);
  this->connect(&this->DeferedRenderTimer, SIGNAL(timeout()), SLOT(doDeferredRender()));
}

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::~QVTKOpenGLWidget()
{
  this->SetRenderWindow(static_cast<vtkGenericOpenGLRenderWindow*>(NULL));
  this->Observer->SetTarget(NULL);
  delete this->InteractorAdaptor;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::SetRenderWindow(vtkRenderWindow* win)
{
  this->SetRenderWindow(vtkGenericOpenGLRenderWindow::SafeDownCast(win));
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

  // before releasing old render window, release any textures we may have
  // allocated on it.
  this->markCachedImageAsDirty();

  this->RenderWindow = win;
  if (this->RenderWindow)
  {
    // tell the vtk window what the size of this window is
    this->RenderWindow->SetSize(this->width(), this->height());
    this->RenderWindow->SetPosition(this->x(), this->y());

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
    this->RenderWindow->GetInteractor()->SetSize(this->width(), this->height());

    // Add an observer to monitor when the image changes.  Should work most
    // of the time.  The application will have to call
    // markCachedImageAsDirty for any other case.
    this->RenderWindow->AddObserver(vtkCommand::WindowMakeCurrentEvent, this->Observer.Get());
    this->RenderWindow->AddObserver(vtkCommand::WindowIsCurrentEvent, this->Observer.Get());
    this->RenderWindow->AddObserver(vtkCommand::WindowFrameEvent, this->Observer.Get());
    this->NeedToReinitializeWindow = true;
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
void QVTKOpenGLWidget::markCachedImageAsDirty()
{
  // this is done as an extra precaution.
  this->SkipRenderInPaintGL = false;
  if (this->CachedTexture)
  {
    this->CachedTexture = NULL;
    emit this->cachedImageDirty();
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::saveImageToCache()
{
  // this only works in non-multisampling mode.
  if (this->RenderWindow->GetMultiSamples())
  {
    return;
  }

  if (this->CachedTexture == NULL)
  {
    this->CachedTexture = vtkSmartPointer<vtkTextureObject>::New();
    this->CachedTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->CachedTexture->SetWrapT(vtkTextureObject::ClampToEdge);
    this->CachedTexture->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->CachedTexture->SetMinificationFilter(vtkTextureObject::Nearest);
  }
  this->CachedTexture->SetContext(vtkOpenGLRenderWindow::SafeDownCast(this->RenderWindow));
  const int* wsize = this->RenderWindow->GetSize();
  this->CachedTexture->Allocate2D(wsize[0], wsize[1], 3, VTK_UNSIGNED_CHAR);
  this->CachedTexture->CopyFromFrameBuffer(0, 0, 0, 0, wsize[0], wsize[1]);
  emit this->cachedImageClean();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::setAutomaticImageCacheEnabled(bool flag)
{
  if (this->AutomaticImageCacheEnabled != flag)
  {
    this->AutomaticImageCacheEnabled = flag;
    if (!flag)
    {
      this->markCachedImageAsDirty();
    }
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::setMaxRenderRateForImageCache(double rate)
{
  this->MaxRenderRateForImageCache = rate;
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
    // FIXME:
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
  fmt.setAlphaBufferSize(0);
  fmt.setStereo(false);
  fmt.setSamples(vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples());
  return fmt;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::initializeGL()
{
  vtkQVTKOpenGLWidgetDebugMacro("initializeGL");
  this->Superclass::initializeGL();
  if (this->RenderWindow)
  {
    // use QSurfaceFormat for the widget, update ivars on the vtkRenderWindow.
    // FIXME: not sure if we should do this, or except client code to keep
    // things in sync?
    QVTKOpenGLWidget::copyFromFormat(this->format(), this->RenderWindow);
  }

  this->connect(
    this->context(), SIGNAL(aboutToBeDestroyed()), SLOT(cleanupContext()), Qt::UniqueConnection);
  this->NeedToReinitializeWindow = true;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::resizeGL(int w, int h)
{
  vtkQVTKOpenGLWidgetDebugMacro("resizeGL");
  if (this->RenderWindow)
  {
    this->RenderWindow->SetSize(w, h);
    this->markCachedImageAsDirty();
  }
  this->Superclass::resizeGL(w, h);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::paintGL()
{
  if (this->SkipRenderInPaintGL)
  {
    vtkQVTKOpenGLWidgetDebugMacro("paintGL:skipped");
    this->SkipRenderInPaintGL = false;
    return;
  }

  vtkQVTKOpenGLWidgetDebugMacro("paintGL");
  bool prev = this->InPaintGL;
  this->InPaintGL = true;
  this->Superclass::paintGL();
  if (this->NeedToReinitializeWindow && this->RenderWindow)
  {
    this->RenderWindow->SetForceMaximumHardwareLineWidth(1);
    this->RenderWindow->InitializeFromCurrentContext();
    this->NeedToReinitializeWindow = false;
  }

  // if we have a saved image, use it
  if (this->paintCachedImage() == false)
  {
    if (this->DeferRenderInPaintEvent)
    {
      this->deferRender();
    }
    else
    {
      this->doDeferredRender();
    }
  }
  this->InPaintGL = prev;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::cleanupContext()
{
  vtkQVTKOpenGLWidgetDebugMacro("cleanupContext");
  if (this->RenderWindow)
  {
    this->RenderWindow->Finalize();
  }
  this->markCachedImageAsDirty();
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWidget::paintCachedImage()
{
  // if we have a saved image, use it
  if (this->CachedTexture)
  {
    vtkQVTKOpenGLWidgetDebugMacro("using cache");
    // Since we're not caching depth buffer (should we), we clear depth buffer
    // before pasting the cached image.
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glClear(GL_DEPTH_BUFFER_BIT);
    this->CachedTexture->CopyToFrameBuffer(0, 0, this->CachedTexture->GetWidth() - 1,
      this->CachedTexture->GetHeight() - 1, 0, 0, this->width() - 1, this->height() - 1, NULL,
      NULL);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::windowFrameEventCallback()
{
  Q_ASSERT(this->RenderWindow);

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
    if (this->RenderWindow->GetSwapBuffers())
    {
      this->SkipRenderInPaintGL = true;

      // Means that the vtkRenderWindow rendered outside a paintGL call. That can
      // happen when application code call vtkRenderWindow::Render() directly,
      // instead of calling QVTKOpenGLWidget::update() or letting Qt update the
      // widget. In that case, since QOpenGLWidget rendering into an offscreen
      // FBO, the result still needs to be composed by Qt widget stack. We request
      // that using `update()`.
      vtkQVTKOpenGLWidgetDebugMacro("update");
      this->update();
    }
    else
    {
      // VTK has destroyed the frame buffer by rendering something in it. Now,
      // if Qt for some reason comes back with a paint request, then we'll need
      // to ensure VTK renders again (or uses cache, if caching was enabled).
      this->SkipRenderInPaintGL = false;
    }
  }

  // prevent capturing the selection buffer as the cached image. to do this
  // we iterate through each renderer in the view and check if they have an
  // active selector object. if so we return without saving the image
  vtkRendererCollection* renderers = this->RenderWindow->GetRenderers();
  if (renderers)
  {
    renderers->InitTraversal();
    while (vtkRenderer* renderer = renderers->GetNextItem())
    {
      if (renderer->GetSelector() != NULL)
      {
        return;
      }
    }
  }

  // Render happened. If we have requested a render to happen, it has happened,
  // so no need to request another render. Stop the timer.
  this->DeferedRenderTimer.stop();
  if (this->isAutomaticImageCacheEnabled() &&
    (this->RenderWindow->GetDesiredUpdateRate() < this->maxRenderRateForImageCache()) &&
    this->RenderWindow->GetSwapBuffers())
  {
    this->saveImageToCache();
    vtkQVTKOpenGLWidgetDebugMacro("saving to cache");
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
  if (iren)
  {
    iren->Render();
    this->DeferedRenderTimer.stop(); // not necessary, but no harm.
  }
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWidget::event(QEvent* evt)
{
  switch (evt->type())
  {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
      emit this->mouseEvent(static_cast<QMouseEvent*>(evt));
      break;
    default:
      break;
  }

  if (this->RenderWindow && this->RenderWindow->GetInteractor())
  {
    this->InteractorAdaptor->ProcessEvent(evt, this->RenderWindow->GetInteractor());
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
