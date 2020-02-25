#include "QVTKOpenGLNativeWidget.h"
#include "QVTKOpenGLStereoWidget.h"
#include "QVTKOpenGLWindow.h"
#include "QVTKRenderWidget.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkLogger.h"

#ifndef VTK_LEGACY_REMOVE
#include "QVTKOpenGLWidget.h"
#endif

#include <QApplication>
#include <QEventLoop>
#include <QScopedPointer>
#include <QSurfaceFormat>
#include <QTimer>
#include <memory>

namespace detail
{
enum class Type
{
  USE_QVTKRENDERWIDGET = 0,
  USE_QVTKOPENGLNATIVEWIDGET = 1,
  USE_QVTKOPENGLWINDOW = 2,
  USE_QVTKOPENGLSTEREOWIDGET = 3,
  USE_QVTKOPENGLWIDGET = 4
};

Type select_widget(int argc, char* argv[]);
void set_default_format(Type type);
std::shared_ptr<QObject> create_widget_or_window(Type type, vtkGenericOpenGLRenderWindow* renWin);
std::shared_ptr<QWidget> create_widget(
  Type type, vtkGenericOpenGLRenderWindow* renWin, QWidget* parent);
vtkRenderWindow* get_render_window(std::shared_ptr<QObject> widgetOrWindow);
void set_render_window(std::shared_ptr<QObject> widgetOrWindow, vtkRenderWindow* renWin);
void process_events_and_wait(int msec);
void show(std::shared_ptr<QObject> widgetOrWindow, const QSize& size);
QImage grab_framebuffer(std::shared_ptr<QObject> widgetOrWindow);

}
