/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygonalHandleRepresentations.cxx

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
#include "vtkPolygonalHandleRepresentation3D.h"
#include "vtkOrientedPolygonalHandleRepresentation3D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolygonalSurfacePointPlacer.h"
#include "vtkSphereSource.h"
#include "vtkGlyphSource2D.h"
#include "vtkTestUtilities.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


vtkSmartPointer< vtkHandleWidget >
CreateWidget( vtkRenderWindowInteractor * iren,
              int                         shape,
              double                      x,
              double                      y,
              double                      z,
              bool                        cameraFacing = false,
              const char *                label = NULL,
              vtkActor       *            demActor = NULL,
              vtkPolyData    *            demPolys = NULL,
              bool                        constrainedToSurface = false,
              double                      heightOffsetAboveSurface = 0.0
              )
{
  VTK_CREATE( vtkHandleWidget, widget );

  vtkHandleRepresentation *rep = NULL;

  if (cameraFacing && shape <= 12)
    {
    rep = vtkOrientedPolygonalHandleRepresentation3D::New();

    VTK_CREATE( vtkGlyphSource2D, glyphs );
    glyphs->SetGlyphType( shape );
    glyphs->SetScale( 600 );
    glyphs->Update();
    static_cast<vtkOrientedPolygonalHandleRepresentation3D *>(rep)->
      SetHandle(glyphs->GetOutput());
    }
  else
    {
    if (shape == 12)
      {
      rep = vtkPolygonalHandleRepresentation3D::New();

      VTK_CREATE( vtkSphereSource, sphere );
      sphere->SetThetaResolution(10);
      sphere->SetPhiResolution(10);
      sphere->SetRadius(300.0);
      sphere->Update();
      static_cast<vtkPolygonalHandleRepresentation3D *>(rep)->SetHandle(sphere->GetOutput());
      }

    if (shape == 13)
      {
      rep = vtkPointHandleRepresentation3D::New();
      }
    }

  if (constrainedToSurface)
    {
    VTK_CREATE( vtkPolygonalSurfacePointPlacer, pointPlacer );
    pointPlacer->AddProp(demActor);
    pointPlacer->GetPolys()->AddItem( demPolys );
    pointPlacer->SetDistanceOffset( heightOffsetAboveSurface );
    rep->SetPointPlacer(pointPlacer);

    // Let the surface constrained point-placer be the sole constraint dictating
    // the placement of handles. Lets not over-constrain it allowing axis
    // constrained interactions.
    widget->EnableAxisConstraintOff();
    }

  double xyz[3] = {x, y, z};
  rep->SetWorldPosition( xyz );
  widget->SetInteractor(iren);
  widget->SetRepresentation( rep );

  // Set some defaults on the handle widget
  double color[3] = { ((double)(shape%4))/3.0,
                      ((double)((shape+3)%7))/6.0,
                      (double)(shape%2) };
  double selectedColor[3] = { 1.0, 0.0, 0.0 };

  if (vtkAbstractPolygonalHandleRepresentation3D *arep =
      vtkAbstractPolygonalHandleRepresentation3D::SafeDownCast(rep))
    {
    arep->GetProperty()->SetColor( color );
    arep->GetProperty()->SetLineWidth(1.0);
    arep->GetSelectedProperty()->SetColor( selectedColor );

    if (label)
      {
      arep->SetLabelVisibility(1);
      arep->SetLabelText(label);
      }
    }

  if (vtkPointHandleRepresentation3D *prep =
      vtkPointHandleRepresentation3D::SafeDownCast(rep))
    {
    prep->GetProperty()->SetColor( color );
    prep->GetProperty()->SetLineWidth(1.0);
    prep->GetSelectedProperty()->SetColor( selectedColor );
    }


  rep->Delete();

  return widget;

}

int TestPolygonalHandleRepresentations(int argc, char*argv[])
{
  if (argc < 2)
    {
    std::cerr
      << "Demonstrates various polyonal handle representations in a scene."
      << std::endl;
    return EXIT_FAILURE;
    }

  // Read height field.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");

  // Read height field.
  //
  vtkSmartPointer<vtkDEMReader> demReader =
    vtkSmartPointer<vtkDEMReader>::New();
  demReader->SetFileName(fname);
  delete [] fname;

  vtkSmartPointer<vtkImageResample>  resample =
    vtkSmartPointer<vtkImageResample>::New();
  resample->SetInputConnection(demReader->GetOutputPort());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0,1);
  resample->SetAxisMagnificationFactor(1,1);

  // Extract geometry
  vtkSmartPointer<vtkImageDataGeometryFilter> surface =
    vtkSmartPointer<vtkImageDataGeometryFilter>::New();
  surface->SetInputConnection(resample->GetOutputPort());

  // The Dijkistra interpolator will not accept cells that aren't triangles
  vtkSmartPointer<vtkTriangleFilter> triangleFilter =
    vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputConnection( surface->GetOutputPort() );
  triangleFilter->Update();

  vtkSmartPointer<vtkWarpScalar> warp =
    vtkSmartPointer<vtkWarpScalar>::New();
  warp->SetInputConnection(triangleFilter->GetOutputPort());
  warp->SetScaleFactor(1);
  warp->UseNormalOn();
  warp->SetNormal(0, 0, 1);
  warp->Update();

  // Define a LUT mapping for the height field

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  vtkSmartPointer<vtkLookupTable> lut =
    vtkSmartPointer<vtkLookupTable>::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);

  vtkSmartPointer<vtkPolyDataNormals> normals =
    vtkSmartPointer<vtkPolyDataNormals>::New();

  normals->SetInputConnection(warp->GetOutputPort());
  normals->SetFeatureAngle(60);
  normals->SplittingOff();

  // vtkPolygonalSurfacePointPlacer needs cell normals
  normals->ComputeCellNormalsOn();
  normals->Update();

  vtkPolyData *pd = normals->GetOutput();

  vtkSmartPointer<vtkPolyDataMapper> demMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  demMapper->SetInputConnection(normals->GetOutputPort());
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> demActor =
    vtkSmartPointer<vtkActor>::New();
  demActor->SetMapper(demMapper);

  // Create the RenderWindow, Renderer and the DEM + path actors.

  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer, set the background and size

  ren1->AddActor(demActor);

  ren1->GetActiveCamera()->SetViewUp(0, 0, 1);
  ren1->GetActiveCamera()->SetPosition(-99900, -21354, 131801);
  ren1->GetActiveCamera()->SetFocalPoint(41461, 41461, 2815);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  // Here comes the surface constrained handle widget stuff.....
  vtkSmartPointer< vtkHandleWidget > widget[14];
  widget[0] = CreateWidget( iren, VTK_VERTEX_GLYPH,        561909, 5.11921e+06, 4381.48, true, "Vertex" );
  widget[1] = CreateWidget( iren, VTK_DASH_GLYPH,          559400, 5.11064e+06, 2323.25, true, "Dash" );
  widget[2] = CreateWidget( iren, VTK_CROSS_GLYPH,         563531, 5.11924e+06, 5202.51, true, "cross" );
  widget[3] = CreateWidget( iren, VTK_THICKCROSS_GLYPH,    563300, 5.11729e+06, 4865.47, true, "Thick Cross" );
  widget[4] = CreateWidget( iren, VTK_TRIANGLE_GLYPH,      564392, 5.11248e+06, 3936.91, true, "triangle" );
  widget[5] = CreateWidget( iren, VTK_SQUARE_GLYPH,        563715, 5.11484e+06, 4345.68, true, "square" );
  widget[6] = CreateWidget( iren, VTK_CIRCLE_GLYPH,        564705, 5.10849e+06, 2335.16, true, "circle" );
  widget[7] = CreateWidget( iren, VTK_DIAMOND_GLYPH,       560823, 5.1202e+06 ,3783.94, true, "diamond" );
  widget[8] = CreateWidget( iren, VTK_ARROW_GLYPH,         559637, 5.12068e+06, 2718.66, true, "arrow" );
  widget[9] = CreateWidget( iren, VTK_THICKARROW_GLYPH,    560597, 5.10817e+06, 3582.44, true, "thickArrow" );
  widget[10] = CreateWidget( iren, VTK_HOOKEDARROW_GLYPH,  558266, 5.12137e+06, 2559.14, true, "hookedArrow" );
  widget[11] = CreateWidget( iren, VTK_EDGEARROW_GLYPH,    568869, 5.11028e+06, 2026.57, true, "EdgeArrow" );
  widget[12] = CreateWidget( iren, 12,                    561753, 5.11577e+06, 3183, false, "Sphere contrained to surface", demActor, pd, true, 100.0 );
  widget[13] = CreateWidget( iren, 13,                     562692, 5.11521e+06, 3355.65, false, "Crosshair" );

  renWin->SetSize(700,700);
  renWin->Render();
  iren->Initialize();

  for (unsigned int i = 0; i < 14; i++)
    {
    widget[i]->EnabledOn();
    }

  renWin->Render();

  iren->Start();


  return EXIT_SUCCESS;
}


