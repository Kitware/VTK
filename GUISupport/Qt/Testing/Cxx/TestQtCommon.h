#include "QVTKOpenGLNativeWidget.h"
#include "QVTKOpenGLWidget.h"
#include "QVTKOpenGLWindow.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkLogger.h"

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
  USE_QVTKOPENGLNATIVEWIDGET = 0,
  USE_QVTKOPENGLWINDOW = 1,
  USE_QVTKOPENGLWIDGET = 2
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
