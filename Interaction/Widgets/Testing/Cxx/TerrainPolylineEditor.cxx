/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TerrainPolylineEditor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkDEMReader.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyData.h"
#include "vtkProjectedTerrainPath.h"
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
#include "vtkTerrainDataPointPlacer.h"
#include "vtkTerrainContourLineInterpolator.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkTestUtilities.h"

const char TerrainPolylineEditorLog[] =
"# StreamVersion 1\n"
"EnterEvent 522 259 0 0 0 0 0 i\n"
"MouseMoveEvent 446 277 0 0 0 0 0 i\n"
"MouseMoveEvent 166 322 0 0 0 0 0 i\n"
"MouseMoveEvent 138 333 0 0 0 0 0 i\n"
"LeftButtonPressEvent 138 333 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 138 333 0 0 0 0 0 i\n"
"MouseMoveEvent 170 338 0 0 0 0 0 i\n"
"MouseMoveEvent 184 336 0 0 0 0 0 i\n"
"MouseMoveEvent 190 335 0 0 0 0 0 i\n"
"LeftButtonPressEvent 190 335 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 190 335 0 0 0 0 0 i\n"
"MouseMoveEvent 234 328 0 0 0 0 0 i\n"
"MouseMoveEvent 235 327 0 0 0 0 0 i\n"
"LeftButtonPressEvent 235 327 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 235 327 0 0 0 0 0 i\n"
"MouseMoveEvent 263 310 0 0 0 0 0 i\n"
"MouseMoveEvent 267 307 0 0 0 0 0 i\n"
"LeftButtonPressEvent 267 307 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 267 307 0 0 0 0 0 i\n"
"MouseMoveEvent 270 294 0 0 0 0 0 i\n"
"MouseMoveEvent 271 289 0 0 0 0 0 i\n"
"MouseMoveEvent 272 281 0 0 0 0 0 i\n"
"LeftButtonPressEvent 272 281 0 0 0 0 0 i\n"
"MouseMoveEvent 281 263 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 272 280 0 0 0 0 0 i\n"
"MouseMoveEvent 281 263 0 0 0 0 0 i\n"
"MouseMoveEvent 290 258 0 0 0 0 0 i\n"
"MouseMoveEvent 291 258 0 0 0 0 0 i\n"
"LeftButtonPressEvent 291 258 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 291 258 0 0 0 0 0 i\n"
"MouseMoveEvent 335 251 0 0 0 0 0 i\n"
"MouseMoveEvent 350 251 0 0 0 0 0 i\n"
"MouseMoveEvent 354 251 0 0 0 0 0 i\n"
"LeftButtonPressEvent 354 251 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 354 251 0 0 0 0 0 i\n"
"MouseMoveEvent 440 247 0 0 0 0 0 i\n"
"MouseMoveEvent 437 256 0 0 0 0 0 i\n"
"MouseMoveEvent 438 263 0 0 0 0 0 i\n"
"LeftButtonPressEvent 438 263 0 0 0 0 0 i\n"
"MouseMoveEvent 472 252 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 439 263 0 0 0 0 0 i\n"
"MouseMoveEvent 472 252 0 0 0 0 0 i\n"
"MouseMoveEvent 475 250 0 0 0 0 0 i\n"
"MouseMoveEvent 475 249 0 0 0 0 0 i\n"
"LeftButtonPressEvent 475 249 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 475 249 0 0 0 0 0 i\n"
"MouseMoveEvent 475 248 0 0 0 0 0 i\n"
"MouseMoveEvent 491 239 0 0 0 0 0 i\n"
"MouseMoveEvent 511 238 0 0 0 0 0 i\n"
"LeftButtonPressEvent 511 238 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 511 238 0 0 0 0 0 i\n"
"MouseMoveEvent 541 217 0 0 0 0 0 i\n"
"MouseMoveEvent 544 213 0 0 0 0 0 i\n"
"MouseMoveEvent 544 212 0 0 0 0 0 i\n"
"LeftButtonPressEvent 544 212 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 544 212 0 0 0 0 0 i\n"
"MouseMoveEvent 542 209 0 0 0 0 0 i\n"
"MouseMoveEvent 531 198 0 0 0 0 0 i\n"
"MouseMoveEvent 553 208 0 0 0 0 0 i\n"
"MouseMoveEvent 554 209 0 0 0 0 0 i\n"
"RightButtonPressEvent 554 209 0 0 0 0 0 i\n"
"RightButtonReleaseEvent 554 209 0 0 0 0 0 i\n"
"MouseMoveEvent 533 200 0 0 0 0 0 i\n"
"MouseMoveEvent 230 257 0 0 0 0 0 i\n"
"MouseMoveEvent 237 325 0 0 0 0 0 i\n"
"MouseMoveEvent 261 314 0 0 0 0 0 i\n"
"MouseMoveEvent 266 313 0 0 0 0 0 i\n"
"LeftButtonPressEvent 266 313 0 0 0 0 0 i\n"
"MouseMoveEvent 267 313 0 0 0 0 0 i\n"
"MouseMoveEvent 310 316 0 0 0 0 0 i\n"
"MouseMoveEvent 299 316 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 299 316 0 0 0 0 0 i\n"
"MouseMoveEvent 295 312 0 0 0 0 0 i\n"
"MouseMoveEvent 267 263 0 0 0 0 0 i\n"
"MouseMoveEvent 274 258 0 0 0 0 0 i\n"
"MouseMoveEvent 285 262 0 0 0 0 0 i\n"
"MouseMoveEvent 285 263 0 0 0 0 0 i\n"
"LeftButtonPressEvent 285 263 0 0 0 0 0 i\n"
"MouseMoveEvent 286 262 0 0 0 0 0 i\n"
"MouseMoveEvent 314 266 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 314 266 0 0 0 0 0 i\n"
"MouseMoveEvent 316 266 0 0 0 0 0 i\n"
"MouseMoveEvent 217 305 0 0 0 0 0 i\n"
"MouseMoveEvent 140 335 0 0 0 0 0 i\n"
"MouseMoveEvent 185 339 0 0 0 0 0 i\n"
"MouseMoveEvent 191 339 0 0 0 0 0 i\n"
"LeftButtonPressEvent 191 339 0 0 0 0 0 i\n"
"MouseMoveEvent 191 340 0 0 0 0 0 i\n"
"MouseMoveEvent 199 353 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 199 353 0 0 0 0 0 i\n"
"MouseMoveEvent 199 351 0 0 0 0 0 i\n"
"MouseMoveEvent 235 332 0 0 0 0 0 i\n"
"MouseMoveEvent 235 333 0 0 0 0 0 i\n"
"LeftButtonPressEvent 235 333 0 0 0 0 0 i\n"
"MouseMoveEvent 235 332 0 0 0 0 0 i\n"
"MouseMoveEvent 248 346 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 248 346 0 0 0 0 0 i\n"
"MouseMoveEvent 248 344 0 0 0 0 0 i\n"
"MouseMoveEvent 246 313 0 0 0 0 0 i\n"
"MouseMoveEvent 259 238 0 0 0 0 0 i\n"
"MouseMoveEvent 294 216 0 0 0 0 0 i\n"
"MouseMoveEvent 506 211 0 0 0 0 0 i\n"
"MouseMoveEvent 547 206 0 0 0 0 0 i\n"
"MouseMoveEvent 554 209 0 0 0 0 0 i\n"
"MouseMoveEvent 555 210 0 0 0 0 0 i\n"
"LeftButtonPressEvent 555 210 0 0 0 0 0 i\n"
"MouseMoveEvent 555 209 0 0 0 0 0 i\n"
"MouseMoveEvent 511 115 0 0 0 0 0 i\n"
"MouseMoveEvent 510 114 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 510 114 0 0 0 0 0 i\n"
"MouseMoveEvent 512 115 0 0 0 0 0 i\n"
"MouseMoveEvent 534 154 0 0 0 0 0 i\n"
"MouseMoveEvent 531 165 0 0 0 0 0 i\n"
"MouseMoveEvent 526 169 0 0 0 0 0 i\n"
"LeftButtonPressEvent 526 169 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 526 169 0 0 0 0 0 i\n"
"MouseMoveEvent 525 166 0 0 0 0 0 i\n"
"MouseMoveEvent 526 166 0 0 0 0 0 i\n"
"MouseMoveEvent 526 173 0 0 0 0 0 i\n"
"LeftButtonPressEvent 526 173 0 0 0 0 0 i\n"
"MouseMoveEvent 526 174 0 0 0 0 0 i\n"
"MouseMoveEvent 510 179 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 510 179 0 0 0 0 0 i\n"
"MouseMoveEvent 510 177 0 0 0 0 0 i\n"
"MouseMoveEvent 510 125 0 0 0 0 0 i\n"
"MouseMoveEvent 500 121 0 0 0 0 0 i\n"
"MouseMoveEvent 435 104 0 0 0 0 0 i\n"
"MouseMoveEvent 422 109 0 0 0 0 0 i\n"
"MouseMoveEvent 409 121 0 0 0 0 0 i\n"
"MouseMoveEvent 402 130 0 0 0 0 0 i\n"
"KeyPressEvent 402 130 0 0 113 1 q i\n"
"CharEvent 402 130 0 0 113 1 q i\n"
"ExitEvent 402 130 0 0 113 1 q i\n";

int TerrainPolylineEditor(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr
    << "Demonstrates editing capabilities of a contour widget on terrain \n"
    << "data. Additional arguments : \n"
    << "\tThe projection mode may optionally be specified. [0-Simple,1-NonOccluded\n"
    << ",2-Hug]. (defaults to Hug)\n"
    << "\tA height offset may be specified. Defaults to 0.0\n"
    << "\tIf a polydata is specified, an initial contour is constucted from\n"
    << "the points in the polydata. The polydata is expected to be a polyline\n"
    << "(one cell and two or more points on that cell)."
    << std::endl;
    std::cerr << "\n\nUsage: " << argv[0] << "\n"
              << "  [-ProjectionMode (0,1 or 2)]\n"
              << "  [-HeightOffset heightOffset]\n"
              << "  [-InitialPath SomeVTKXmlfileContainingPath.vtk]"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Read height field.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");
  vtkSmartPointer<vtkDEMReader> demReader =
    vtkSmartPointer<vtkDEMReader>::New();
  demReader->SetFileName(fname);
  delete [] fname;

  // Extract geometry

  vtkSmartPointer<vtkImageDataGeometryFilter> surface =
    vtkSmartPointer<vtkImageDataGeometryFilter>::New();
  surface->SetInputConnection(demReader->GetOutputPort());

  vtkSmartPointer<vtkWarpScalar> warp =
    vtkSmartPointer<vtkWarpScalar>::New();
  warp->SetInputConnection(surface->GetOutputPort());
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

  vtkSmartPointer<vtkPolyDataMapper> demMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  demMapper->SetInputConnection(normals->GetOutputPort());
  normals->Update();
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

  renWin->SetSize(600,600);
  ren1->AddActor(demActor);
  ren1->GetActiveCamera()->SetViewUp(0, 0, 1);
  ren1->GetActiveCamera()->SetPosition(-99900, -21354, 131801);
  ren1->GetActiveCamera()->SetFocalPoint(41461, 41461, 2815);
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Dolly(1.2);
  ren1->ResetCameraClippingRange();

  // Here comes the contour widget stuff.....

  vtkSmartPointer<vtkContourWidget> contourWidget =
    vtkSmartPointer<vtkContourWidget>::New();
  vtkOrientedGlyphContourRepresentation *rep =
      vtkOrientedGlyphContourRepresentation::SafeDownCast(
                        contourWidget->GetRepresentation());
  rep->GetLinesProperty()->SetColor(1.0, 0.0, 0.0);
  contourWidget->SetInteractor(iren);

  // Set the point placer to the one used for terrains...

  vtkSmartPointer<vtkTerrainDataPointPlacer>  pointPlacer =
    vtkSmartPointer<vtkTerrainDataPointPlacer>::New();
  pointPlacer->AddProp(demActor);    // the actor(s) containing the terrain.
  rep->SetPointPlacer(pointPlacer);

  // Set a terrain interpolator. Interpolates points as they are placed,
  // so that they lie on the terrain.

  vtkSmartPointer<vtkTerrainContourLineInterpolator> interpolator
          = vtkSmartPointer<vtkTerrainContourLineInterpolator>::New();
  rep->SetLineInterpolator(interpolator);
  interpolator->SetImageData(demReader->GetOutput());

  // Set the default projection mode to hug the terrain, unless user
  // overrides it.
  //
  interpolator->GetProjector()->SetProjectionModeToHug();
  for (int i = 0; i < argc-1; i++)
  {
    if (strcmp("-ProjectionMode", argv[i]) == 0)
    {
      interpolator->GetProjector()->SetProjectionMode(atoi(argv[i+1]));
    }
    if (strcmp("-HeightOffset", argv[i]) == 0)
    {
      interpolator->GetProjector()->SetHeightOffset(atoi(argv[i+1]));
      pointPlacer->SetHeightOffset(atof(argv[i+1]));
    }
    if (strcmp("-InitialPath", argv[i]) == 0)
    {
      // If we had an input poly as an initial path, build a contour
      // widget from that path.
      //
      vtkSmartPointer<vtkPolyDataReader> terrainPathReader =
        vtkSmartPointer<vtkPolyDataReader>::New();
      terrainPathReader->SetFileName(argv[i+1]);
      terrainPathReader->Update();
      contourWidget->Initialize( terrainPathReader->GetOutput(), 0 );
    }
  }

  contourWidget->EnabledOn();

  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TerrainPolylineEditorLog);
  recorder->EnabledOn();

  renWin->Render();
  iren->Initialize();

  recorder->Play();

  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
