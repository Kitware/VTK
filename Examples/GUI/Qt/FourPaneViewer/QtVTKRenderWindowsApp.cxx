#include <QApplication>
#include "QtVTKRenderWindows.h"

int main( int argc, char** argv )
{
  // QT Stuff
  QApplication app( argc, argv );

  QtVTKRenderWindows myQtVTKRenderWindows(argc, argv);
  myQtVTKRenderWindows.show();

  return app.exec();
}
