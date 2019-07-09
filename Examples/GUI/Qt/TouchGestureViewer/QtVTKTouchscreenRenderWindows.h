#ifndef QtVTKTouchscreenRenderWindows_H
#define QtVTKTouchscreenRenderWindows_H

#include "vtkSmartPointer.h"
#include "vtkResliceImageViewer.h"
#include "vtkImagePlaneWidget.h"
#include "vtkDistanceWidget.h"
#include "vtkResliceImageViewerMeasurements.h"
#include <QMainWindow>

// Forward Qt class declarations
class Ui_QtVTKTouchscreenRenderWindows;

class QtVTKTouchscreenRenderWindows : public QMainWindow
{
  Q_OBJECT
public:

  // Constructor/Destructor
  QtVTKTouchscreenRenderWindows(int argc, char* argv[]);
  ~QtVTKTouchscreenRenderWindows() override {}

private:

  // Designer form
  Ui_QtVTKTouchscreenRenderWindows* ui;
};

#endif // QtVTKTouchscreenRenderWindows_H
