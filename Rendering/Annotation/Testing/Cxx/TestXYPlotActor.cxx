/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXYPlotActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2012

#include "vtkActor.h"
#include "vtkDoubleArray.h"
#include "vtkLegendBoxActor.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkXYPlotActor.h"

#include "vtkTestUtilities.h"

int TestXYPlotActor( int argc, char* argv[] )
{
  // Create container for points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  // Create containers for data
  unsigned int nPlots = 4;
  vtkStdString names[] =
    {
      "sqrt(x)",
      "sqrt(x)sin(10ln(sqrt(x)))",
      "sqrt(x)cos(x/10)",
      "-sqrt(x)",
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
  unsigned int nVals = nSteps * stepSize + 1;
  for ( unsigned int i = 0; i < nVals; ++ i )
    {
    points->InsertNextPoint( i, 0., 0. );

    double val0 = sqrt( static_cast<double>( i ) );
    data[0]->InsertNextValue( val0 );
    double val1 = val0 * sin( 10*log( val0 ) );
    data[1]->InsertNextValue( val1 );
    double val2 = val0  * cos( 2. * val0 );
    data[2]->InsertNextValue( val2 );
    data[3]->InsertNextValue( -val0 );
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
    .24, .57, .25,  // cobalt green
    0., 0., 0.502,  // navy blue
  };
  vtkSmartPointer<vtkXYPlotActor> xyPlot = vtkSmartPointer<vtkXYPlotActor>::New();
  for ( unsigned int i = 0; i < nPlots; ++ i )
    {
    xyPlot->AddDataSetInput( polydata[i] );
    xyPlot->SetPlotColor( i, colors[3 * i], colors[3 * i + 1], colors[3 * i + 2] );
    }
  xyPlot->GetPositionCoordinate()->SetValue( .01, .01, .0 );
  xyPlot->GetPosition2Coordinate()->SetValue( .99, .99, .0 );
  xyPlot->SetLineWidth( 2 );
  xyPlot->SetBorder( 10  );

  // Title settings
  xyPlot->SetTitleItalic( 0 );
  xyPlot->SetTitleBold( 1 );
  xyPlot->SetTitleFontFamily( VTK_ARIAL );
  xyPlot->SetTitleColor( .9, .06, .02 );
  xyPlot->SetTitle( "XY Plot Actor Test");

  // Legend settings
  xyPlot->SetLegend( 1 );
  xyPlot->SetLegendPosition( .7, .6 );
  xyPlot->SetLegendPosition2( .25, .2 );
  xyPlot->SetLegendBorder( 1 );
  xyPlot->SetLegendBox( 0 );
  xyPlot->SetLegendUseBackground( 1 );
  xyPlot->SetLegendBackgroundColor( .86, .86, .86 );
  for ( unsigned int i = 0; i < nPlots; ++ i )
    {
    xyPlot->GetLegendActor()->SetEntryString( i, names[i] );
    }

  // Axes settings
  xyPlot->SetAxisTitleFontFamily( VTK_TIMES );
  xyPlot->SetAxisTitleColor( 0., 0., 1. );
  xyPlot->SetYTitlePositionToVCenter();
  xyPlot->SetXTitle( "x" );
  xyPlot->SetYTitle( "f(x)" );
  xyPlot->SetXValuesToIndex();
  xyPlot->SetXRange( 0, nVals - 1 );
  xyPlot->SetYRange( floor( range[0] ), ceil( range[1] ) );
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
  ren1->SetBackground( .99, 1., .94 ); // titanium white
  ren1->AddActor( xyPlot );
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples( 0 );
  renWin->AddRenderer( ren1 );
  renWin->SetSize( 600, 400 );
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  // Clean up
  delete [] polydata;
  delete [] data;

  return !retVal;
}
