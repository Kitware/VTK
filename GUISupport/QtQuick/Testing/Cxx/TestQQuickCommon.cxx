// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestQQuickCommon.h"
#include "QQuickVTKItem.h"
#include "QVTKRenderWindowAdapter.h"
#include "vtkPNGWriter.h"
#include "vtkTesting.h"
#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QQuickView>
#include <QTimer>
#include <QUrl>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlApplicationEngine>
#include <QQuickRenderControl>
#include <QQuickWindow>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickGraphicsDevice>
#include <QQuickRenderTarget>
#endif

namespace detail
{
int performTest(int argc, char* argv[], const char* filename)
{
  QUrl url(filename);
  QImage im;

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  if (vtktesting->IsInteractiveModeSpecified())
  {
    QQuickView view;
    view.setSource(url);
    view.show();

    QApplication::exec();
    return EXIT_SUCCESS;
  }

  struct Offscreen : QObject
  {
    Offscreen(QUrl url)
    {
      _context = new QOpenGLContext;
      _context->setFormat(QSurfaceFormat::defaultFormat());
      if (!_context->create())
        exit(EXIT_FAILURE);

      _offscreenSurface = new QOffscreenSurface;
      _offscreenSurface->setFormat(_context->format());
      _offscreenSurface->create();

      _renderControl = new QQuickRenderControl;
      _quickWindow = new QQuickWindow(_renderControl);
      _engine = new QQmlEngine;
      if (!_engine->incubationController())
        _engine->setIncubationController(_quickWindow->incubationController());

      _updateTimer.setSingleShot(true);
      _updateTimer.setInterval(5);
      connect(&_updateTimer, &QTimer::timeout, this, [this] { render(); });

      connect(_quickWindow, &QQuickWindow::sceneGraphInitialized, this, [this] {
        _fbo = new QOpenGLFramebufferObject(
          _quickWindow->size(), QOpenGLFramebufferObject::CombinedDepthStencil);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        _quickWindow->setRenderTarget(_fbo);
#else
        _quickWindow->setRenderTarget(QQuickRenderTarget::fromOpenGLTexture(_fbo->texture(), _fbo->size()));
#endif
      });
      connect(_quickWindow, &QQuickWindow::sceneGraphInvalidated, this, [this] {
        delete _fbo;
        _fbo = nullptr;
      });
      connect(_renderControl, &QQuickRenderControl::renderRequested, this, [this] {
        if (!_updateTimer.isActive())
          _updateTimer.start();
      });
      connect(_renderControl, &QQuickRenderControl::sceneChanged, this, [this] {
        if (!_updateTimer.isActive())
          _updateTimer.start();
      });

      _component = new QQmlComponent(_engine, url);
      if (_component->isLoading())
        connect(_component, &QQmlComponent::statusChanged, this, [this]() { run(); });
      else
        run();
    }
    ~Offscreen() override
    {
      _context->makeCurrent(_offscreenSurface);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      delete _renderControl;
#endif
      delete _component;
      delete _quickWindow;
      delete _engine;
      delete _fbo;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      delete _renderControl;
#endif
      _context->doneCurrent();
      delete _offscreenSurface;
      delete _context;
    }
    void run()
    {
      auto reportFailure = [this] {
        if (_component->isError())
        {
          const QList<QQmlError> errorList = _component->errors();
          for (const QQmlError& error : errorList)
            qWarning() << error.url() << error.line() << error;
          exit(EXIT_FAILURE);
        }
      };

      disconnect(_component, &QQmlComponent::statusChanged, this, nullptr);
      reportFailure();

      auto* rootObject = _component->create();
      reportFailure();

      _rootItem = qobject_cast<QQuickItem*>(rootObject);
      if (!_rootItem)
      {
        qWarning() << "run: Not a QQuickItem";
        exit(EXIT_FAILURE);
      }
      _rootItem->setParentItem(_quickWindow->contentItem());
      _rootItem->setParent(_quickWindow);

      auto width = _rootItem->width();
      auto height = _rootItem->height();
      _quickWindow->setGeometry(0, 0, width, height);

      if (!_context->makeCurrent(_offscreenSurface))
        exit(EXIT_FAILURE);
#if QT_VERSION <= QT_VERSION_CHECK(6, 0, 0)
      _renderControl->initialize(_context);
#else
      _quickWindow->setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(_context));
      if (!_renderControl->initialize())
        exit(EXIT_FAILURE);
#endif
    }
    void render()
    {
      if (!_context->makeCurrent(_offscreenSurface))
        exit(EXIT_FAILURE);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      _renderControl->beginFrame();
#endif
      _renderControl->polishItems();
      _renderControl->sync();
      _renderControl->render();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      _renderControl->endFrame();
#endif
      QOpenGLFramebufferObject::bindDefault();
      _context->functions()->glFlush();

      while (auto err = _context->functions()->glGetError() != GL_NO_ERROR)
        qWarning() << "YIKES!, glGetError(): " << err;
    }
    QOpenGLContext* _context;
    QOffscreenSurface* _offscreenSurface;
    QQuickRenderControl* _renderControl;
    QQuickWindow* _quickWindow;
    QQmlEngine* _engine;
    QQmlComponent* _component;
    QOpenGLFramebufferObject* _fbo;
    QQuickItem* _rootItem;
    QTimer _updateTimer;
    QImage _im;

  } offscreen(url);

  // Wait a little for the application and window to be set up properly
  QEventLoop loop;
  QTimer::singleShot(1000, &loop, SLOT(quit()));
  loop.exec();

  // Capture a screenshot of the item
  im = offscreen._fbo->toImage();
  if (im.isNull())
    return EXIT_FAILURE;
  if (!im.reinterpretAsFormat(QImage::Format_RGB32))
    return EXIT_FAILURE;

  std::string validName = std::string(vtktesting->GetValidImageFileName());
  std::string::size_type slashPos = validName.rfind('/');
  if (slashPos != std::string::npos)
  {
    validName = validName.substr(slashPos + 1);
  }
  std::string tmpDir = vtktesting->GetTempDirectory();
  std::string vImage = tmpDir + "/" + validName;
  im.save(QString::fromStdString(vImage), "PNG");

  int retVal = vtktesting->RegressionTest(vImage, 0.05);

  switch (retVal)
  {
    case vtkTesting::FAILED:
    case vtkTesting::NOT_RUN:
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
}
