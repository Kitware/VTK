/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDijkstraGraphGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDEMReader.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkWarpScalar.h"
#include "vtkPolyDataNormals.h"
#include "vtkLODActor.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyDataCollection.h"
#include "vtkTriangleFilter.h"
#include "vtkImageResample.h"
#include "vtkInteractorEventRecorder.h"

#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkPolygonalSurfacePointPlacer.h"
#include "vtkPolygonalSurfaceContourLineInterpolator.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

char TestDijkstraGraphGeodesicPathLog[] =
"# StreamVersion 1\n"
"EnterEvent 260 15 0 0 0 0 0 i\n"
"MouseMoveEvent 186 15 0 0 0 0 0 i\n"
"MouseMoveEvent 124 15 0 0 0 0 0 i\n"
"MouseMoveEvent 74 11 0 0 0 0 0 i\n"
"MouseMoveEvent 30 7 0 0 0 0 0 i\n"
"LeaveEvent -5 3 0 0 0 0 0 i\n"
"EnterEvent 7 5 0 0 0 0 0 i\n"
"MouseMoveEvent 17 15 0 0 0 0 0 i\n"
"MouseMoveEvent 29 29 0 0 0 0 0 i\n"
"MouseMoveEvent 37 41 0 0 0 0 0 i\n"
"MouseMoveEvent 45 59 0 0 0 0 0 i\n"
"MouseMoveEvent 55 77 0 0 0 0 0 i\n"
"MouseMoveEvent 63 93 0 0 0 0 0 i\n"
"MouseMoveEvent 71 111 0 0 0 0 0 i\n"
"MouseMoveEvent 81 127 0 0 0 0 0 i\n"
"MouseMoveEvent 87 143 0 0 0 0 0 i\n"
"MouseMoveEvent 95 157 0 0 0 0 0 i\n"
"MouseMoveEvent 97 169 0 0 0 0 0 i\n"
"MouseMoveEvent 99 175 0 0 0 0 0 i\n"
"MouseMoveEvent 99 183 0 0 0 0 0 i\n"
"MouseMoveEvent 99 184 0 0 0 0 0 i\n"
"MouseMoveEvent 98 184 0 0 0 0 0 i\n"
"MouseMoveEvent 90 184 0 0 0 0 0 i\n"
"MouseMoveEvent 87 184 0 0 0 0 0 i\n"
"MouseMoveEvent 79 182 0 0 0 0 0 i\n"
"MouseMoveEvent 73 180 0 0 0 0 0 i\n"
"MouseMoveEvent 72 179 0 0 0 0 0 i\n"
"MouseMoveEvent 71 179 0 0 0 0 0 i\n"
"MouseMoveEvent 70 179 0 0 0 0 0 i\n"
"MouseMoveEvent 69 180 0 0 0 0 0 i\n"
"MouseMoveEvent 68 181 0 0 0 0 0 i\n"
"MouseMoveEvent 67 182 0 0 0 0 0 i\n"
"MouseMoveEvent 67 184 0 0 0 0 0 i\n"
"MouseMoveEvent 66 185 0 0 0 0 0 i\n"
"MouseMoveEvent 62 189 0 0 0 0 0 i\n"
"MouseMoveEvent 61 191 0 0 0 0 0 i\n"
"MouseMoveEvent 60 192 0 0 0 0 0 i\n"
"MouseMoveEvent 58 193 0 0 0 0 0 i\n"
"MouseMoveEvent 57 194 0 0 0 0 0 i\n"
"MouseMoveEvent 56 195 0 0 0 0 0 i\n"
"MouseMoveEvent 54 196 0 0 0 0 0 i\n"
"MouseMoveEvent 53 197 0 0 0 0 0 i\n"
"MouseMoveEvent 47 199 0 0 0 0 0 i\n"
"MouseMoveEvent 46 200 0 0 0 0 0 i\n"
"MouseMoveEvent 45 201 0 0 0 0 0 i\n"
"MouseMoveEvent 43 201 0 0 0 0 0 i\n"
"MouseMoveEvent 42 202 0 0 0 0 0 i\n"
"MouseMoveEvent 41 203 0 0 0 0 0 i\n"
"MouseMoveEvent 40 203 0 0 0 0 0 i\n"
"MouseMoveEvent 39 204 0 0 0 0 0 i\n"
"MouseMoveEvent 38 204 0 0 0 0 0 i\n"
"MouseMoveEvent 37 204 0 0 0 0 0 i\n"
"LeftButtonPressEvent 37 204 0 0 0 0 0 i\n"
"RenderEvent 37 204 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 37 204 0 0 0 0 0 i\n"
"MouseMoveEvent 67 198 0 0 0 0 0 i\n"
"MouseMoveEvent 68 198 0 0 0 0 0 i\n"
"MouseMoveEvent 69 197 0 0 0 0 0 i\n"
"MouseMoveEvent 70 197 0 0 0 0 0 i\n"
"MouseMoveEvent 71 196 0 0 0 0 0 i\n"
"MouseMoveEvent 72 196 0 0 0 0 0 i\n"
"MouseMoveEvent 73 196 0 0 0 0 0 i\n"
"MouseMoveEvent 73 195 0 0 0 0 0 i\n"
"MouseMoveEvent 74 195 0 0 0 0 0 i\n"
"MouseMoveEvent 75 195 0 0 0 0 0 i\n"
"MouseMoveEvent 76 195 0 0 0 0 0 i\n"
"MouseMoveEvent 77 195 0 0 0 0 0 i\n"
"LeftButtonPressEvent 77 195 0 0 0 0 0 i\n"
"RenderEvent 77 195 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 77 195 0 0 0 0 0 i\n"
"MouseMoveEvent 105 159 0 0 0 0 0 i\n"
"LeftButtonPressEvent 105 159 0 0 0 0 0 i\n"
"RenderEvent 105 159 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 105 159 0 0 0 0 0 i\n"
"MouseMoveEvent 103 122 0 0 0 0 0 i\n"
"MouseMoveEvent 103 121 0 0 0 0 0 i\n"
"MouseMoveEvent 104 120 0 0 0 0 0 i\n"
"MouseMoveEvent 105 119 0 0 0 0 0 i\n"
"MouseMoveEvent 106 119 0 0 0 0 0 i\n"
"MouseMoveEvent 107 119 0 0 0 0 0 i\n"
"MouseMoveEvent 108 119 0 0 0 0 0 i\n"
"MouseMoveEvent 108 118 0 0 0 0 0 i\n"
"MouseMoveEvent 109 118 0 0 0 0 0 i\n"
"MouseMoveEvent 110 118 0 0 0 0 0 i\n"
"MouseMoveEvent 111 117 0 0 0 0 0 i\n"
"MouseMoveEvent 112 117 0 0 0 0 0 i\n"
"MouseMoveEvent 112 116 0 0 0 0 0 i\n"
"MouseMoveEvent 113 116 0 0 0 0 0 i\n"
"MouseMoveEvent 114 116 0 0 0 0 0 i\n"
"LeftButtonPressEvent 114 116 0 0 0 0 0 i\n"
"RenderEvent 114 116 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 114 116 0 0 0 0 0 i\n"
"KeyPressEvent 270 115 0 0 113 1 q i\n"
"CharEvent 270 115 0 0 113 1 q i\n"
"ExitEvent 270 115 0 0 113 1 q i\n"
;

int TestDijkstraGraphGeodesicPath(int argc, char*argv[])
{
  if (argc < 2)
    {
    cerr
      << "Demonstrates editing capabilities of a contour widget on polygonal \n"
      << "data. For consistency, this accepts a DEM data as input, (to compare\n"
      << "it with the TerrainPolylineEditor example. However, it converts the DEM\n"
      << "data to a polygonal data before feeding it to the contour widget.\n\n"
      << "Usage args: [height_offset]." 
      << endl;
    return EXIT_FAILURE;
    }

  // Read height field. 
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");
  
  // Read height field. 
  //
  vtkDEMReader *demReader = vtkDEMReader::New();
  demReader->SetFileName(fname);
  delete [] fname;

  vtkImageResample * resample = vtkImageResample::New();
  resample->SetInput(demReader->GetOutput());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0,1.0);
  resample->SetAxisMagnificationFactor(1,1.0);
  
  // Extract geometry
  vtkImageDataGeometryFilter *surface = vtkImageDataGeometryFilter::New();
  surface->SetInput(resample->GetOutput());
  resample->Delete();

  // The Dijkistra interpolator will not accept cells that aren't triangles
  vtkTriangleFilter *triangleFilter = vtkTriangleFilter::New();
  triangleFilter->SetInput( surface->GetOutput() );
  triangleFilter->Update();
  
  vtkWarpScalar *warp = vtkWarpScalar::New();
  warp->SetInput(triangleFilter->GetOutput());
  warp->SetScaleFactor(1);
  warp->UseNormalOn();
  warp->SetNormal(0, 0, 1);
  surface->Delete();
  warp->Update();
  triangleFilter->Delete();

  // Define a LUT mapping for the height field 

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);
  
  vtkPolyDataNormals *normals = vtkPolyDataNormals::New();

  bool   distanceOffsetSpecified = false;
  double distanceOffset = 0.0;
  for (int i = 0; i < argc-1; i++)
    {
    if (strcmp("-DistanceOffset", argv[i]) == 0)
      {
      distanceOffset = atof(argv[i+1]);
      distanceOffsetSpecified = true;
      }
    }

  if (distanceOffsetSpecified)
    {
    normals->SetInput(warp->GetPolyDataOutput());
    normals->SetFeatureAngle(60);
    normals->SplittingOff();

    // vtkPolygonalSurfacePointPlacer needs cell normals
    // vtkPolygonalSurfaceContourLineInterpolator needs vertex normals
    normals->ComputeCellNormalsOn(); 
    normals->ComputePointNormalsOn();
    normals->Update();
    }

  vtkPolyData *pd = (distanceOffsetSpecified) ? normals->GetOutput()
                                            : warp->GetPolyDataOutput();

  vtkPolyDataMapper *demMapper = vtkPolyDataMapper::New();
  demMapper->SetInput(pd);
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  lut->Delete();
  normals->Delete();
  warp->Delete();

  vtkActor *demActor = vtkActor::New();
  demActor->SetMapper(demMapper);
  demMapper->Delete();

  // Create the RenderWindow, Renderer and the DEM + path actors.
 
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  
  // Add the actors to the renderer, set the background and size
  
  ren1->AddActor(demActor);

  ren1->GetActiveCamera()->SetViewUp(0, 0, 1);
  ren1->GetActiveCamera()->SetPosition(-99900, -21354, 131801);
  ren1->GetActiveCamera()->SetFocalPoint(41461, 41461, 2815);
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Dolly(4.2);
  ren1->ResetCameraClippingRange();

  // Here comes the contour widget stuff.....

  vtkContourWidget *contourWidget = vtkContourWidget::New();
  contourWidget->SetInteractor(iren);
  vtkOrientedGlyphContourRepresentation *rep = 
      vtkOrientedGlyphContourRepresentation::SafeDownCast(
                        contourWidget->GetRepresentation());
  rep->GetLinesProperty()->SetColor(1, 0.2, 0);
  rep->GetLinesProperty()->SetLineWidth(3.0);

  vtkPolygonalSurfacePointPlacer * pointPlacer 
        = vtkPolygonalSurfacePointPlacer::New();
  pointPlacer->AddProp(demActor);
  pointPlacer->GetPolys()->AddItem( pd );
  rep->SetPointPlacer(pointPlacer);

  vtkPolygonalSurfaceContourLineInterpolator * interpolator =
    vtkPolygonalSurfaceContourLineInterpolator::New();
  interpolator->GetPolys()->AddItem( pd );
  rep->SetLineInterpolator(interpolator);
  interpolator->Delete();
  if (distanceOffsetSpecified)
    {
    pointPlacer->SetDistanceOffset( distanceOffset );
    interpolator->SetDistanceOffset( distanceOffset );
    }
  
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestDijkstraGraphGeodesicPathLog); 
  recorder->EnabledOn();

  renWin->Render();
  iren->Initialize();

  contourWidget->EnabledOn();

  recorder->Play();
    
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanups
  recorder->Delete();
  contourWidget->Delete();
  pointPlacer->Delete();
  demReader->Delete();
  demActor->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  
  return !retVal;
}

