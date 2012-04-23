#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkImagePlaneWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkKochanekSpline.h"
#include "vtkParametricSpline.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSplineWidget.h"
#include "vtkTextProperty.h"

// Callback for the spline widget interaction
class vtkSplineWidgetCallback : public vtkCommand
{
public:
  static vtkSplineWidgetCallback *New()
    { return new vtkSplineWidgetCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkSplineWidget *spline = reinterpret_cast<vtkSplineWidget*>(caller);
      spline->GetPolyData(Poly);
    }
  vtkSplineWidgetCallback():Poly(0){};
  vtkPolyData* Poly;
};

int main( int, char *[] )
{
  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
    renWin->AddRenderer( ren1);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow( renWin);

    planeSource->Update();

  vtkPlaneSource* planeSource;
    planeSource->Update();

  vtkPolyDataMapper* planeSourceMapper = vtkPolyDataMapper::New();
    planeSourceMapper->SetInput(planeSource->GetOutput());
  vtkActor* planeSourceActor = vtkActor::New();
    planeSourceActor->SetMapper(planeSourceMapper);

  vtkSplineWidget* spline = vtkSplineWidget::New();
    spline->SetInteractor( iren);
    spline->SetInput(planeSource->GetOutput());
    spline->SetPriority(1.0);
    spline->KeyPressActivationOff();
    spline->PlaceWidget();
    spline->ProjectToPlaneOn();
    spline->SetProjectionNormal(0);
    spline->SetProjectionPosition(102.4);  //initial plane oriented position
    spline->SetProjectionNormal(3); //allow arbitrary oblique orientations
    spline->SetPlaneSource(planeSource);


  // Specify the type of spline (change from default vtkCardinalSpline)
  vtkKochanekSpline* xspline = vtkKochanekSpline::New();
  vtkKochanekSpline* yspline = vtkKochanekSpline::New();
  vtkKochanekSpline* zspline = vtkKochanekSpline::New();

  vtkParametricSpline* para = spline->GetParametricSpline();

  para->SetXSpline(xspline);
  para->SetYSpline(yspline);
  para->SetZSpline(zspline);

  vtkPolyData* poly = vtkPolyData::New();
    spline->GetPolyData(poly);

  vtkSplineWidgetCallback* swcb = vtkSplineWidgetCallback::New();
    swcb->Poly = poly;

  spline->AddObserver(vtkCommand::InteractionEvent,swcb);

  ren1->SetBackground( 0.1, 0.2, 0.4);
  ren1->AddActor(planeSourceActor);

  renWin->SetSize( 600, 300);
  renWin->Render();

  spline->On();
  spline->SetNumberOfHandles(4);
  spline->SetNumberOfHandles(5);
  spline->SetResolution(399);

  // Set up an interesting viewpoint
  vtkCamera* camera = ren1->GetActiveCamera();

  // Render the image
  iren->Initialize();
  renWin->Render();

  return EXIT_SUCCESS;
}


