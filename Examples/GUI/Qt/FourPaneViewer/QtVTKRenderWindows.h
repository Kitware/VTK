#ifndef QtVTKRenderWindows_H
#define QtVTKRenderWindows_H

#include "vtkDistanceWidget.h"
#include "vtkImagePlaneWidget.h"
#include "vtkResliceImageViewer.h"
#include "vtkResliceImageViewerMeasurements.h"
#include "vtkSmartPointer.h"
#include <QMainWindow>

// Forward Qt class declarations
class Ui_QtVTKRenderWindows;

class QtVTKRenderWindows : public QMainWindow
{
  Q_OBJECT
public:
  // Constructor/Destructor
  QtVTKRenderWindows(int argc, char* argv[]);
  ~QtVTKRenderWindows() override {}

public Q_SLOTS:

  virtual void slotExit();
  virtual void resliceMode(int);
  virtual void thickMode(int);
  virtual void SetBlendModeToMaxIP();
  virtual void SetBlendModeToMinIP();
  virtual void SetBlendModeToMeanIP();
  virtual void SetBlendMode(int);
  virtual void ResetViews();
  virtual void Render();
  virtual void AddDistanceMeasurementToView1();
  virtual void AddDistanceMeasurementToView(int);

protected:
  vtkSmartPointer<vtkResliceImageViewer> riw[3];
  vtkSmartPointer<vtkImagePlaneWidget> planeWidget[3];
  vtkSmartPointer<vtkDistanceWidget> DistanceWidget[3];
  vtkSmartPointer<vtkResliceImageViewerMeasurements> ResliceMeasurements;

protected Q_SLOTS:

private:
  // Designer form
  Ui_QtVTKRenderWindows* ui;
};

#endif // QtVTKRenderWindows_H
