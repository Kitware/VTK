/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCropping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGPUVolumeRayCastMapper.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkSLCReader.h"
#include "vtkStructuredPoints.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkCullerCollection.h"
#include "vtkCuller.h"
#include "vtkFrustumCoverageCuller.h"
#include "vtkVolumeTextureMapper3D.h"

//#define USE_TEXTURE_MAPPER_3D

int TestGPURayCastCropping(int argc, char *argv[])
{
  // Create the standard renderer, render window, and interactor.
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  ren1->Delete();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  iren->SetDesiredUpdateRate(3);

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  vtkSLCReader *reader=vtkSLCReader::New();
  char *cfname=vtkTestUtilities::ExpandDataFileName(argc,argv,"Data/sphere.slc");
  reader->SetFileName(cfname);
  delete[] cfname;

  // Create transfer mapping scalar value to opacity.
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
  opacityTransferFunction->AddPoint(0.0,  0.0);
  opacityTransferFunction->AddPoint(30.0, 0.0);
  opacityTransferFunction->AddPoint(80.0, 0.5);
  opacityTransferFunction->AddPoint(255.0,0.5);

  // Create transfer mapping scalar value to color.
  vtkColorTransferFunction *colorTransferFunction
    = vtkColorTransferFunction::New();
  colorTransferFunction->AddRGBPoint(  0.0,0.0,0.0,0.0);
  colorTransferFunction->AddRGBPoint( 64.0,1.0,0.0,0.0);
  colorTransferFunction->AddRGBPoint(128.0,0.0,0.0,1.0);
  colorTransferFunction->AddRGBPoint(192.0,0.0,1.0,0.0);
  colorTransferFunction->AddRGBPoint(255.0,0.0,0.2,0.0);

  // The property describes how the data will look.
  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
  volumeProperty->SetColor(colorTransferFunction);
  colorTransferFunction->Delete();
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  opacityTransferFunction->Delete();
  volumeProperty->ShadeOn(); //
  volumeProperty->SetInterpolationTypeToLinear();


  ren1->SetBackground(0.1,0.2,0.4);
  renWin->SetSize(600, 300);
  renWin->Render();
  ren1->ResetCamera();
  renWin->Render();

  vtkGPUVolumeRayCastMapper *volumeMapper[2][4];
  int i=0;
  while(i<2)
    {
    int j=0;
    while(j<4)
      {
      volumeMapper[i][j]= vtkGPUVolumeRayCastMapper::New();
      volumeMapper[i][j]->SetInputConnection(reader->GetOutputPort());
      volumeMapper[i][j]->SetSampleDistance(0.25);
      volumeMapper[i][j]->CroppingOn();
      volumeMapper[i][j]->SetCroppingRegionPlanes(17,33,17,33,17,33);

      vtkVolume *volume=vtkVolume::New();
      volume->SetMapper(volumeMapper[i][j]);
      volumeMapper[i][j]->Delete();
      volume->SetProperty(volumeProperty);

      vtkTransform *userMatrix=vtkTransform::New();
      userMatrix->PostMultiply();
      userMatrix->Identity();
      userMatrix->Translate(-25,-25,-25);

      if(i==0)
        {
        userMatrix->RotateX(j*90+20);
        userMatrix->RotateY(20);
        }
      else
        {
        userMatrix->RotateX(20);
        userMatrix->RotateY(j*90+20);
        }

      userMatrix->Translate(j*55+25,i*55+25,0);
      volume->SetUserTransform(userMatrix);
      userMatrix->Delete();
//      if(i==1 && j==0)
//        {
        ren1->AddViewProp(volume);
//        }
      volume->Delete();
      ++j;
      }
    ++i;
    }
  reader->Delete();
  volumeProperty->Delete();

  int I=1;
  int J=0;

  volumeMapper[0][0]->SetCroppingRegionFlagsToSubVolume();
  volumeMapper[0][1]->SetCroppingRegionFlagsToCross();
  volumeMapper[0][2]->SetCroppingRegionFlagsToInvertedCross();
  volumeMapper[0][3]->SetCroppingRegionFlags(24600);

  //
  volumeMapper[1][0]->SetCroppingRegionFlagsToFence();
  volumeMapper[1][1]->SetCroppingRegionFlagsToInvertedFence();
  volumeMapper[1][2]->SetCroppingRegionFlags(1);
  volumeMapper[1][3]->SetCroppingRegionFlags(67117057);
  ren1->GetCullers()->InitTraversal();
  vtkCuller *culler=ren1->GetCullers()->GetNextItem();

  vtkFrustumCoverageCuller *fc=vtkFrustumCoverageCuller::SafeDownCast(culler);
  if(fc!=0)
    {
    fc->SetSortingStyleToBackToFront();
    }
  else
    {
    cout<<"culler is not a vtkFrustumCoverageCuller"<<endl;
    }

  // First test if mapper is supported

  int valid=volumeMapper[I][J]->IsRenderSupported(renWin,volumeProperty);

  int retVal;
  if(valid)
    {
    ren1->ResetCamera();
    ren1->GetActiveCamera()->Zoom(3.0);
//  ren1->GetActiveCamera()->SetParallelProjection(1);
    renWin->Render();

    retVal = vtkTesting::Test(argc, argv, renWin, 75);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }
    }
  else
    {
    retVal=vtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
    }

  // Clean up.
  iren->Delete();

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
