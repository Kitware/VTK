#include <QVTKOpenGLNativeWidget.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <QApplication>
#include <QBoxLayout>
#include <QSurfaceFormat>
#include <QWidget>

int TestQVTKOpenGLNativeWidgetSwapWindows(int argc, char* argv[])
{
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
  QApplication app(argc, argv);

  // Set up frame with two horizontally stacked panels,
  // Each containing a QVTKOpenGLNativeWidget
  QWidget frame;
  QHBoxLayout *layout = new QHBoxLayout(&frame);

  QWidget *leftPanel = new QWidget(&frame);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
  QVTKOpenGLNativeWidget* leftVTKWidget = new QVTKOpenGLNativeWidget(leftPanel);
  vtkSmartPointer<vtkRenderer> leftRenderer = vtkSmartPointer<vtkRenderer>::New();
  leftRenderer->SetBackground(1, 0, 0);
  leftVTKWidget->GetRenderWindow()->AddRenderer(leftRenderer);
  leftLayout->addWidget(leftVTKWidget);

  QWidget *rightPanel = new QWidget(&frame);
  QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
  QVTKOpenGLNativeWidget* rightVTKWidget = new QVTKOpenGLNativeWidget(rightPanel);
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

  // Swap QVTKOpenGLNativeWidget
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
