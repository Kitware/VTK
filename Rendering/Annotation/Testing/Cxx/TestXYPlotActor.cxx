#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProbeFilter.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkVolume16Reader.h"
#include "vtkXYPlotActor.h"

#include "vtkTestUtilities.h"

int TestXYPlotActor( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/headsq/quarter" );

  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions( 64, 64 );
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange( 1, 93 );
  v16->SetDataSpacing( 3.2, 3.2, 1.5 );
  v16->SetFilePrefix( fname );
  v16->SetDataMask( 0x7fff );
  v16->Update();

  delete[] fname;

  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples( 0 );
  renWin->AddRenderer( ren1 );
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );

  vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();

  vtkSmartPointer<vtkProbeFilter> probe = vtkSmartPointer<vtkProbeFilter>::New();
  probe->SetInputConnection( line->GetOutputPort() );
  probe->SetSourceConnection( v16->GetOutputPort() );

  vtkImageData* data = v16->GetOutput();
  double* range = data->GetPointData()->GetScalars()->GetRange();

  vtkSmartPointer<vtkXYPlotActor> xyPlot = vtkSmartPointer<vtkXYPlotActor>::New();
  xyPlot->AddDataSetInputConnection( probe->GetOutputPort() );
  xyPlot->GetPositionCoordinate()->SetValue( 0.05, 0.05, 0 );
  xyPlot->GetPosition2Coordinate()->SetValue( 0.95, 0.95, 0 );
  xyPlot->SetXValuesToNormalizedArcLength();
  xyPlot->SetNumberOfXLabels( 6 );
  xyPlot->SetNumberOfYLabels( 8 );
  xyPlot->SetTitle( "XY Plot Actor Test");
  xyPlot->SetYTitlePositionToTop();
  xyPlot->SetXTitle( "x value");
  xyPlot->SetYTitle( "y value");
  xyPlot->SetXRange( 0, 1 );
  xyPlot->SetYRange( range[0], range[1] );
  xyPlot->GetProperty()->SetColor( 0, 0, 0 );
  xyPlot->GetProperty()->SetLineWidth( 2 );
  xyPlot->SetLabelFormat("%g");
  vtkTextProperty* tprop = xyPlot->GetTitleTextProperty();
  tprop->SetColor( 0.02,0.06,0.62 );
  tprop->SetFontFamilyToArial();
  xyPlot->SetAxisTitleTextProperty( tprop );
  xyPlot->SetAxisLabelTextProperty( tprop );
  xyPlot->SetTitleTextProperty( tprop );

  ren1->SetBackground( .8, .8, .8 );
  ren1->AddActor( xyPlot );

  renWin->SetSize( 600, 300 );

  // Set up an interesting viewpoint
  vtkCamera* camera = ren1->GetActiveCamera();
  camera->Elevation( 110 );
  camera->SetViewUp( 0, 0, -1 );
  camera->Azimuth( 45 );
  camera->SetFocalPoint( 100.8,100.8,69 );
  camera->SetPosition( 560.949, 560.949, -167.853 );
  ren1->ResetCameraClippingRange();

  // Render the image
  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
