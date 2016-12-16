/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistanceWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkContourWidget.h"
#include "vtkPolyPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkCutter.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkContourWidget.h"
#include "vtkTextProperty.h"
#include "vtkDEMReader.h"
#include "vtkXYPlotActor.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyLine.h"
#include "vtkPoints.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkLookupTable.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkWarpScalar.h"
#include "vtkTriangleFilter.h"
#include "vtkImageResample.h"
#include "vtkImageData.h"
#include "vtkCellArray.h"
#include "vtkLinearContourLineInterpolator.h"
#include "vtkXMLPolyDataWriter.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"


// --------------------------------------------------------------------------
// Callback for the widget interaction
class vtkTestPolyPlaneCallback : public vtkCommand
{
public:
  static vtkTestPolyPlaneCallback *New()
  {
    return new vtkTestPolyPlaneCallback;
  }

  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkContourWidget *widget = reinterpret_cast<vtkContourWidget*>(caller);
    vtkContourRepresentation *rep = vtkContourRepresentation::
                        SafeDownCast(widget->GetRepresentation());

    vtkPolyData *pd = rep->GetContourRepresentationAsPolyData();

    // If less than 2 points, we can't define a polyplane..

    if (pd->GetPoints()->GetNumberOfPoints() >= 2)
    {

      vtkPolyLine *polyline = vtkPolyLine::New();
      polyline->Initialize( pd->GetNumberOfPoints(),
                            pd->GetLines()->GetPointer()+1,
                            pd->GetPoints() );

      this->PolyPlane->SetPolyLine( polyline );
      polyline->Delete();

      this->Cutter->SetCutFunction(this->PolyPlane);
    }
  }


  vtkTestPolyPlaneCallback() : PolyPlane(0),Cutter(0) {};
  vtkPolyPlane * PolyPlane;
  vtkCutter    * Cutter;
};


// --------------------------------------------------------------------------
int TestPolyPlane( int argc, char *argv[] )
{
  // Read height field.

  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");

  vtkSmartPointer<vtkDEMReader> demReader =
    vtkSmartPointer<vtkDEMReader>::New();
  demReader->SetFileName(fname);

  delete [] fname;

  // Resample (left incase, we want to subsample, supersample)

  vtkSmartPointer<vtkImageResample>  resample =
    vtkSmartPointer<vtkImageResample>::New();
  resample->SetInputConnection(demReader->GetOutputPort());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0,0.25);
  resample->SetAxisMagnificationFactor(1,0.25);

  // Extract geometry

  vtkSmartPointer<vtkImageDataGeometryFilter> surface =
    vtkSmartPointer<vtkImageDataGeometryFilter>::New();
  surface->SetInputConnection(resample->GetOutputPort());

  // Convert to triangle mesh

  vtkSmartPointer<vtkTriangleFilter> triangleFilter =
    vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputConnection( surface->GetOutputPort() );
  triangleFilter->Update();

  // Warp

  vtkSmartPointer<vtkWarpScalar> warp =
    vtkSmartPointer<vtkWarpScalar>::New();
  warp->SetInputConnection(triangleFilter->GetOutputPort());
  warp->SetScaleFactor(1);
  warp->UseNormalOn();
  warp->SetNormal(0, 0, 1);

  //  Update the pipeline until now.

  warp->Update();

  // Define a LUT mapping for the height field

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  vtkSmartPointer<vtkLookupTable> lut =
    vtkSmartPointer<vtkLookupTable>::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);

  // Create Renderer, Render window and interactor

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

  // Render the height field

  vtkSmartPointer<vtkPolyDataMapper> demMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  demMapper->SetInputConnection(warp->GetOutputPort());
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> demActor =
    vtkSmartPointer<vtkActor>::New();
  demActor->SetMapper(demMapper);

  ren1->AddActor(demActor);


  // Create a contour widget on ren1

  vtkSmartPointer<vtkContourWidget> contourWidget =
    vtkSmartPointer<vtkContourWidget>::New();
  contourWidget->SetInteractor(iren);
  vtkOrientedGlyphContourRepresentation *rep =
    vtkOrientedGlyphContourRepresentation::SafeDownCast(
      contourWidget->GetRepresentation());
  rep->GetLinesProperty()->SetColor(1, 0.2, 0);
  rep->GetLinesProperty()->SetLineWidth(3.0);

  // Use no interpolation (default is bezier).

  vtkSmartPointer< vtkLinearContourLineInterpolator > lineInterpolator
    = vtkSmartPointer< vtkLinearContourLineInterpolator >::New();
  rep->SetLineInterpolator(lineInterpolator);


  // Create a polyplane to cut with

  vtkSmartPointer< vtkPolyPlane > polyPlane =
    vtkSmartPointer< vtkPolyPlane >::New();

  // Create a cutter

  vtkSmartPointer<vtkCutter> cutter =
    vtkSmartPointer<vtkCutter>::New();
  cutter->SetInputConnection(warp->GetOutputPort());

  // Callback to update the polyplane when the contour is updated

  vtkSmartPointer<vtkTestPolyPlaneCallback> cb =
    vtkSmartPointer<vtkTestPolyPlaneCallback>::New();
  cb->PolyPlane = polyPlane;
  cb->Cutter = cutter;


  vtkPolyData * data = warp->GetPolyDataOutput();
  double* range = data->GetPointData()->GetScalars()->GetRange();

  //  plot the height field

  vtkSmartPointer<vtkXYPlotActor> profile =
    vtkSmartPointer<vtkXYPlotActor>::New();
  profile->AddDataSetInputConnection(cutter->GetOutputPort());
  profile->GetPositionCoordinate()->SetValue( 0.05, 0.05, 0);
  profile->GetPosition2Coordinate()->SetValue( 0.95, 0.95, 0);
  profile->SetXValuesToArcLength();
  profile->SetNumberOfXLabels( 6 );
  profile->SetTitle( "Profile Data ");
  profile->SetXTitle( "Arc length");
  profile->SetYTitle( "Height");
  profile->SetYRange( range[0], range[1] );
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

  ren2->SetBackground( 1, 1, 1);
  ren2->SetViewport( 0.5, 0, 1, 1);

  renWin->SetSize( 800, 500);

  // Set up an interesting viewpoint

  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  vtkCamera* camera = ren1->GetActiveCamera();
  camera->SetViewUp(0.796081, -0.277969, 0.537576);
  camera->SetParallelScale(10726.6);
  camera->SetFocalPoint(562412, 5.11456e+06, 1955.44);
  camera->SetPosition(544402, 5.11984e+06, 31359.2);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  // Create some default points

  contourWidget->On();

  // First remove all nodes.
  rep->ClearAllNodes();
  rep->AddNodeAtWorldPosition(560846, 5.12018e+06, 2205.95);
  rep->AddNodeAtWorldPosition(562342, 5.11663e+06, 3630.72);
  rep->AddNodeAtWorldPosition(562421, 5.11321e+06, 3156.75);
  rep->AddNodeAtWorldPosition(565885, 5.11067e+06, 2885.73);
  contourWidget->SetWidgetState(vtkContourWidget::Manipulate);

  // Execute the cut
  cb->Execute(contourWidget,0,NULL);

  vtkXMLPolyDataWriter *pWriter = vtkXMLPolyDataWriter::New();
  pWriter->SetInputConnection(cutter->GetOutputPort());
  cutter->Update();
  pWriter->SetFileName("CutPolyPlane.vtp");
  pWriter->Write();
  pWriter->SetInputConnection(warp->GetOutputPort());
  pWriter->SetFileName("Dataset.vtp");
  pWriter->Write();
  pWriter->SetInputData(rep->GetContourRepresentationAsPolyData());
  pWriter->SetFileName("Contour.vtp");
  pWriter->Write();
  pWriter->Delete();

  // Observe and update profile when contour widget is interacted with
  contourWidget->AddObserver(vtkCommand::InteractionEvent,cb);

  // Render the image
  iren->Initialize();
  ren2->AddActor2D( profile);
  renWin->Render();
  ren1->ResetCamera();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
