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

// This test demonstrates a distance widget constrained to lie on the
// surface of a polygonal mesh. Both handles are constrained to the mesh'
// surface. Optionally, one can also specify a height offset. If specified,
// the end points of the distance widget are constrained to lie at a height
// offset from the surface of the mesh. The "height" at any location on
// the surface is measured as the offset of the point in the direction of
// the surface normal.

#include "vtkSmartPointer.h"

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
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyDataCollection.h"
#include "vtkTriangleFilter.h"
#include "vtkImageResample.h"
#include "vtkInteractorEventRecorder.h"

#include "vtkDistanceWidget.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolygonalSurfacePointPlacer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

const char TestPolygonalSurfaceConstrainedDistanceWidgetLog[] =
  "# StreamVersion 1 i\n"
  "RenderEvent 0 0 0 0 0 0 0 i\n"
  "EnterEvent 293 1 0 0 0 0 0 i\n"
  "MouseMoveEvent 293 1 0 0 0 0 0 i\n"
  "MouseMoveEvent 281 6 0 0 0 0 0 i\n"
  "MouseMoveEvent 270 10 0 0 0 0 0 i\n"
  "MouseMoveEvent 261 14 0 0 0 0 0 i\n"
  "MouseMoveEvent 253 17 0 0 0 0 0 i\n"
  "MouseMoveEvent 247 20 0 0 0 0 0 i\n"
  "MouseMoveEvent 243 23 0 0 0 0 0 i\n"
  "MouseMoveEvent 240 26 0 0 0 0 0 i\n"
  "MouseMoveEvent 230 33 0 0 0 0 0 i\n"
  "MouseMoveEvent 220 41 0 0 0 0 0 i\n"
  "MouseMoveEvent 210 49 0 0 0 0 0 i\n"
  "MouseMoveEvent 201 58 0 0 0 0 0 i\n"
  "MouseMoveEvent 192 67 0 0 0 0 0 i\n"
  "MouseMoveEvent 183 75 0 0 0 0 0 i\n"
  "MouseMoveEvent 176 84 0 0 0 0 0 i\n"
  "MouseMoveEvent 168 93 0 0 0 0 0 i\n"
  "MouseMoveEvent 162 102 0 0 0 0 0 i\n"
  "MouseMoveEvent 154 110 0 0 0 0 0 i\n"
  "MouseMoveEvent 149 119 0 0 0 0 0 i\n"
  "MouseMoveEvent 143 127 0 0 0 0 0 i\n"
  "MouseMoveEvent 139 135 0 0 0 0 0 i\n"
  "MouseMoveEvent 134 140 0 0 0 0 0 i\n"
  "MouseMoveEvent 130 144 0 0 0 0 0 i\n"
  "MouseMoveEvent 128 148 0 0 0 0 0 i\n"
  "MouseMoveEvent 125 151 0 0 0 0 0 i\n"
  "MouseMoveEvent 123 154 0 0 0 0 0 i\n"
  "MouseMoveEvent 122 155 0 0 0 0 0 i\n"
  "MouseMoveEvent 121 156 0 0 0 0 0 i\n"
  "MouseMoveEvent 121 158 0 0 0 0 0 i\n"
  "MouseMoveEvent 121 159 0 0 0 0 0 i\n"
  "MouseMoveEvent 120 160 0 0 0 0 0 i\n"
  "MouseMoveEvent 119 160 0 0 0 0 0 i\n"
  "MouseMoveEvent 119 161 0 0 0 0 0 i\n"
  "MouseMoveEvent 119 162 0 0 0 0 0 i\n"
  "MouseMoveEvent 118 162 0 0 0 0 0 i\n"
  "MouseMoveEvent 117 163 0 0 0 0 0 i\n"
  "MouseMoveEvent 116 164 0 0 0 0 0 i\n"
  "MouseMoveEvent 115 165 0 0 0 0 0 i\n"
  "MouseMoveEvent 115 166 0 0 0 0 0 i\n"
  "MouseMoveEvent 113 166 0 0 0 0 0 i\n"
  "MouseMoveEvent 112 167 0 0 0 0 0 i\n"
  "MouseMoveEvent 111 168 0 0 0 0 0 i\n"
  "MouseMoveEvent 109 168 0 0 0 0 0 i\n"
  "MouseMoveEvent 108 169 0 0 0 0 0 i\n"
  "MouseMoveEvent 107 170 0 0 0 0 0 i\n"
  "MouseMoveEvent 105 170 0 0 0 0 0 i\n"
  "MouseMoveEvent 104 171 0 0 0 0 0 i\n"
  "MouseMoveEvent 103 172 0 0 0 0 0 i\n"
  "MouseMoveEvent 102 172 0 0 0 0 0 i\n"
  "MouseMoveEvent 101 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 100 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 99 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 98 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 97 173 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 97 173 0 0 0 0 0 i\n"
  "RenderEvent 97 173 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 97 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 177 134 0 0 0 0 0 i\n"
  "RenderEvent 177 134 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 177 134 0 0 0 0 0 i\n"
  "RenderEvent 177 134 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 177 134 0 0 0 0 0 i\n"
  "MouseMoveEvent 177 134 0 0 0 0 0 i\n"
  "RenderEvent 177 134 0 0 0 0 0 i\n"
  "MouseMoveEvent 178 134 0 0 0 0 0 i\n"
  "RenderEvent 178 134 0 0 0 0 0 i\n"
  "MouseMoveEvent 216 133 0 0 0 0 0 i\n"
  "RenderEvent 216 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 236 133 0 0 0 0 0 i\n"
  "RenderEvent 236 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 232 133 0 0 0 0 0 i\n"
  "RenderEvent 232 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 211 133 0 0 0 0 0 i\n"
  "RenderEvent 211 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 189 133 0 0 0 0 0 i\n"
  "RenderEvent 189 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 186 133 0 0 0 0 0 i\n"
  "RenderEvent 186 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 185 133 0 0 0 0 0 i\n"
  "RenderEvent 185 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 182 133 0 0 0 0 0 i\n"
  "RenderEvent 182 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 179 133 0 0 0 0 0 i\n"
  "RenderEvent 179 133 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 179 133 0 0 0 0 0 i\n"
  "RenderEvent 179 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 187 145 0 0 0 0 0 i\n"
  "RenderEvent 187 145 0 0 0 0 0 i\n"
  "MouseMoveEvent 211 158 0 0 0 0 0 i\n"
  "RenderEvent 211 158 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 211 158 0 0 0 0 0 i\n"
  "RenderEvent 211 158 0 0 0 0 0 i\n"
  "MouseMoveEvent 211 158 0 0 0 0 0 i\n"
  "RenderEvent 211 158 0 0 0 0 0 i\n"
  "MouseMoveEvent 211 155 0 0 0 0 0 i\n"
  "RenderEvent 211 155 0 0 0 0 0 i\n"
  "MouseMoveEvent 209 118 0 0 0 0 0 i\n"
  "RenderEvent 209 118 0 0 0 0 0 i\n"
  "MouseMoveEvent 208 119 0 0 0 0 0 i\n"
  "RenderEvent 208 119 0 0 0 0 0 i\n"
  "KeyPressEvent 208 119 0 0 113 1 q i\n"
  "CharEvent 208 119 0 0 113 1 q i\n"
  "ExitEvent 208 119 0 0 113 1 q i\n"
  ;

int TestPolygonalSurfaceConstrainedDistanceWidget(int argc, char*argv[])
{
  if (argc < 2)
  {
    std::cerr
      << "Demonstrates interaction of a handle, so that it is constrained \n"
      << "to lie on a polygonal surface.\n\n"
      << "Usage args: [-DistanceOffset height_offset]."
      << std::endl;
    return EXIT_FAILURE;
  }

  // Read height field.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");

  // Convert it to a vtkPolyData


  // Read height field.
  //
  vtkSmartPointer<vtkDEMReader> demReader =
    vtkSmartPointer<vtkDEMReader>::New();
  demReader->SetFileName(fname);
  delete [] fname;

  vtkSmartPointer<vtkImageResample>  resample =
    vtkSmartPointer<vtkImageResample>::New();
  resample->SetInputConnection(
    demReader->GetOutputPort());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0,1.0);
  resample->SetAxisMagnificationFactor(1,1.0);

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
    normals->SetInputConnection(warp->GetOutputPort());
    normals->SetFeatureAngle(60);
    normals->SplittingOff();

    // vtkPolygonalSurfacePointPlacer needs cell normals
    normals->ComputeCellNormalsOn();
    normals->Update();
  }

  vtkPolyData *pd = (distanceOffsetSpecified) ? normals->GetOutput()
    : warp->GetPolyDataOutput();

  // Now pd is the "Polydata" on which we want our distance widget to be
  // constrained.

  // First create the mapper for pd.

  vtkSmartPointer<vtkPolyDataMapper> demMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  demMapper->SetInputData(pd);
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> demActor =
    vtkSmartPointer<vtkActor>::New();
  demActor->SetMapper(demMapper);

  // Create the RenderWindow, Renderer and the DEM actor.

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

  // Here comes the surface constrained distance widget stuff.....

  vtkSmartPointer<vtkDistanceWidget> widget =
    vtkSmartPointer<vtkDistanceWidget>::New();
  widget->CreateDefaultRepresentation();
  vtkSmartPointer< vtkDistanceRepresentation2D > rep =
    vtkSmartPointer< vtkDistanceRepresentation2D >::New();
  rep->GetAxis()->GetProperty()->SetColor( 0.0, 0.0, 1.0 );

  // Create a 3D handle reprensentation template for this distance
  // widget
  vtkSmartPointer< vtkPointHandleRepresentation3D > handleRep3D =
    vtkSmartPointer< vtkPointHandleRepresentation3D >::New();
  handleRep3D->GetProperty()->SetLineWidth(4.0);
  rep->SetHandleRepresentation( handleRep3D );
  handleRep3D->GetProperty()->SetColor( 0.0, 0.0, 0.5 );
  widget->SetRepresentation(rep);

  widget->SetInteractor(iren);

  // Create a polygonal surface point placer to constrain the distance
  // to the surface of the object

  vtkSmartPointer<vtkPolygonalSurfacePointPlacer>  pointPlacer =
    vtkSmartPointer<vtkPolygonalSurfacePointPlacer>::New();
  pointPlacer->AddProp(demActor);
  pointPlacer->GetPolys()->AddItem( pd );

  // We can optionally constain the handles to a certain height (measured as
  // the offset along the surface normal) from the surface.

  if (distanceOffsetSpecified)
  {
    pointPlacer->SetDistanceOffset( distanceOffset );
  }

  // set the placer on the distance' handle representations.

  rep->InstantiateHandleRepresentation();
  rep->GetPoint1Representation()->SetPointPlacer(pointPlacer);
  rep->GetPoint2Representation()->SetPointPlacer(pointPlacer);

  renWin->Render();
  iren->Initialize();
  widget->EnabledOn();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(
      argc, argv, iren, TestPolygonalSurfaceConstrainedDistanceWidgetLog );
}
