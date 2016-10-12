/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDijkstraImageGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"

#include "vtkContourWidget.h"
#include "vtkDijkstraImageContourLineInterpolator.h"
#include "vtkDijkstraImageGeodesicPath.h"
#include "vtkImageActor.h"
#include "vtkImageActorPointPlacer.h"
#include "vtkImageAnisotropicDiffusion2D.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkImageGradientMagnitude.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkImageShiftScale.h"
#include "vtkInteractorStyleImage.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkPNGReader.h"
#include "vtkProperty.h"
#include "vtkTesting.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"

const char TestDijkstraImageGeodesicPathLog[] =
  "# StreamVersion 1 i\n"
  "RenderEvent 0 0 0 0 0 0 0 i\n"
  "EnterEvent 399 96 0 0 0 0 0 i\n"
  "MouseMoveEvent 321 96 0 0 0 0 0 i\n"
  "RightButtonPressEvent 321 96 0 0 0 0 0 i\n"
  "StartInteractionEvent 321 96 0 0 0 0 0 i\n"
  "MouseMoveEvent 321 97 0 0 0 0 0 i\n"
  "RenderEvent 321 97 0 0 0 0 0 i\n"
  "MouseMoveEvent 316 169 0 0 0 0 0 i\n"
  "RenderEvent 316 169 0 0 0 0 0 i\n"
  "RightButtonReleaseEvent 316 169 0 0 0 0 0 i\n"
  "EndInteractionEvent 316 169 0 0 0 0 0 i\n"
  "RenderEvent 316 169 0 0 0 0 0 i\n"
  "MouseMoveEvent 190 356 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 190 356 0 0 0 0 0 i\n"
  "RenderEvent 190 356 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 190 356 0 0 0 0 0 i\n"
  "MouseMoveEvent 61 226 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 61 226 0 0 0 0 0 i\n"
  "RenderEvent 61 226 0 0 0 0 0 i\n"
  "MouseMoveEvent 62 226 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 62 226 0 0 0 0 0 i\n"
  "MouseMoveEvent 131 49 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 131 49 0 0 0 0 0 i\n"
  "RenderEvent 131 49 0 0 0 0 0 i\n"
  "MouseMoveEvent 131 50 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 131 50 0 0 0 0 0 i\n"
  "MouseMoveEvent 292 69 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 292 69 0 0 0 0 0 i\n"
  "RenderEvent 292 69 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 292 69 0 0 0 0 0 i\n"
  "MouseMoveEvent 347 189 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 347 189 0 0 0 0 0 i\n"
  "RenderEvent 347 189 0 0 0 0 0 i\n"
  "MouseMoveEvent 347 190 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 347 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 300 302 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 300 302 0 0 0 0 0 i\n"
  "RenderEvent 300 302 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 300 302 0 0 0 0 0 i\n"
  "MouseMoveEvent 191 354 0 0 0 0 0 i\n"
  "RightButtonPressEvent 191 354 0 0 0 0 0 i\n"
  "RenderEvent 191 354 0 0 0 0 0 i\n"
  "RightButtonReleaseEvent 191 354 0 0 0 0 0 i\n"
  "MouseMoveEvent 63 225 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 63 225 0 0 0 0 0 i\n"
  "MouseMoveEvent 63 226 0 0 0 0 0 i\n"
  "RenderEvent 63 226 0 0 0 0 0 i\n"
  "MouseMoveEvent 63 238 0 0 0 0 0 i\n"
  "RenderEvent 63 238 0 0 0 0 0 i\n"
  "MouseMoveEvent 63 239 0 0 0 0 0 i\n"
  "RenderEvent 63 239 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 63 239 0 0 0 0 0 i\n"
  "MouseMoveEvent 127 47 0 0 0 0 0 i\n"
  "KeyPressEvent 127 47 0 0 0 1 Delete i\n"
  "RenderEvent 127 47 0 0 0 1 Delete  i\n"
  "KeyReleaseEvent 127 47 0 0 0 1 Delete i\n"
  "MouseMoveEvent 286 71 0 0 0 0 Delete i\n"
  "RenderEvent 286 71 0 0 0 0 Delete i\n"
  "MouseMoveEvent 287 68 0 0 0 0 Delete i\n"
  "KeyPressEvent 287 68 0 0 0 1 Delete i\n"
  "RenderEvent 287 68 0 0 0 1 Delete i\n"
  "KeyReleaseEvent 287 68 0 0 0 1 Delete i\n"
  "MouseMoveEvent 179 218 0 0 0 0 Delete i\n"
  "LeftButtonPressEvent 179 218 0 0 0 0 Delete i\n"
  "MouseMoveEvent 78 122 0 0 0 0 Delete i\n"
  "RenderEvent 78 122 0 0 0 0 Delete i\n"
  "LeftButtonReleaseEvent 78 122 0 0 0 0 Delete i\n"
  "MouseMoveEvent 154 106 0 0 0 0 Delete i\n"
  "KeyPressEvent 154 106 0 0 113 1 q i\n"
  "CharEvent 154 106 0 0 113 1 q i\n"
  "ExitEvent 154 106 0 0 113 1 q i\n"
  ;

int TestDijkstraImageGeodesicPath(int argc, char*argv[])
{
  bool followCursor = false;
  for (int i = 0; i < argc; i++)
  {
    followCursor  |= (strcmp("--FollowCursor", argv[i]) == 0);
  }

  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fullhead15.png");

  vtkSmartPointer<vtkPNGReader> reader =
    vtkSmartPointer<vtkPNGReader>::New();
  reader->SetFileName( fname );
  delete [] fname;

  // Smooth the image
  vtkSmartPointer<vtkImageAnisotropicDiffusion2D> diffusion =
    vtkSmartPointer<vtkImageAnisotropicDiffusion2D>::New();
  diffusion->SetInputConnection( reader->GetOutputPort() );
  diffusion->SetDiffusionFactor( 1.0 );
  diffusion->SetDiffusionThreshold( 200.0 );
  diffusion->SetNumberOfIterations( 5 );

  // Gradient magnitude for edges
  vtkSmartPointer<vtkImageGradientMagnitude> grad =
    vtkSmartPointer<vtkImageGradientMagnitude>::New();
  grad->SetDimensionality( 2 );
  grad->HandleBoundariesOn();
  grad->SetInputConnection( diffusion->GetOutputPort() );
  grad->Update();

  double* range = grad->GetOutput()->GetScalarRange();

  // Invert the gradient magnitude so that low costs are
  // associated with strong edges and scale from 0 to 1
  vtkSmartPointer<vtkImageShiftScale> gradInvert =
    vtkSmartPointer<vtkImageShiftScale>::New();
  gradInvert->SetShift( -1.0*range[ 1 ] );
  gradInvert->SetScale( 1.0 /( range[ 0 ] - range[ 1 ] ) );
  gradInvert->SetOutputScalarTypeToFloat();
  gradInvert->SetInputConnection( grad->GetOutputPort() );
  gradInvert->Update();

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer( renderer );
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( renWin );
  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  iren->SetInteractorStyle( style );

  // The color map will accept any scalar image type and convert to
  // unsigned char for the image actor
  vtkSmartPointer<vtkImageMapToWindowLevelColors> colorMap =
    vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
  colorMap->SetInputConnection( gradInvert->GetOutputPort() );

  range = gradInvert->GetOutput()->GetScalarRange();
  colorMap->SetWindow(1.0);
  colorMap->SetLevel(0.5);

  vtkSmartPointer<vtkImageActor> actor =
    vtkSmartPointer<vtkImageActor>::New();
  actor->GetMapper()->SetInputConnection( colorMap->GetOutputPort() );
  actor->SetDisplayExtent( 0, 255, 0, 255, 0, 0 );

  renderer->AddActor( actor );

  renderer->SetBackground(0.2,0.2,1);
  renWin->SetSize(400,400);

  // Contour widget for interactive path definition
  vtkSmartPointer<vtkContourWidget> contourWidget =
    vtkSmartPointer<vtkContourWidget>::New();
  contourWidget->SetInteractor( iren );

  vtkSmartPointer<vtkOrientedGlyphContourRepresentation> rep =
    vtkSmartPointer<vtkOrientedGlyphContourRepresentation>::New();
  contourWidget->SetRepresentation( rep );
  contourWidget->SetFollowCursor( followCursor );

  rep->GetLinesProperty()->SetColor(1, 0.2, 0);
  rep->GetProperty()->SetColor(0, 0.2, 1);
  rep->GetLinesProperty()->SetLineWidth( 3 );

  // The contour rep requires a suitable point placer
  vtkSmartPointer<vtkImageActorPointPlacer> placer =
    vtkSmartPointer<vtkImageActorPointPlacer>::New();
  placer->SetImageActor( actor );
  rep->SetPointPlacer( placer );

  // The line interpolator defines how intermediate points are
  // generated between the representations nodes.  This
  // interpolator uses Dijkstra's shortest path algorithm.
  vtkSmartPointer< vtkDijkstraImageContourLineInterpolator > interpolator =
    vtkSmartPointer< vtkDijkstraImageContourLineInterpolator >::New();
  interpolator->SetCostImage( gradInvert->GetOutput() );

  vtkDijkstraImageGeodesicPath *path =
    interpolator->GetDijkstraImageGeodesicPath();
  path->StopWhenEndReachedOn();
  // prevent contour segments from overlapping
  path->RepelPathFromVerticesOn();
  // weights are scaled from 0 to 1 as are associated cost
  // components
  path->SetCurvatureWeight( 0.15 );
  path->SetEdgeLengthWeight( 0.8 );
  path->SetImageWeight( 1.0 );

  rep->SetLineInterpolator( interpolator );
  contourWidget->EnabledOn();

  renWin->Render();
  renderer->ResetCamera();
  iren->Initialize();

  return vtkTesting::InteractorEventLoop(
      argc, argv, iren, TestDijkstraImageGeodesicPathLog );
}
