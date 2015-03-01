/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCornerAnnotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCornerAnnotation.h"
#include "vtkRegressionTestImage.h"

int TestCornerAnnotation( int argc, char * argv [] )
{
  vtkSmartPointer<vtkSphereSource> sphereSource =
      vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->Update();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphereSource->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Visualize
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
      vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->AddActor(actor);

   // Annotate the image with window/level and mouse over pixel information
  vtkSmartPointer<vtkCornerAnnotation> cornerAnnotation =
      vtkSmartPointer<vtkCornerAnnotation>::New();
  cornerAnnotation->SetLinearFontScaleFactor( 2 );
  cornerAnnotation->SetNonlinearFontScaleFactor( 1 );
  cornerAnnotation->SetMaximumFontSize( 20 );
  cornerAnnotation->SetText( vtkCornerAnnotation::LowerLeft, "lower left" );
  cornerAnnotation->SetText( vtkCornerAnnotation::LowerRight, "lower right" );
  cornerAnnotation->SetText( vtkCornerAnnotation::UpperLeft, "upper left" );
  cornerAnnotation->SetText( vtkCornerAnnotation::UpperRight, "upper right" );
  cornerAnnotation->GetTextProperty()->SetColor( 1,0,0);

  renderer->AddViewProp(cornerAnnotation);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renderWindowInteractor->Start();
    }

  return !retVal;
}
