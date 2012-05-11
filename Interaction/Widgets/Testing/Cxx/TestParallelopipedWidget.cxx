/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParallelopipedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkCommand.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkParallelopipedWidget.h"
#include "vtkParallelopipedRepresentation.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkCubeSource.h"
#include "vtkMatrix4x4.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkPoints.h"
#include "vtkCubeAxesActor2D.h"

//----------------------------------------------------------------------------
int TestParallelopipedWidget(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  renderer->SetBackground(0.8,0.8,1.0);
  renWin->SetSize(800,600);

  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  cone->SetResolution(6);
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkSmartPointer<vtkAppendPolyData> append =
    vtkSmartPointer<vtkAppendPolyData>::New();
  append->AddInputConnection(glyph->GetOutputPort());
  append->AddInputConnection(sphere->GetOutputPort());
  append->Update();

  vtkSmartPointer<vtkCubeSource>  cube =
    vtkSmartPointer<vtkCubeSource>::New();
  double bounds[6];
  append->GetOutput()->GetBounds(bounds);
  bounds[0] -= (bounds[1]-bounds[0])*0.25;
  bounds[1] += (bounds[1]-bounds[0])*0.25;
  bounds[2] -= (bounds[3]-bounds[2])*0.25;
  bounds[3] += (bounds[3]-bounds[2])*0.25;
  bounds[4] -= (bounds[5]-bounds[4])*0.25;
  bounds[5] += (bounds[5]-bounds[4])*0.25;
  bounds[0] = -1.0;
  bounds[1] = 1.0;
  bounds[2] = -1.0;
  bounds[3] = 1.0;
  bounds[4] = -1.0;
  bounds[5] = 1.0;
  cube->SetBounds(bounds);

  vtkSmartPointer<vtkMatrix4x4>  affineMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  const double m[] = { 1.0,  0.1,  0.2,  0.0,
                       0.1,  1.0,  0.1,  0.0,
                       0.2,  0.1,  1.0,  0.0,
                       0.0,  0.0,  0.0,  1.0 };
  affineMatrix->DeepCopy( m );
  vtkSmartPointer<vtkMatrixToLinearTransform>  transform =
    vtkSmartPointer<vtkMatrixToLinearTransform>::New();
  transform->SetInput(affineMatrix);
  transform->Update();
  vtkSmartPointer<vtkTransformPolyDataFilter>  transformFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(cube->GetOutputPort());
  transformFilter->Update();

  vtkSmartPointer<vtkPoints>  parallelopipedPoints =
    vtkSmartPointer<vtkPoints>::New();
  parallelopipedPoints->DeepCopy(transformFilter->GetOutput()->GetPoints());

  transformFilter->SetInputConnection(append->GetOutputPort());
  transformFilter->Update();

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(transformFilter->GetOutputPort());

  vtkSmartPointer<vtkActor> maceActor =
    vtkSmartPointer<vtkActor>::New();
  maceActor->SetMapper(maceMapper);

  renderer->AddActor(maceActor);

  double parallelopipedPts[8][3];
  parallelopipedPoints->GetPoint(0, parallelopipedPts[0]);
  parallelopipedPoints->GetPoint(1, parallelopipedPts[1]);
  parallelopipedPoints->GetPoint(2, parallelopipedPts[3]);
  parallelopipedPoints->GetPoint(3, parallelopipedPts[2]);
  parallelopipedPoints->GetPoint(4, parallelopipedPts[4]);
  parallelopipedPoints->GetPoint(5, parallelopipedPts[5]);
  parallelopipedPoints->GetPoint(6, parallelopipedPts[7]);
  parallelopipedPoints->GetPoint(7, parallelopipedPts[6]);

  vtkSmartPointer<vtkParallelopipedWidget> widget =
    vtkSmartPointer<vtkParallelopipedWidget>::New();
  vtkSmartPointer<vtkParallelopipedRepresentation> rep =
    vtkSmartPointer<vtkParallelopipedRepresentation>::New();
  widget->SetRepresentation(rep);
  widget->SetInteractor( iren );
  rep->SetPlaceFactor( 0.5 );
  rep->PlaceWidget(parallelopipedPts);

  iren->Initialize();
  renWin->Render();

  widget->EnabledOn();

  vtkSmartPointer<vtkCubeAxesActor2D> axes =
    vtkSmartPointer<vtkCubeAxesActor2D>::New();
  axes ->SetInputConnection (transformFilter-> GetOutputPort());
  axes ->SetCamera (renderer-> GetActiveCamera());
  axes ->SetLabelFormat ("%6.1f");
  axes ->SetFlyModeToOuterEdges();
  axes ->SetFontFactor (0.8);
  renderer-> AddViewProp( axes );

  iren->Start();

  return EXIT_SUCCESS;
}
