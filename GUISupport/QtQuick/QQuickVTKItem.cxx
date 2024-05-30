// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) John Stone
// SPDX-License-Identifier: BSD-3-Clause
#include "QQuickVTKItem.h"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRenderNode>
#include <QtQuick/QSGRendererInterface>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/QThread>
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#include <QtGui/QWheelEvent>
#endif
#endif

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTextureObject.h"

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "QVTKRenderWindowAdapter.h"

// The Qt macro Q_D(X) creates a local variable named 'd' which shadows a private member variable in
// QQmlParserStatus
QT_WARNING_DISABLE_GCC("-Wshadow")
QT_WARNING_DISABLE_CLANG("-Wshadow")
QT_WARNING_DISABLE_MSVC(4458)

// no touch events for now
#define NO_TOUCH

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
void QQuickVTKItem::setGraphicsApi()
{
  QSurfaceFormat fmt = QVTKRenderWindowAdapter::defaultFormat(false);
  // By default QtQuick sets the alpha buffer size to 0. We follow the same thing here to prevent a
  // transparent background.
  fmt.setAlphaBufferSize(0);
  QSurfaceFormat::setDefaultFormat(fmt);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
#else
  QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
#endif
}

//-------------------------------------------------------------------------------------------------

class QSGVtkObjectNode;

class QQuickVTKItemPrivate
{
public:
  QQuickVTKItemPrivate(QQuickVTKItem* ptr)
    : q_ptr(ptr)
  {
  }

  QQueue<std::function<void(vtkRenderWindow*, QQuickVTKItem::vtkUserData)>> asyncDispatch;

  QVTKInteractorAdapter qt2vtkInteractorAdapter;
  bool scheduleRender = false;

  mutable QSGVtkObjectNode* node = nullptr;

private:
  Q_DISABLE_COPY(QQuickVTKItemPrivate)
  Q_DECLARE_PUBLIC(QQuickVTKItem)
  QQuickVTKItem* const q_ptr;
};

namespace
{
bool checkGraphicsApi(QQuickWindow* window)
{
  auto api = window->rendererInterface()->graphicsApi();
  if (api != QSGRendererInterface::OpenGL
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    && api != QSGRendererInterface::OpenGLRhi
#endif
  )
  {
    qFatal(R"***(Error: QtQuick scenegraph is using an unsupported graphics API: %d.
Set the QSG_INFO environment variable to get more information.
Use QQuickVTKItem::setupGraphicsApi() to set the OpenGLRhi backend.)***",
      api);
  }
  return true;
}
}
//-------------------------------------------------------------------------------------------------

QQuickVTKItem::QQuickVTKItem(QQuickItem* parent)
  : QQuickItem(parent)
  , _d_ptr(new QQuickVTKItemPrivate(this))
{
  setAcceptHoverEvents(true);
#ifndef NO_TOUCH
  setAcceptTouchEvents(true);
#endif
  setAcceptedMouseButtons(Qt::AllButtons);

  setFlag(QQuickItem::ItemIsFocusScope);
  setFlag(QQuickItem::ItemHasContents);
}

QQuickVTKItem::~QQuickVTKItem() = default;

void QQuickVTKItem::dispatch_async(std::function<void(vtkRenderWindow*, vtkUserData)> f)
{
  Q_D(QQuickVTKItem);

  d->asyncDispatch.append(f);

  update();
}

class QSGVtkObjectNode
  : public QSGTextureProvider
  , public QSGSimpleTextureNode
{
  Q_OBJECT
public:
  QSGVtkObjectNode() { qsgnode_set_description(this, QStringLiteral("vtknode")); }

  ~QSGVtkObjectNode() override
  {
    if (m_item)
      m_item->destroyingVTK(vtkWindow, vtkUserData);

    delete QSGVtkObjectNode::texture();

    // Cleanup the VTK window resources
    vtkWindow->GetRenderers()->InitTraversal();
    while (auto renderer = vtkWindow->GetRenderers()->GetNextItem())
      renderer->ReleaseGraphicsResources(vtkWindow);
    vtkWindow->ReleaseGraphicsResources(vtkWindow);
    vtkWindow = nullptr;

    // Cleanup the User Data
    vtkUserData = nullptr;
  }

  QSGTexture* texture() const override { return QSGSimpleTextureNode::texture(); }

  void initialize(QQuickVTKItem* item)
  {
    // Create and initialize the vtkWindow
    vtkWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWindow->SetMultiSamples(0);
    vtkWindow->SetReadyForRendering(false);
    vtkWindow->SetFrameBlitModeToNoBlit();
    vtkNew<QVTKInteractor> iren;
    iren->SetRenderWindow(vtkWindow);
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    iren->SetInteractorStyle(style);
    vtkUserData = item->initializeVTK(vtkWindow);
    auto* ia = vtkWindow->GetInteractor();
    if (ia && !QVTKInteractor::SafeDownCast(ia))
    {
      qWarning().nospace() << "QQuickVTKItem.cpp:" << __LINE__
                           << ", YIKES!! Only QVTKInteractor is supported";
      return;
    }
    vtkWindow->SetReadyForRendering(false);
    vtkWindow->GetInteractor()->Initialize();
    vtkWindow->SetMapped(true);
    vtkWindow->SetIsCurrent(true);
    vtkWindow->SetForceMaximumHardwareLineWidth(1);
    vtkWindow->SetOwnContext(false);
    vtkWindow->OpenGLInitContext();
  }

  void scheduleRender()
  {
    // Update only if we have a window and a render is not already queued.
    if (m_window && !m_renderPending)
    {
      m_renderPending = true;
      m_window->update();
    }
  }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void render()
  {
    if (m_renderPending)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const bool needsWrap = m_window &&
        QSGRendererInterface::isApiRhiBased(m_window->rendererInterface()->graphicsApi());
      if (needsWrap)
        m_window->beginExternalCommands();
#endif

      // Render VTK into it's framebuffer
      auto ostate = vtkWindow->GetState();
      ostate->Reset();
      ostate->Push();
      ostate->vtkglDepthFunc(GL_LEQUAL); // note: By default, Qt sets the depth function to GL_LESS
      // but VTK expects GL_LEQUAL
      vtkWindow->SetReadyForRendering(true);
      vtkWindow->GetInteractor()->ProcessEvents();
      vtkWindow->GetInteractor()->Render();
      vtkWindow->SetReadyForRendering(false);
      ostate->Pop();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      if (needsWrap)
        m_window->endExternalCommands();
#endif

      m_renderPending = false;
      markDirty(QSGNode::DirtyMaterial);
      Q_EMIT textureChanged();
    }
  }

  void handleScreenChange(QScreen*)
  {
    if (!m_window || !m_item)
      return;

    if (m_window->effectiveDevicePixelRatio() != m_devicePixelRatio)
    {

      m_item->update();
    }
  }

private:
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> vtkWindow;
  vtkSmartPointer<vtkObject> vtkUserData;
  bool m_renderPending = false;

protected:
  // variables set in QQuickVTKItem::updatePaintNode()
  QPointer<QQuickWindow> m_window;
  QPointer<QQuickVTKItem> m_item;
  qreal m_devicePixelRatio = 0;
  QSizeF m_size;
  friend class QQuickVTKItem;
};

QSGNode* QQuickVTKItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData*)
{
  auto* n = static_cast<QSGVtkObjectNode*>(node);

  // Don't create the node if our size is invalid
  if (!n && (width() <= 0 || height() <= 0))
    return nullptr;

  Q_D(QQuickVTKItem);

  // Create the QSGRenderNode
  if (!n)
  {
    if (!checkGraphicsApi(window()))
      return nullptr;
    if (!d->node)
      d->node = new QSGVtkObjectNode;
    n = d->node;
  }

  // Initialize the QSGRenderNode
  if (!n->m_item)
  {
    n->initialize(this);
    n->m_window = window();
    n->m_item = this;
    connect(window(), &QQuickWindow::beforeRendering, n, &QSGVtkObjectNode::render);
    connect(window(), &QQuickWindow::screenChanged, n, &QSGVtkObjectNode::handleScreenChange);
  }

  // Watch for size changes
  auto size = QSizeF(width(), height());
  n->m_devicePixelRatio = window()->devicePixelRatio();
  d->qt2vtkInteractorAdapter.SetDevicePixelRatio(n->m_devicePixelRatio);
  auto sz = size * n->m_devicePixelRatio;
  bool dirtySize = sz != n->m_size;
  if (dirtySize)
  {
    n->vtkWindow->SetSize(sz.width(), sz.height());
    n->vtkWindow->GetInteractor()->SetSize(n->vtkWindow->GetSize());
    delete n->texture();
    n->m_size = sz;
  }

  // Dispatch commands to VTK
  if (!d->asyncDispatch.empty())
  {
    n->scheduleRender();

    n->vtkWindow->SetReadyForRendering(true);
    while (!d->asyncDispatch.empty())
      d->asyncDispatch.dequeue()(n->vtkWindow, n->vtkUserData);
    n->vtkWindow->SetReadyForRendering(false);
  }

  // Whenever the size changes we need to get a new FBO from VTK so we need to render right now
  // (with the gui-thread blocked) for this one frame.
  if (dirtySize)
  {
    n->scheduleRender();
    n->render();
    auto fb = n->vtkWindow->GetDisplayFramebuffer();
    if (fb && fb->GetNumberOfColorAttachments() > 0)
    {
      GLuint texId = fb->GetColorAttachmentAsTextureObject(0)->GetHandle();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      auto* texture =
        window()->createTextureFromId(texId, sz.toSize(), QQuickWindow::TextureIsOpaque);
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      auto* texture = window()->createTextureFromNativeObject(
        QQuickWindow::NativeObjectTexture, &texId, 0, sz.toSize(), QQuickWindow::TextureIsOpaque);
#else
      auto* texture = QNativeInterface::QSGOpenGLTexture::fromNative(
        texId, window(), sz.toSize(), QQuickWindow::TextureIsOpaque);
#endif
      n->setTexture(texture);
    }
    else if (!fb)
      qFatal("%s %d %s", "QQuickVTKItem.cpp:", __LINE__,
        ", YIKES!!, Render() didn't create a FrameBuffer!?");
    else
      qFatal("%s %d %s", "QQuickVTKItem.cpp:", __LINE__,
        ", YIKES!!, Render() didn't create any ColorBufferAttachements in its FrameBuffer!?");
  }

  n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
  n->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
  n->setRect(0, 0, width(), height());

  if (d->scheduleRender)
  {
    n->scheduleRender();
    d->scheduleRender = false;
  }

  return n;
}

void QQuickVTKItem::scheduleRender()
{
  Q_D(QQuickVTKItem);

  d->scheduleRender = true;
  update();
}

bool QQuickVTKItem::isTextureProvider() const
{
  return true;
}

QSGTextureProvider* QQuickVTKItem::textureProvider() const
{
  // When Item::layer::enabled == true, QQuickItem will be a texture provider.
  // In this case we should prefer to return the layer rather than the VTK texture.
  if (QQuickItem::isTextureProvider())
    return QQuickItem::textureProvider();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QQuickWindow* w = window();
  if (!w || !w->openglContext() || QThread::currentThread() != w->openglContext()->thread())
  {
    qFatal("%s",
      "QQuickFramebufferObject::textureProvider: can only be queried on the rendering "
      "thread of an exposed window");
  }
#endif

  if (!checkGraphicsApi(window()))
    return nullptr;

  Q_D(const QQuickVTKItem);

  if (!d->node)
    d->node = new QSGVtkObjectNode;

  return d->node;
}

void QQuickVTKItem::releaseResources()
{
  // When release resources is called on the GUI thread, we only need to
  // forget about the node. Since it is the node we returned from updatePaintNode
  // it will be managed by the scene graph.
  Q_D(QQuickVTKItem);
  d->node = nullptr;
}

void QQuickVTKItem::invalidateSceneGraph()
{
  Q_D(QQuickVTKItem);
  d->node = nullptr;
}

bool QQuickVTKItem::event(QEvent* ev)
{
  Q_D(QQuickVTKItem);

  if (!ev)
    return false;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  switch (ev->type())
  {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    {
      auto e = static_cast<QHoverEvent*>(ev);
      auto c =
        QSharedPointer<QHoverEvent>::create(e->type(), e->posF(), e->oldPosF(), e->modifiers());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::Enter:
    {
      auto e = static_cast<QEnterEvent*>(ev);
      auto c = QSharedPointer<QEnterEvent>::create(e->localPos(), e->windowPos(), e->screenPos());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::Leave:
    {
      auto e = static_cast<QEvent*>(ev);
      auto c = QSharedPointer<QEvent>::create(e->type());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::DragEnter:
    {
      auto e = static_cast<QDragEnterEvent*>(ev);
      auto c = QSharedPointer<QDragEnterEvent>::create(
        e->pos(), e->possibleActions(), e->mimeData(), e->mouseButtons(), e->keyboardModifiers());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::DragLeave:
    {
      auto c = QSharedPointer<QDragLeaveEvent>::create();
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::DragMove:
    {
      auto e = static_cast<QDragMoveEvent*>(ev);
      auto c = QSharedPointer<QDragMoveEvent>::create(
        e->pos(), e->possibleActions(), e->mimeData(), e->mouseButtons(), e->keyboardModifiers());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::Drop:
    {
      auto e = static_cast<QDropEvent*>(ev);
      auto c = QSharedPointer<QDropEvent>::create(
        e->pos(), e->possibleActions(), e->mimeData(), e->mouseButtons(), e->keyboardModifiers());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::ContextMenu:
    {
      auto e = static_cast<QContextMenuEvent*>(ev);
      auto c = QSharedPointer<QContextMenuEvent>::create(
        e->reason(), e->pos(), e->globalPos(), e->modifiers());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
      auto e = static_cast<QKeyEvent*>(ev);
      auto c =
        QSharedPointer<QKeyEvent>::create(e->type(), e->key(), e->modifiers(), e->nativeScanCode(),
          e->nativeVirtualKey(), e->nativeModifiers(), e->text(), e->isAutoRepeat(), e->count());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    {
      auto e = static_cast<QFocusEvent*>(ev);
      auto c = QSharedPointer<QFocusEvent>::create(e->type(), e->reason());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    {
      auto e = static_cast<QMouseEvent*>(ev);
      auto c = QSharedPointer<QMouseEvent>::create(e->type(), e->localPos(), e->windowPos(),
        e->screenPos(), e->button(), e->buttons(), e->modifiers(), e->source());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
    {
      auto e = static_cast<QWheelEvent*>(ev);
      auto c =
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
        QSharedPointer<QWheelEvent>::create(e->pos(), e->globalPos(), e->pixelDelta(),
          e->angleDelta(), e->delta(), e->orientation(), e->buttons(), e->modifiers(), e->phase(),
          e->source(), e->inverted());
#elif QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QSharedPointer<QWheelEvent>::create(e->pos(), e->globalPos(), e->pixelDelta(),
          e->angleDelta(), e->buttons(), e->modifiers(), e->phase(), e->inverted(), e->source());
#else
        QSharedPointer<QWheelEvent>::create(e->position(), e->globalPosition(), e->pixelDelta(),
          e->angleDelta(), e->buttons(), e->modifiers(), e->phase(), e->inverted(), e->source());
#endif
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
#endif
#ifndef NO_TOUCH
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    {
      auto e = static_cast<QTouchEvent*>(ev);
      auto c = QSharedPointer<QTouchEvent>::create(
        e->type(), e->device(), e->modifiers(), e->touchPointStates(), e->touchPoints());
      dispatch_async([d, c](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
        d->qt2vtkInteractorAdapter.ProcessEvent(c.data(), vtkWindow->GetInteractor());
      });
      break;
    }
#endif
    default:
      return QQuickItem::event(ev);
  }
#else
  auto e = ev->clone();
  dispatch_async([d, e](vtkRenderWindow* vtkWindow, vtkUserData) mutable {
    d->qt2vtkInteractorAdapter.ProcessEvent(e, vtkWindow->GetInteractor());
    delete e;
  });
#endif

  ev->accept();

  return true;
}
VTK_ABI_NAMESPACE_END

#include "QQuickVTKItem.moc"
