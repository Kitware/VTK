/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DGeometryMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Joachim Pouderoux, Kitware 2013
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkBitArray.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkHyperTreeGrid.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"
#include <vtkObjectFactory.h>

// Define interaction style
class KeyPressInteractorStyle : public vtkInteractorStyleTrackballCamera
{
  public:
    static KeyPressInteractorStyle* New();
    vtkTypeMacro(KeyPressInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnKeyPress()
    {
      // Get the keypress
      vtkRenderWindowInteractor *rwi = this->Interactor;
      std::string key = rwi->GetKeySym();

      // Handle a "normal" key
      if(key == "a")
        {
        double *pos = this->Renderer->GetActiveCamera()->GetPosition();
        double *focal = this->Renderer->GetActiveCamera()->GetFocalPoint();
        double *clip = this->Renderer->GetActiveCamera()->GetClippingRange();
        double *up = this->Renderer->GetActiveCamera()->GetViewUp();
        cout << "----" << endl;
        cout << "Camera position " << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;
        cout << "Camera focalpoint " << focal[0] << ", " << focal[1] << ", " << focal[2] << endl;
        cout << "Camera viewup " << up[0] << ", " << up[1] << ", " << up[2] << endl;
        cout << "Camera range " << clip[0] << ", " << clip[1] << endl;
        }

      // Forward events
      vtkInteractorStyleTrackballCamera::OnKeyPress();
    }

    vtkRenderer* Renderer;
};
vtkStandardNewMacro(KeyPressInteractorStyle);

int TestHyperTreeGridTernary3DGeometryLargeMaterialBits( int argc, char* argv[] )
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaximumLevel( 6 );
  htGrid->SetGridSize( 100, 100, 20 );
  htGrid->SetGridScale( 1.5, 1., .7 );
  htGrid->SetDimension( 3 );
  htGrid->SetBranchFactor( 3 );
  htGrid->UseMaterialMaskOn();
  const std::string descriptor = ".RR _R. _RR ..R _.R .R_|" // Level 0 refinement
   "R.......................... ........................... ........................... .............R............. ....RR.RR........R......... .....RRRR.....R.RR......... ........................... ...........................|........................... ........................... ........................... ...RR.RR.......RR.......... ........................... RR......................... ........................... ........................... ........................... ........................... ........................... ........................... ........................... ............RRR............|........................... ........................... .......RR.................. ........................... ........................... ........................... ........................... ........................... ........................... ........................... ...........................|........................... ...........................";
  const std::string materialMask = // Level 0 materials are not needed, visible cells are described with LevelZeroMaterialIndex
   "111111111111111111111111111 000000000100110111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 000110011100000100100010100|000001011011111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111001111111101111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111|000000000111100100111100100 000000000111001001111001001 000000111100100111111111111 000000111001001111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 110110110100111110111000000|111111111111111111111111111 111111111111111111111111111";
  vtkIdType zeroArray[] = { 0, 1, 2, 4, 5, 7, 8, 9, 30, 29*30+1, 30*30, 30*30*19, 30*30*20-2, 30*30*20-1 };
  vtkNew<vtkIdTypeArray> zero;
  zero->SetArray( zeroArray, sizeof(zeroArray) / sizeof(vtkIdType), 1, 0 );
  htGrid->SetLevelZeroMaterialIndex( zero.GetPointer() );
  vtkBitArray* desc = htGrid->ConvertDescriptorStringToBitArray( descriptor );
  htGrid->SetDescriptorBits(desc);
  desc->Delete();
  vtkBitArray* mat = htGrid->ConvertMaterialMaskStringToBitArray( materialMask );
  htGrid->SetMaterialMaskBits(mat);
  mat->Delete();
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
  cout << "Tree created in " << timer->GetElapsedTime() << "s" << endl;
  htGrid->GetOutput()->GetNumberOfCells();

  timer->StartTimer();
  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection( htGrid->GetOutputPort() );
  geometry->Update();
  vtkPolyData* pd = geometry->GetOutput();
  timer->StopTimer();
  cout << "Geometry computed in " << timer->GetElapsedTime() << "s" <<  endl;

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection( geometry->GetOutputPort() );
  mapper1->SetScalarRange( pd->GetCellData()->GetScalars()->GetRange() );
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection( geometry->GetOutputPort() );
  mapper2->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper( mapper1.GetPointer() );
  vtkNew<vtkActor> actor2;
  actor2->SetMapper( mapper2.GetPointer() );
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .7, .7, .7 );

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground( 1., 1., 1. );
  renderer->AddActor( actor1.GetPointer() );
  renderer->AddActor( actor2.GetPointer() );

  // Camera
  renderer->GetActiveCamera()->SetFocalPoint( 39.47, 14.97, 5.83 );
  renderer->GetActiveCamera()->SetPosition( -34.83, -20.41, -27.78 );
  renderer->GetActiveCamera()->SetViewUp( -0.257301, 0.959041, -0.118477 );
  renderer->GetActiveCamera()->SetClippingRange( 0.314716, 314.716 );

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer( renderer.GetPointer() );
  renWin->SetSize( 400, 400 );
  renWin->SetMultiSamples( 0 );

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow( renWin.GetPointer() );
  vtkNew<KeyPressInteractorStyle> style;
  style->Renderer = renderer.GetPointer();
  iren->SetInteractorStyle( style.GetPointer() );

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold( renWin.GetPointer(), 30 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    iren->Start();
    }

  return !retVal;
}
