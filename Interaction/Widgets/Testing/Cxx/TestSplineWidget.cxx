#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkImagePlaneWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkKochanekSpline.h"
#include "vtkOutlineFilter.h"
#include "vtkParametricSpline.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSplineWidget.h"
#include "vtkTextProperty.h"
#include "vtkVolume16Reader.h"
#include "vtkXYPlotActor.h"

#include "vtkTestUtilities.h"

static char TSWeventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 133 125 0 0 98 1 i\n"
  "KeyReleaseEvent 133 125 0 0 98 1 i\n"
  "MouseMoveEvent 133 125 0 0 0 0 i\n"
  "RightButtonPressEvent 133 125 0 0 0 0 i\n"
  "MouseMoveEvent 133 123 0 0 0 0 i\n"
  "MouseMoveEvent 133 119 0 0 0 0 i\n"
  "MouseMoveEvent 132 115 0 0 0 0 i\n"
  "MouseMoveEvent 132 111 0 0 0 0 i\n"
  "MouseMoveEvent 132 107 0 0 0 0 i\n"
  "RightButtonReleaseEvent 132 107 0 0 0 0 i\n"
  "MouseMoveEvent 132 129 0 0 0 0 i\n"
  "LeftButtonPressEvent 132 129 0 0 0 0 i\n"
  "MouseMoveEvent 132 130 0 0 0 0 i\n"
  "MouseMoveEvent 132 135 0 0 0 0 i\n"
  "MouseMoveEvent 132 143 0 0 0 0 i\n"
  "MouseMoveEvent 131 152 0 0 0 0 i\n"
  "MouseMoveEvent 130 159 0 0 0 0 i\n"
  "MouseMoveEvent 129 165 0 0 0 0 i\n"
  "MouseMoveEvent 127 170 0 0 0 0 i\n"
  "MouseMoveEvent 125 176 0 0 0 0 i\n"
  "MouseMoveEvent 124 181 0 0 0 0 i\n"
  "MouseMoveEvent 122 183 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 122 183 0 0 0 0 i\n"
  "MouseMoveEvent 133 163 0 0 0 0 i\n"
  "MiddleButtonPressEvent 133 163 0 0 0 0 i\n"
  "MouseMoveEvent 132 161 0 0 0 0 i\n"
  "MouseMoveEvent 128 158 0 0 0 0 i\n"
  "MouseMoveEvent 124 155 0 0 0 0 i\n"
  "MouseMoveEvent 120 151 0 0 0 0 i\n"
  "MouseMoveEvent 116 147 0 0 0 0 i\n"
  "MouseMoveEvent 118 146 0 0 0 0 i\n"
  "MouseMoveEvent 121 148 0 0 0 0 i\n"
  "MouseMoveEvent 123 150 0 0 0 0 i\n"
  "MouseMoveEvent 125 154 0 0 0 0 i\n"
  "MouseMoveEvent 129 158 0 0 0 0 i\n"
  "MouseMoveEvent 132 161 0 0 0 0 i\n"
  "MouseMoveEvent 134 165 0 0 0 0 i\n"
  "MouseMoveEvent 136 168 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 136 168 0 0 0 0 i\n"
  "MouseMoveEvent 178 186 0 0 0 0 i\n"
  "KeyPressEvent 178 186 -128 0 0 1 Control_L\n"
  "MiddleButtonPressEvent 178 186 8 0 0 0 Control_L\n"
  "MouseMoveEvent 178 185 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 183 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 181 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 179 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 177 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 175 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 173 8 0 0 0 Control_L\n"
  "MouseMoveEvent 179 171 8 0 0 0 Control_L\n"
  "MouseMoveEvent 177 169 8 0 0 0 Control_L\n"
  "MouseMoveEvent 176 167 8 0 0 0 Control_L\n"
  "MouseMoveEvent 174 165 8 0 0 0 Control_L\n"
  "MouseMoveEvent 172 164 8 0 0 0 Control_L\n"
  "MouseMoveEvent 171 163 8 0 0 0 Control_L\n"
  "MiddleButtonReleaseEvent 171 163 8 0 0 0 Control_L\n"
  "KeyReleaseEvent 171 163 0 0 0 1 Control_L\n"
  "MouseMoveEvent 170 167 0 0 0 0 Control_L\n"
  "MiddleButtonPressEvent 170 167 0 0 0 0 Control_L\n"
  "MouseMoveEvent 172 167 0 0 0 0 Control_L\n"
  "MouseMoveEvent 176 167 0 0 0 0 Control_L\n"
  "MouseMoveEvent 181 167 0 0 0 0 Control_L\n"
  "MouseMoveEvent 188 167 0 0 0 0 Control_L\n"
  "MouseMoveEvent 198 165 0 0 0 0 Control_L\n"
  "MouseMoveEvent 205 163 0 0 0 0 Control_L\n"
  "MouseMoveEvent 211 161 0 0 0 0 Control_L\n"
  "MouseMoveEvent 216 160 0 0 0 0 Control_L\n"
  "MouseMoveEvent 222 158 0 0 0 0 Control_L\n"
  "MiddleButtonReleaseEvent 222 158 0 0 0 0 Control_L\n"
  "MouseMoveEvent 230 158 0 0 0 0 Control_L\n"
  "MiddleButtonPressEvent 230 158 0 0 0 0 Control_L\n"
  "MouseMoveEvent 229 156 0 0 0 0 Control_L\n"
  "MouseMoveEvent 228 153 0 0 0 0 Control_L\n"
  "MouseMoveEvent 226 150 0 0 0 0 Control_L\n"
  "MouseMoveEvent 224 148 0 0 0 0 Control_L\n"
  "MouseMoveEvent 222 145 0 0 0 0 Control_L\n"
  "MouseMoveEvent 220 141 0 0 0 0 Control_L\n"
  "MouseMoveEvent 216 135 0 0 0 0 Control_L\n"
  "MouseMoveEvent 214 129 0 0 0 0 Control_L\n"
  "MouseMoveEvent 212 123 0 0 0 0 Control_L\n"
  "MouseMoveEvent 209 118 0 0 0 0 Control_L\n"
  "MouseMoveEvent 207 113 0 0 0 0 Control_L\n"
  "MouseMoveEvent 204 109 0 0 0 0 Control_L\n"
  "MouseMoveEvent 202 105 0 0 0 0 Control_L\n"
  "MouseMoveEvent 200 103 0 0 0 0 Control_L\n"
  "MouseMoveEvent 198 99 0 0 0 0 Control_L\n"
  "MouseMoveEvent 196 97 0 0 0 0 Control_L\n"
  "MouseMoveEvent 194 93 0 0 0 0 Control_L\n"
  "MiddleButtonReleaseEvent 194 93 0 0 0 0 Control_L\n"
  "MouseMoveEvent 254 98 0 0 0 0 Control_L\n"
  "MiddleButtonPressEvent 254 98 0 0 0 0 Control_L\n"
  "MouseMoveEvent 254 100 0 0 0 0 Control_L\n"
  "MouseMoveEvent 254 104 0 0 0 0 Control_L\n"
  "MouseMoveEvent 255 108 0 0 0 0 Control_L\n"
  "MouseMoveEvent 255 112 0 0 0 0 Control_L\n"
  "MouseMoveEvent 255 116 0 0 0 0 Control_L\n"
  "MouseMoveEvent 255 120 0 0 0 0 Control_L\n"
  "MouseMoveEvent 256 124 0 0 0 0 Control_L\n"
  "MouseMoveEvent 257 128 0 0 0 0 Control_L\n"
  "MouseMoveEvent 257 132 0 0 0 0 Control_L\n"
  "MouseMoveEvent 257 136 0 0 0 0 Control_L\n"
  "MouseMoveEvent 258 141 0 0 0 0 Control_L\n"
  "MouseMoveEvent 258 146 0 0 0 0 Control_L\n"
  "MouseMoveEvent 258 151 0 0 0 0 Control_L\n"
  "MouseMoveEvent 258 157 0 0 0 0 Control_L\n"
  "MouseMoveEvent 258 159 0 0 0 0 Control_L\n"
  "MiddleButtonReleaseEvent 80 206 0 0 0 0 Control_L\n"
  ;

// Callback for the image plane widget interaction
class vtkIPWCallback : public vtkCommand
{
public:
  static vtkIPWCallback *New()
  { return new vtkIPWCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkImagePlaneWidget *planeWidget = reinterpret_cast<vtkImagePlaneWidget*>(caller);
    if(planeWidget->GetPlaneOrientation() == 3)
    {
      Spline->SetProjectionPosition(0);
    }
    else
    {
      Spline->SetProjectionPosition(planeWidget->GetSlicePosition());
    }
    Spline->GetPolyData(Poly);
  }
  vtkIPWCallback():Spline(0),Poly(0){};
  vtkSplineWidget* Spline;
  vtkPolyData* Poly;
};

// Callback for the spline widget interaction
class vtkSWCallback : public vtkCommand
{
public:
  static vtkSWCallback *New()
  { return new vtkSWCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkSplineWidget *spline = reinterpret_cast<vtkSplineWidget*>(caller);
    spline->GetPolyData(Poly);
  }
  vtkSWCallback():Poly(0){};
  vtkPolyData* Poly;
};

int TestSplineWidget( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions( 64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange( 1, 93);
  v16->SetDataSpacing( 3.2, 3.2, 1.5);
  v16->SetFilePrefix( fname);
  v16->SetDataMask( 0x7fff);
  v16->Update();

  delete[] fname;

  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderer> ren2 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer( ren1);
  renWin->AddRenderer( ren2);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin);

  vtkSmartPointer<vtkOutlineFilter> outline =
    vtkSmartPointer<vtkOutlineFilter>::New();
  outline->SetInputConnection(v16->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  vtkSmartPointer<vtkImagePlaneWidget> ipw =
    vtkSmartPointer<vtkImagePlaneWidget>::New();
  ipw->DisplayTextOn();
  ipw->TextureInterpolateOff();
  ipw->UserControlledLookupTableOff();
  ipw->SetInputConnection(v16->GetOutputPort());
  ipw->KeyPressActivationOn();
  ipw->SetKeyPressActivationValue('x');
  ipw->SetResliceInterpolateToNearestNeighbour();
  ipw->SetInteractor(iren);
  ipw->SetPlaneOrientationToXAxes();
  ipw->SetSliceIndex(32);
  ipw->GetPlaneProperty()->SetColor(1,0,0);

  vtkSmartPointer<vtkSplineWidget> spline =
    vtkSmartPointer<vtkSplineWidget>::New();
  spline->SetInteractor( iren);
  spline->SetInputConnection(v16->GetOutputPort());
  spline->SetPriority(1.0);
  spline->KeyPressActivationOff();
  spline->PlaceWidget();
  spline->ProjectToPlaneOn();
  spline->SetProjectionNormal(0);
  spline->SetProjectionPosition(102.4);  //initial plane oriented position
  spline->SetProjectionNormal(3); //allow arbitrary oblique orientations
  spline->SetPlaneSource(
    static_cast<vtkPlaneSource*>(ipw->GetPolyDataAlgorithm()));


  // Specify the type of spline (change from default vtkCardinalSpline)
  vtkSmartPointer<vtkKochanekSpline> xspline =
    vtkSmartPointer<vtkKochanekSpline>::New();
  vtkSmartPointer<vtkKochanekSpline> yspline =
    vtkSmartPointer<vtkKochanekSpline>::New();
  vtkSmartPointer<vtkKochanekSpline> zspline =
    vtkSmartPointer<vtkKochanekSpline>::New();

  vtkParametricSpline* para = spline->GetParametricSpline();

  para->SetXSpline(xspline);
  para->SetYSpline(yspline);
  para->SetZSpline(zspline);

  vtkSmartPointer<vtkPolyData> poly =
    vtkSmartPointer<vtkPolyData>::New();
  spline->GetPolyData(poly);

  vtkSmartPointer<vtkProbeFilter> probe =
    vtkSmartPointer<vtkProbeFilter>::New();
  probe->SetInputData(poly);
  probe->SetSourceConnection(v16->GetOutputPort());

  vtkSmartPointer<vtkIPWCallback> ipwcb =
    vtkSmartPointer<vtkIPWCallback>::New();
  ipwcb->Spline = spline;
  ipwcb->Poly = poly;

  ipw->AddObserver(vtkCommand::InteractionEvent,ipwcb);

  vtkSmartPointer<vtkSWCallback> swcb =
    vtkSmartPointer<vtkSWCallback>::New();
  swcb->Poly = poly;

  spline->AddObserver(vtkCommand::InteractionEvent,swcb);

  vtkImageData* data = v16->GetOutput();
  double* range = data->GetPointData()->GetScalars()->GetRange();

  vtkSmartPointer<vtkXYPlotActor> profile =
    vtkSmartPointer<vtkXYPlotActor>::New();
  profile->AddDataSetInputConnection(probe->GetOutputPort());
  profile->GetPositionCoordinate()->SetValue( 0.05, 0.05, 0);
  profile->GetPosition2Coordinate()->SetValue( 0.95, 0.95, 0);
  profile->SetXValuesToNormalizedArcLength();
  profile->SetNumberOfXLabels( 6);
  profile->SetTitle( "Profile Data ");
  profile->SetXTitle( "s");
  profile->SetYTitle( "I(s)");
  profile->SetXRange( 0, 1);
  profile->SetYRange( range[0], range[1]);
  profile->GetProperty()->SetColor( 0, 0, 0);
  profile->GetProperty()->SetLineWidth( 2);
  profile->SetLabelFormat("%g");
  vtkTextProperty* tprop = profile->GetTitleTextProperty();
  tprop->SetColor(0.02,0.06,0.62);
  tprop->SetFontFamilyToArial();
  profile->SetAxisTitleTextProperty(tprop);
  profile->SetAxisLabelTextProperty(tprop);
  profile->SetTitleTextProperty(tprop);

  ren1->SetBackground( 0.1, 0.2, 0.4);
  ren1->SetViewport( 0, 0, 0.5, 1);
  ren1->AddActor(outlineActor);

  ren2->SetBackground( 1, 1, 1);
  ren2->SetViewport( 0.5, 0, 1, 1);
  ren2->AddActor2D( profile);

  renWin->SetSize( 600, 300);

  ipw->On();
  ipw->SetInteraction(0);
  ipw->SetInteraction(1);
  spline->On();
  spline->SetNumberOfHandles(4);
  spline->SetNumberOfHandles(5);
  spline->SetResolution(399);

  // Set up an interesting viewpoint
  vtkCamera* camera = ren1->GetActiveCamera();
  camera->Elevation(110);
  camera->SetViewUp(0, 0, -1);
  camera->Azimuth(45);
  camera->SetFocalPoint(100.8,100.8,69);
  camera->SetPosition(560.949, 560.949, -167.853);
  ren1->ResetCameraClippingRange();

  // Position the actors
//  renWin->Render();
//  iren->SetEventPosition(200,200);
//  iren->SetKeyCode('r');
//  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
//  ren1->ResetCameraClippingRange();
//  renWin->Render();
//  iren->SetKeyCode('t');
//  iren->InvokeEvent(vtkCommand::CharEvent,NULL);

  // Playback recorded events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TSWeventLog);

  // Test On Off mechanism
  ipw->SetEnabled(0);
  spline->EnabledOff();
  ipw->SetEnabled(1);
  spline->EnabledOn();

  // Test Set Get handle positions
  double pos[3];
  int i;
  for(i=0;i<spline->GetNumberOfHandles();i++)
  {
    spline->GetHandlePosition(i,pos);
    spline->SetHandlePosition(i,pos);
  }

  // Test Closed On Off
  spline->ClosedOn();
  spline->ClosedOff();

  // Render the image
  iren->Initialize();
  renWin->Render();
  //ren1->ResetCamera();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  // Clean up
  recorder->Off();

  return EXIT_SUCCESS;
}
