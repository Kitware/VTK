/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCubeAxesOrientedBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkCubeAxesActor.h"
#include "vtkLight.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
int TestCubeAxesInnerGridAll( int argc, char * argv [] )
{
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1.0, 100.0);
  camera->SetFocalPoint(1.26612, -0.81045, 1.24353);
  camera->SetPosition(-5.66214, -2.58773, 11.243);

  vtkNew<vtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(8.3761, 4.94858, 4.12505);

  vtkNew<vtkRenderer> ren2;
  ren2->SetActiveCamera(camera.GetPointer());
  ren2->AddLight(light.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2.GetPointer());
  renWin->SetWindowName("Cube Axes");
  renWin->SetSize(600, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren2->SetBackground(0.1, 0.2, 0.4);

  double baseX[3] = {1,1,0};
  double baseY[3] = {0,1,1};
  double baseZ[3] = {1,0,1};

  vtkMath::Normalize(baseX);
  vtkMath::Normalize(baseY);
  vtkMath::Normalize(baseZ);

  vtkNew<vtkCubeAxesActor> axes;
  axes->SetUseOrientedBounds(1);
  axes->SetOrientedBounds(-1,1,-.5,0.5,0,4);
  axes->SetAxisBaseForX(baseX);
  axes->SetAxisBaseForY(baseY);
  axes->SetAxisBaseForZ(baseZ);
  axes->SetCamera(ren2->GetActiveCamera());
  axes->SetXLabelFormat( "%6.1f" );
  axes->SetYLabelFormat( "%6.1f" );
  axes->SetZLabelFormat( "%6.1f" );
  axes->SetScreenSize( 15. );
  axes->SetFlyModeToClosestTriad();
  axes->SetDrawXGridlines(1);
  axes->SetDrawYGridlines(1);
  axes->SetDrawZGridlines(1);
  axes->SetGridLineLocation(VTK_GRID_LINES_ALL);
  axes->SetCornerOffset( .0 );

  // Use red color for X axis
  axes->GetXAxesLinesProperty()->SetColor( 1., 0., 0. );
  axes->GetTitleTextProperty(0)->SetColor( 1., 0., 0. );
  axes->GetLabelTextProperty(0)->SetColor( .8, 0., 0. );

  // Use green color for Y axis
  axes->GetYAxesLinesProperty()->SetColor( 0., 1., 0. );
  axes->GetTitleTextProperty(1)->SetColor( 0., 1., 0. );
  axes->GetLabelTextProperty(1)->SetColor( 0., .8, 0. );

  ren2->AddViewProp( axes.GetPointer()) ;
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  cout << camera->GetFocalPoint()[0] << ", " << camera->GetFocalPoint()[1] << ", " << camera->GetFocalPoint()[2] << endl;
  cout << camera->GetPosition()[0] << ", " << camera->GetPosition()[1] << ", " << camera->GetPosition()[2] << endl;

  return !retVal;
}
