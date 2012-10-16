#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkXYPlotActor.h"

#include "vtkTestUtilities.h"

int TestXYPlotActor( int, char *[] )
{
  // Create container for points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  // Create container for data
  vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
  data->SetNumberOfComponents( 1 );
  data->SetName( "Test Data" );

  // Fill in points and data
  unsigned int nVals = 500;
  for ( unsigned int i = 0; i < nVals; ++ i )
    {
    double val = sin( i ) * cos( 2 * i ) * sqrt( i );
    data->InsertNextValue( val );
    points->InsertNextPoint( i, 0., 0. );
    }
  double* range = data->GetRange();

  // Create data set with created points and data
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints( points );
  //  polydata->SetPolys(triangles);
  polydata->GetPointData()->SetScalars( data );

  // Set up rendering contraption
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples( 0 );
  renWin->AddRenderer( ren1 );
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );

  // Set XY plot actor
  vtkSmartPointer<vtkXYPlotActor> xyPlot = vtkSmartPointer<vtkXYPlotActor>::New();
  xyPlot->AddDataSetInput( polydata );
  xyPlot->GetPositionCoordinate()->SetValue( 0.05, 0.05, 0 );
  xyPlot->GetPosition2Coordinate()->SetValue( 0.95, 0.95, 0 );
  xyPlot->SetXValuesToNormalizedArcLength();
  xyPlot->SetNumberOfXLabels( 9 );
  xyPlot->SetNumberOfYLabels( 8 );
  xyPlot->SetTitle( "XY Plot Actor Test");
  xyPlot->SetYTitlePositionToTop();
  xyPlot->SetXTitle( "x");
  xyPlot->SetYTitle( "f(x)");
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
