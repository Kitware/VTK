
#include <QApplication>
#include "GraphicsView.hpp"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  GraphicsView view;
  view.show();

  view.resize(800, 600);
  return app.exec();
}
