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
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkXYPlotActor.h"

#include "vtkTestUtilities.h"

int TestXYPlotActor( int, char *[] )
{
  // Create container for points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  // Create containers for data
  unsigned int nPlots = 3;
  vtkStdString names[] =
    {
      "sqrt(x)",
      "sqrt(x)*sin(x/5)",
      "sqrt(x)*cos(x/10)",
    };
  vtkSmartPointer<vtkDoubleArray>* data = new vtkSmartPointer<vtkDoubleArray>[nPlots];
  for ( unsigned int i = 0; i < nPlots; ++ i )
    {
    data[i] = vtkSmartPointer<vtkDoubleArray>::New();
    data[i]->SetNumberOfComponents( 1 );
    data[i]->SetName( names[i].c_str() );
    }

  // Fill in points and data
  unsigned int nSteps = 10;
  unsigned int stepSize = 50;
  unsigned int nVals = nSteps * stepSize;
  for ( unsigned int i = 0; i < nVals; ++ i )
    {
    double val0 = sqrt( i );
    data[0]->InsertNextValue( val0 );
    double val1 = val0 * sin( .2 * i );
    data[1]->InsertNextValue( val1 );
    double val2 = val0  * cos( .1 * i );
    data[2]->InsertNextValue( val2 );
    points->InsertNextPoint( i, 0., 0. );
    }

  // Determine extrema
  double* rangeCurr = data[0]->GetRange();
  double range[2];
  range[0] = rangeCurr[0];
  range[1] = rangeCurr[1];
  for ( unsigned int i = 1; i < nPlots; ++ i )
    {
    rangeCurr = data[i]->GetRange();
    range[0] = rangeCurr[0] < range[0] ? rangeCurr[0] : range[0];
    range[1] = rangeCurr[1] > range[1] ? rangeCurr[1] : range[1];
    }

  // Create data sets with created points and data
  vtkSmartPointer<vtkPolyData>* polydata = new vtkSmartPointer<vtkPolyData>[nPlots];
  for ( unsigned int i = 0; i < nPlots; ++ i )
    {
    polydata[i] = vtkSmartPointer<vtkPolyData>::New();
    polydata[i]->SetPoints( points );
    polydata[i]->GetPointData()->SetScalars( data[i] );
    }

  // Set XY plot actor
  double colors[] =
  {
    .54, .21, .06,  // burnt sienna
    1., .38, .01,   // cadmium orange
    .498, 1., 0.,   // chartreuse
    0., .78, 0.55,  // turquoise blue
  };
  vtkSmartPointer<vtkXYPlotActor> xyPlot = vtkSmartPointer<vtkXYPlotActor>::New();
  for ( unsigned int i = 0; i < nPlots; ++ i )
    {
    xyPlot->AddDataSetInput( polydata[i] );
    xyPlot->SetPlotColor( i, colors[3 * i], colors[3 * i + 1], colors[3 * i + 2] );
    }
  xyPlot->GetPositionCoordinate()->SetValue( .1, .1, .0 );
  xyPlot->GetPosition2Coordinate()->SetValue( .9, .9, .0 );
  xyPlot->SetLineWidth( 2 );

  // Title settings
  xyPlot->SetTitleFontFamily( VTK_ARIAL );
  xyPlot->SetTitleColor( .9, .06, .02 );
  xyPlot->SetTitle( "XY Plot Actor Test");

  // Axes settings
  xyPlot->SetAxisTitleFontFamily( VTK_TIMES );
  xyPlot->SetAxisTitleColor( 0., 0., 1. );
  xyPlot->SetYTitlePositionToTop();
  xyPlot->SetXTitle( "x");
  xyPlot->SetYTitle( "f(x)");
  xyPlot->SetXValuesToIndex();
  xyPlot->SetXRange( 0, nVals );
  xyPlot->SetYRange( range[0], range[1] );
  xyPlot->SetXAxisColor( 0., 0., 0. );
  xyPlot->SetYAxisColor( 0., 0., 0. );

  // Label settings
  xyPlot->SetAxisLabelFontFamily( VTK_COURIER );
  xyPlot->SetAxisLabelColor( 0., 0., .9 );
  xyPlot->SetLabelFormat("%g");
  xyPlot->SetAdjustXLabels( 0 );
  xyPlot->SetNumberOfXLabels( nSteps + 1 );
  xyPlot->SetAdjustYLabels( 0 );
  xyPlot->SetNumberOfYLabels( 3 );


  // Set up rendering contraption
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  ren1->SetBackground( .99, 1., .94); // titanium white
  ren1->AddActor( xyPlot );
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples( 0 );
  renWin->AddRenderer( ren1 );
  renWin->SetSize( 600, 300 );
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );

  // Set up an interesting viewpoint
  vtkCamera* camera = ren1->GetActiveCamera();
  camera->Elevation( 110 );
  camera->SetViewUp( 0, 0, -1 );
  camera->Azimuth( 45 );
  camera->SetFocalPoint( 100.8, 100.8, 69. );
  camera->SetPosition( 560.949, 560.949, -167.853 );
  ren1->ResetCameraClippingRange();

  // Render the image
  iren->Initialize();
  renWin->Render();

  iren->Start();

  // Clean up
  delete [] polydata;
  delete [] data;

  return EXIT_SUCCESS;
}
