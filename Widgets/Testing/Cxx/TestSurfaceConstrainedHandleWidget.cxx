/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfaceConstrainedHandleWidget.cxx

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

#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolygonalSurfacePointPlacer.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestSurfaceConstrainedHandleWidget(int argc, char*argv[])
{
  if (argc < 2)
    {
    cerr
      << "Demonstrates interaction of a handle, so that it is constrained \n"
      << "to lie on a polygonal surface.\n\n"
      << "Usage args: [-DistanceOffset height_offset]."
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
      }
    }

  if (distanceOffsetSpecified)
    {
    normals->SetInput(warp->GetPolyDataOutput());
    normals->SetFeatureAngle(60);
    normals->SplittingOff();

    // vtkPolygonalSurfacePointPlacer needs cell normals
    normals->ComputeCellNormalsOn();
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
  ren1->ResetCameraClippingRange();

  // Here comes the surface constrained handle widget stuff.....

  vtkHandleWidget *widget = vtkHandleWidget::New();
  widget->SetInteractor(iren);
  vtkPointHandleRepresentation3D *rep =
      vtkPointHandleRepresentation3D::SafeDownCast(
                        widget->GetRepresentation());

  vtkPolygonalSurfacePointPlacer * pointPlacer
        = vtkPolygonalSurfacePointPlacer::New();
  pointPlacer->AddProp(demActor);
  pointPlacer->GetPolys()->AddItem( pd );
  rep->SetPointPlacer(pointPlacer);

  // Let the surface constrained point-placer be the sole constraint dictating 
  // the placement of handles. Lets not over-constrain it allowing axis 
  // constrained interactions.
  widget->EnableAxisConstraintOff();

  // Set some defaults on the handle widget
  double d[3] = {562532, 5.11396e+06, 2618.62};
  rep->SetWorldPosition( d );
  rep->GetProperty()->SetColor( 1.0, 0.0, 0.0 );
  rep->GetProperty()->SetLineWidth(1.0);
  rep->GetSelectedProperty()->SetColor( 0.2, 0.0, 1.0 );

  if (distanceOffsetSpecified)
    {
    pointPlacer->SetDistanceOffset( distanceOffset );
    }

  renWin->Render();
  iren->Initialize();
  widget->EnabledOn();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanups
  widget->Delete();
  pointPlacer->Delete();
  demReader->Delete();
  demActor->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();

  return !retVal;
}

