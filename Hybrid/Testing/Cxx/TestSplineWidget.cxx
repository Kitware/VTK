#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkImagePlaneWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkOutlineFilter.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSplineWidget.h"
#include "vtkTextProperty.h"
#include "vtkVolume16Reader.h"
#include "vtkXYPlotActor.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

char TSWeventLog[] =
"# StreamVersion 1\n"
"CharEvent 141 139 0 0 98 1 i\n"
"KeyReleaseEvent 141 139 0 0 98 1 i\n"
"RightButtonPressEvent 141 139 0 0 0 0 i\n"
"MouseMoveEvent 141 138 0 0 0 0 i\n"
"MouseMoveEvent 141 134 0 0 0 0 i\n"
"MouseMoveEvent 141 130 0 0 0 0 i\n"
"MouseMoveEvent 141 126 0 0 0 0 i\n"
"MouseMoveEvent 141 124 0 0 0 0 i\n"
"RightButtonReleaseEvent 141 124 0 0 0 0 i\n"
"MouseMoveEvent 132 127 0 0 0 0 i\n"
"LeftButtonPressEvent 132 127 0 0 0 0 i\n"
"MouseMoveEvent 132 130 0 0 0 0 i\n"
"MouseMoveEvent 132 135 0 0 0 0 i\n"
"MouseMoveEvent 131 140 0 0 0 0 i\n"
"MouseMoveEvent 131 145 0 0 0 0 i\n"
"MouseMoveEvent 131 149 0 0 0 0 i\n"
"MouseMoveEvent 131 155 0 0 0 0 i\n"
"MouseMoveEvent 131 158 0 0 0 0 i\n"
"MouseMoveEvent 130 160 0 0 0 0 i\n"
"MouseMoveEvent 130 165 0 0 0 0 i\n"
"MouseMoveEvent 130 168 0 0 0 0 i\n"
"MouseMoveEvent 129 170 0 0 0 0 i\n"
"MouseMoveEvent 129 175 0 0 0 0 i\n"
"MouseMoveEvent 128 181 0 0 0 0 i\n"
"MouseMoveEvent 128 185 0 0 0 0 i\n"
"MouseMoveEvent 127 189 0 0 0 0 i\n"
"LeftButtonReleaseEvent 127 189 0 0 0 0 i\n"
"MouseMoveEvent 131 160 0 0 0 0 i\n"
"MiddleButtonPressEvent 131 160 0 0 0 0 i\n"
"MouseMoveEvent 130 160 0 0 0 0 i\n"
"MouseMoveEvent 129 159 0 0 0 0 i\n"
"MouseMoveEvent 127 158 0 0 0 0 i\n"
"MouseMoveEvent 126 157 0 0 0 0 i\n"
"MouseMoveEvent 125 156 0 0 0 0 i\n"
"MouseMoveEvent 124 155 0 0 0 0 i\n"
"MouseMoveEvent 123 154 0 0 0 0 i\n"
"MouseMoveEvent 122 153 0 0 0 0 i\n"
"MouseMoveEvent 118 152 0 0 0 0 i\n"
"MouseMoveEvent 117 153 0 0 0 0 i\n"
"MouseMoveEvent 116 158 0 0 0 0 i\n"
"MouseMoveEvent 115 158 0 0 0 0 i\n"
"MouseMoveEvent 114 163 0 0 0 0 i\n"
"MiddleButtonReleaseEvent 114 163 0 0 0 0 i\n"
"MouseMoveEvent 117 149 0 0 0 0 i\n"
"KeyPressEvent 117 149 -128 0 0 1 Control_L\n"
"LeftButtonPressEvent 117 149 8 0 0 0 Control_L\n"
"MouseMoveEvent 118 149 8 0 0 0 Control_L\n"
"MouseMoveEvent 118 148 8 0 0 0 Control_L\n"
"MouseMoveEvent 119 148 8 0 0 0 Control_L\n"
"MouseMoveEvent 120 148 8 0 0 0 Control_L\n"
"LeftButtonReleaseEvent 120 148 8 0 0 0 Control_L\n"
"MiddleButtonPressEvent 120 148 8 0 0 0 Control_L\n"
"MouseMoveEvent 122 147 8 0 0 0 Control_L\n"
"MouseMoveEvent 124 147 8 0 0 0 Control_L\n"
"MouseMoveEvent 125 146 8 0 0 0 Control_L\n"
"MouseMoveEvent 127 146 8 0 0 0 Control_L\n"
"MouseMoveEvent 128 145 8 0 0 0 Control_L\n"
"MouseMoveEvent 130 145 8 0 0 0 Control_L\n"
"MouseMoveEvent 131 144 8 0 0 0 Control_L\n"
"MouseMoveEvent 133 144 8 0 0 0 Control_L\n"
"MouseMoveEvent 135 143 8 0 0 0 Control_L\n"
"MouseMoveEvent 138 143 8 0 0 0 Control_L\n"
"MouseMoveEvent 140 142 8 0 0 0 Control_L\n"
"MouseMoveEvent 141 142 8 0 0 0 Control_L\n"
"MouseMoveEvent 142 141 8 0 0 0 Control_L\n"
"MouseMoveEvent 145 141 8 0 0 0 Control_L\n"
"MouseMoveEvent 148 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 150 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 153 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 156 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 158 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 160 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 163 140 8 0 0 0 Control_L\n"
"MouseMoveEvent 165 140 8 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 165 140 8 0 0 0 Control_L\n"
"KeyReleaseEvent 165 140 0 0 0 1 Control_L\n"
"MiddleButtonPressEvent 165 140 0 0 0 0 Control_L\n"
"MouseMoveEvent 170 139 0 0 0 0 Control_L\n"
"MouseMoveEvent 174 137 0 0 0 0 Control_L\n"
"MouseMoveEvent 177 136 0 0 0 0 Control_L\n"
"MouseMoveEvent 180 134 0 0 0 0 Control_L\n"
"MouseMoveEvent 184 131 0 0 0 0 Control_L\n"
"MouseMoveEvent 187 129 0 0 0 0 Control_L\n"
"MouseMoveEvent 190 127 0 0 0 0 Control_L\n"
"MouseMoveEvent 193 126 0 0 0 0 Control_L\n"
"MouseMoveEvent 196 125 0 0 0 0 Control_L\n"
"MouseMoveEvent 200 123 0 0 0 0 Control_L\n"
"MouseMoveEvent 205 121 0 0 0 0 Control_L\n"
"MouseMoveEvent 206 120 0 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 206 120 0 0 0 0 Control_L\n"
"MouseMoveEvent 223 115 0 0 0 0 Control_L\n"
"MiddleButtonPressEvent 223 115 0 0 0 0 Control_L\n"
"MouseMoveEvent 222 114 0 0 0 0 Control_L\n"
"MouseMoveEvent 221 113 0 0 0 0 Control_L\n"
"MouseMoveEvent 220 112 0 0 0 0 Control_L\n"
"MouseMoveEvent 219 111 0 0 0 0 Control_L\n"
"MouseMoveEvent 218 110 0 0 0 0 Control_L\n"
"MouseMoveEvent 217 109 0 0 0 0 Control_L\n"
"MouseMoveEvent 215 107 0 0 0 0 Control_L\n"
"MouseMoveEvent 214 106 0 0 0 0 Control_L\n"
"MouseMoveEvent 212 105 0 0 0 0 Control_L\n"
"MouseMoveEvent 211 104 0 0 0 0 Control_L\n"
"MouseMoveEvent 210 103 0 0 0 0 Control_L\n"
"MouseMoveEvent 209 103 0 0 0 0 Control_L\n"
"MouseMoveEvent 207 102 0 0 0 0 Control_L\n"
"MouseMoveEvent 206 101 0 0 0 0 Control_L\n"
"MouseMoveEvent 204 101 0 0 0 0 Control_L\n"
"MouseMoveEvent 203 100 0 0 0 0 Control_L\n"
"MouseMoveEvent 201 100 0 0 0 0 Control_L\n"
"MouseMoveEvent 198 99 0 0 0 0 Control_L\n"
"MouseMoveEvent 196 99 0 0 0 0 Control_L\n"
"MouseMoveEvent 195 98 0 0 0 0 Control_L\n"
"MouseMoveEvent 193 98 0 0 0 0 Control_L\n"
"MouseMoveEvent 191 97 0 0 0 0 Control_L\n"
"MouseMoveEvent 189 97 0 0 0 0 Control_L\n"
"MouseMoveEvent 187 97 0 0 0 0 Control_L\n"
"MouseMoveEvent 185 96 0 0 0 0 Control_L\n"
"MouseMoveEvent 183 96 0 0 0 0 Control_L\n"
"MouseMoveEvent 181 96 0 0 0 0 Control_L\n"
"MouseMoveEvent 179 96 0 0 0 0 Control_L\n"
"MouseMoveEvent 177 96 0 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 177 96 0 0 0 0 Control_L\n"
"MouseMoveEvent 249 100 0 0 0 0 Control_L\n"
"MiddleButtonPressEvent 249 100 0 0 0 0 Control_L\n"
"MouseMoveEvent 249 102 0 0 0 0 Control_L\n"
"MouseMoveEvent 249 106 0 0 0 0 Control_L\n"
"MouseMoveEvent 249 110 0 0 0 0 Control_L\n"
"MouseMoveEvent 250 114 0 0 0 0 Control_L\n"
"MouseMoveEvent 250 118 0 0 0 0 Control_L\n"
"MouseMoveEvent 251 122 0 0 0 0 Control_L\n"
"MouseMoveEvent 251 126 0 0 0 0 Control_L\n"
"MouseMoveEvent 251 130 0 0 0 0 Control_L\n"
"MouseMoveEvent 252 134 0 0 0 0 Control_L\n"
"MiddleButtonReleaseEvent 252 134 0 0 0 0 Control_L\n"
"KeyPressEvent 251 124 0 0 98 1 i\n";


// Callback for the image plane widget interaction
class vtkIPWCallback : public vtkCommand
{
public:
  static vtkIPWCallback *New()
    { return new vtkIPWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
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
  virtual void Execute(vtkObject *caller, unsigned long, void*)
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

  vtkVolume16Reader* v16 =  vtkVolume16Reader::New();
    v16->SetDataDimensions( 64, 64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetImageRange( 1, 93);
    v16->SetDataSpacing( 3.2, 3.2, 1.5);
    v16->SetFilePrefix( fname);
    v16->SetDataMask( 0x7fff);
    v16->Update();

  delete[] fname;

  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderer* ren2 = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
    renWin->AddRenderer( ren1);
    renWin->AddRenderer( ren2);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow( renWin);

  vtkOutlineFilter* outline = vtkOutlineFilter::New();
    outline->SetInput(v16->GetOutput());

  vtkPolyDataMapper* outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  vtkActor* outlineActor =  vtkActor::New();
    outlineActor->SetMapper(outlineMapper);

  vtkImagePlaneWidget* ipw = vtkImagePlaneWidget::New();
    ipw->DisplayTextOn();
    ipw->TextureInterpolateOff();
    ipw->SetInput(v16->GetOutput());
    ipw->SetKeyPressActivationValue('x');
    ipw->SetResliceInterpolateToNearestNeighbour();
    ipw->SetInteractor(iren);
    ipw->SetPlaneOrientationToXAxes();
    ipw->SetSliceIndex(32);
    ipw->GetPlaneProperty()->SetColor(1,0,0);

  vtkSplineWidget* spline = vtkSplineWidget::New();
    spline->SetInteractor( iren);
    spline->SetInput(v16->GetOutput());
    spline->SetPriority(1.0);
    spline->PlaceWidget();
    spline->ProjectToPlaneOn();
    spline->SetProjectionNormal(0);
    spline->SetProjectionPosition(102.4);  //initial plane oriented position
    spline->SetProjectionNormal(3); //allow arbitrary oblique orientations
    spline->SetPlaneSource((vtkPlaneSource*)ipw->GetPolyDataSource());

  vtkPolyData* poly = vtkPolyData::New();
    spline->GetPolyData(poly);

  vtkProbeFilter* probe = vtkProbeFilter::New();
    probe->SetInput(poly);
    probe->SetSource(v16->GetOutput());

  vtkIPWCallback* ipwcb = vtkIPWCallback::New();
    ipwcb->Spline = spline;
    ipwcb->Poly = poly;

  ipw->AddObserver(vtkCommand::InteractionEvent,ipwcb);

  vtkSWCallback* swcb = vtkSWCallback::New();
    swcb->Poly = poly;

  spline->AddObserver(vtkCommand::InteractionEvent,swcb);

  vtkImageData* data = v16->GetOutput();
  float* range = data->GetPointData()->GetScalars()->GetRange();

  vtkXYPlotActor* profile = vtkXYPlotActor::New();
    profile->AddInput(probe->GetOutput());
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
  ren1->ResetCameraClippingRange();

  // Position the actors
  renWin->Render();
  iren->SetEventPosition(200,200);
  iren->SetKeyCode('r');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  ren1->ResetCameraClippingRange();
  renWin->Render();
  iren->SetKeyCode('t');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);

  // Playback recorded events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TSWeventLog);

  // Render the image
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  recorder->Off();
  recorder->Delete();

  outlineActor->Delete();
  outlineMapper->Delete();
  outline->Delete();
  renWin->Delete();
  ren1->Delete();
  ren2->Delete();
  iren->Delete();

  ipw->RemoveObserver(ipwcb);
  ipw->Delete();
  ipwcb->Delete();
  spline->RemoveObserver(swcb);
  spline->Delete();
  swcb->Delete();
  poly->Delete();
  probe->Delete();
  profile->Delete();
  v16->Delete();

  return !retVal;
}


