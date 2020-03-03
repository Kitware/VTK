#include "TestQtCommon.h"
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <QApplication>
#include <QBoxLayout>
#include <QSurfaceFormat>
#include <QWidget>

int TestQtSwapWindows(int argc, char* argv[])
{
  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  // Set up frame with two horizontally stacked panels,
  // Each containing a QVTKOpenGLStereoWidget
  QWidget frame;
  QHBoxLayout* layout = new QHBoxLayout(&frame);

  QWidget* leftPanel = new QWidget(&frame);
  QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
  auto leftVTKWidget = detail::create_widget(type, nullptr, leftPanel);
  vtkSmartPointer<vtkRenderer> leftRenderer = vtkSmartPointer<vtkRenderer>::New();
  leftRenderer->SetBackground(1, 0, 0);
  detail::get_render_window(leftVTKWidget)->AddRenderer(leftRenderer);
  leftLayout->addWidget(leftVTKWidget.get());

  QWidget* rightPanel = new QWidget(&frame);
  QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
  auto rightVTKWidget = detail::create_widget(type, nullptr, rightPanel);
  vtkSmartPointer<vtkRenderer> rightRenderer = vtkSmartPointer<vtkRenderer>::New();
  rightRenderer->SetBackground(0, 1, 0);
  detail::get_render_window(rightVTKWidget)->AddRenderer(rightRenderer);
  rightLayout->addWidget(rightVTKWidget.get());

  layout->addWidget(leftPanel);
  layout->addWidget(rightPanel);

  // Show stuff and process events
  frame.show();
  detail::get_render_window(leftVTKWidget)->Render();
  detail::get_render_window(rightVTKWidget)->Render();
  app.processEvents();

  // Swap QVTKOpenGLStereoWidget
  rightLayout->removeWidget(rightVTKWidget.get());
  leftLayout->removeWidget(leftVTKWidget.get());
  rightVTKWidget->setParent(leftPanel);
  leftVTKWidget->setParent(rightPanel);
  rightLayout->addWidget(leftVTKWidget.get());
  leftLayout->addWidget(rightVTKWidget.get());

  // Process events again
  detail::get_render_window(leftVTKWidget)->Render();
  detail::get_render_window(rightVTKWidget)->Render();
  app.processEvents();
  return EXIT_SUCCESS;
}
