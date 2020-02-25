#include <QApplication>
#include <QSurfaceFormat>

#include "QVTKOpenGLStereoWidget.h"
#include "QtVTKRenderWindows.h"

int main(int argc, char** argv)
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLStereoWidget::defaultFormat());

  // QT Stuff
  QApplication app(argc, argv);

  QtVTKRenderWindows myQtVTKRenderWindows(argc, argv);
  myQtVTKRenderWindows.show();

  return app.exec();
}
