#include <QVTKOpenGLWidget.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <QApplication>
#include <QBoxLayout>
#include <QSurfaceFormat>
#include <QWidget>

int TestQVTKOpenGLWidgetSwapWindows(int argc, char* argv[])
{
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
  QApplication app(argc, argv);

  // Set up frame with two horizontally stacked panels,
  // Each containing a QVTKOpenGLWidget
  QWidget frame;
  QHBoxLayout *layout = new QHBoxLayout(&frame);

  QWidget *leftPanel = new QWidget(&frame);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
  QVTKOpenGLWidget* leftVTKWidget = new QVTKOpenGLWidget(leftPanel);
  vtkSmartPointer<vtkRenderer> leftRenderer = vtkSmartPointer<vtkRenderer>::New();
  leftRenderer->SetBackground(1, 0, 0);
  leftVTKWidget->GetRenderWindow()->AddRenderer(leftRenderer);
  leftLayout->addWidget(leftVTKWidget);

  QWidget *rightPanel = new QWidget(&frame);
  QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
  QVTKOpenGLWidget* rightVTKWidget = new QVTKOpenGLWidget(rightPanel);
  vtkSmartPointer<vtkRenderer> rightRenderer = vtkSmartPointer<vtkRenderer>::New();
  rightRenderer->SetBackground(0, 1, 0);
  rightVTKWidget->GetRenderWindow()->AddRenderer(rightRenderer);
  rightLayout->addWidget(rightVTKWidget);

  layout->addWidget(leftPanel);
  layout->addWidget(rightPanel);

  // Show stuff and process events
  frame.show();
  leftVTKWidget->GetRenderWindow()->Render();
  rightVTKWidget->GetRenderWindow()->Render();
  app.processEvents();

  // Swap QVTKOpenGLWidget
  rightLayout->removeWidget(rightVTKWidget);
  leftLayout->removeWidget(leftVTKWidget);
  rightVTKWidget->setParent(leftPanel);
  leftVTKWidget->setParent(rightPanel);
  rightLayout->addWidget(leftVTKWidget);
  leftLayout->addWidget(rightVTKWidget);

  // Process events again
  leftVTKWidget->GetRenderWindow()->Render();
  rightVTKWidget->GetRenderWindow()->Render();
  app.processEvents();
  return EXIT_SUCCESS;
}
