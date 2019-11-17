#include "TestQtCommon.h"

namespace detail
{

Type select_widget(int argc, char* argv[])
{
  for (int cc = 0; cc < argc; ++cc)
  {
    if (argv[cc] && strcmp(argv[cc], "-w") == 0 && (cc + 1) < argc)
    {
      auto typestr = argv[cc + 1];
      if (strcmp(typestr, "QVTKOpenGLNativeWidget") == 0)
      {
        return Type::USE_QVTKOPENGLNATIVEWIDGET;
      }
      else if (strcmp(typestr, "QVTKOpenGLWindow") == 0)
      {
        return Type::USE_QVTKOPENGLWINDOW;
      }
      else if (strcmp(typestr, "QVTKOpenGLWidget") == 0)
      {
        return Type::USE_QVTKOPENGLWIDGET;
      }
    }
  }
  // default.
  return Type::USE_QVTKOPENGLNATIVEWIDGET;
}

void set_default_format(Type type)
{
  switch (type)
  {
    case Type::USE_QVTKOPENGLNATIVEWIDGET:
      vtkLogF(INFO, "setting default QSurfaceFormat.");
      QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
      break;

    default:
      vtkLogF(INFO, "no need to set default format, skipping.");
      break;
  }
}

std::shared_ptr<QObject> create_widget_or_window(Type type, vtkGenericOpenGLRenderWindow* renWin)
{
  switch (type)
  {
    case Type::USE_QVTKOPENGLNATIVEWIDGET:
    {
      vtkLogF(INFO, "creating QVTKOpenGLNativeWidget.");
      auto widget = std::make_shared<QVTKOpenGLNativeWidget>();
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
    case Type::USE_QVTKOPENGLWINDOW:
    {
      vtkLogF(INFO, "creating QVTKOpenGLWindow.");
      auto widget = std::make_shared<QVTKOpenGLWindow>();
      vtkLogF(INFO, "set format on Qt window explicitly");
      widget->setFormat(QVTKOpenGLWindow::defaultFormat());
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
    case Type::USE_QVTKOPENGLWIDGET:
    {
      vtkLogF(INFO, "creating QVTKOpenGLWidget.");
      auto widget = std::make_shared<QVTKOpenGLWidget>();
      vtkLogF(INFO, "set format on Qt widget explicitly");
      widget->setFormat(QVTKOpenGLWindow::defaultFormat());
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
  }
  return nullptr;
}

std::shared_ptr<QWidget> create_widget(
  Type type, vtkGenericOpenGLRenderWindow* renWin, QWidget* parent)
{
  auto widget = std::dynamic_pointer_cast<QWidget>(create_widget_or_window(type, renWin));
  if (widget && parent)
  {
    widget->setParent(parent);
  }
  return widget;
}

vtkRenderWindow* get_render_window(std::shared_ptr<QObject> widgetOrWindow)
{
  if (auto w1 = qobject_cast<QVTKOpenGLWidget*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }

  if (auto w1 = qobject_cast<QVTKOpenGLNativeWidget*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }

  if (auto w1 = qobject_cast<QVTKOpenGLWindow*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }
  return nullptr;
}

void set_render_window(std::shared_ptr<QObject> widgetOrWindow, vtkRenderWindow* renWin)
{
  if (auto w1 = qobject_cast<QVTKOpenGLWidget*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }

  if (auto w1 = qobject_cast<QVTKOpenGLNativeWidget*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }

  if (auto w1 = qobject_cast<QVTKOpenGLWindow*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }
}

void process_events_and_wait(int msec)
{
  QApplication::sendPostedEvents();
  QApplication::processEvents();

  if (msec > 0)
  {
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    loop.exec();
  }

  QApplication::sendPostedEvents();
  QApplication::processEvents();
  QApplication::sendPostedEvents();
  QApplication::processEvents();
}

void show(std::shared_ptr<QObject> widgetOrWindow, const QSize& size)
{
  if (widgetOrWindow->isWidgetType())
  {
    auto widget = static_cast<QWidget*>(widgetOrWindow.get());
    widget->resize(size);
    widget->show();
  }
  else if (widgetOrWindow->isWindowType())
  {
    auto window = static_cast<QWindow*>(widgetOrWindow.get());
    window->resize(size);
    window->show();
  }

  auto renWindow = vtkGenericOpenGLRenderWindow::SafeDownCast(get_render_window(widgetOrWindow));
  while (renWindow != nullptr && !renWindow->GetReadyForRendering())
  {
    QApplication::sendPostedEvents();
    QApplication::processEvents();
  }
  process_events_and_wait(500);
}

QImage grab_framebuffer(std::shared_ptr<QObject> widgetOrWindow)
{
  if (auto w1 = qobject_cast<QVTKOpenGLWidget*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }

  if (auto w1 = qobject_cast<QVTKOpenGLNativeWidget*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }

  if (auto w1 = qobject_cast<QVTKOpenGLWindow*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }
  return QImage();
}
}
