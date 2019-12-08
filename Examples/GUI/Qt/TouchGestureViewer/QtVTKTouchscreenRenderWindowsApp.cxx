#include <QApplication>
#include <QSurfaceFormat>

#include "QVTKOpenGLWidget.h"
#include "QtVTKTouchscreenRenderWindows.h"

int main(int argc, char** argv)
{
  // Needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat format = QVTKOpenGLWidget::defaultFormat();
#if _WINDOWS
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
#endif
  QSurfaceFormat::setDefaultFormat(format);

  // QT Stuff
  QApplication app(argc, argv);

  QtVTKTouchscreenRenderWindows myQtVTKTouchscreenRenderWindows(argc, argv);
  myQtVTKTouchscreenRenderWindows.show();

  return app.exec();
}
