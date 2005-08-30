/*=========================================================================

  Program:   Visualization Toolkit
  Module:    volProt.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCamera.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeRayCastIsosurfaceFunction.h"
#include "vtkVolumeRayCastMIPFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolumeTextureMapper2D.h"
#include "vtkColorTransferFunction.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// Create an 8x7 grid of render windows in a renderer and render a volume
// using various techniques for testing purposes
int volProt( int argc, char *argv[] )
{
  int i, j, k, l;
  
  // Create the renderers, render window, and interactor
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  vtkRenderer *ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  
  // Read the data from a vtk file
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  vtkStructuredPointsReader *reader = vtkStructuredPointsReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;
  
  // Create a transfer function mapping scalar value to opacity
  vtkPiecewiseFunction *oTFun = vtkPiecewiseFunction::New();
  oTFun->AddSegment(10, 0.0, 255, 0.3);

  vtkPiecewiseFunction *oTFun2 = vtkPiecewiseFunction::New();
  oTFun2->AddSegment(  0, 0.0, 128, 1.0);
  oTFun2->AddSegment(128, 1.0, 255, 0.0);

  // Create a transfer function mapping scalar value to color (grey)
  vtkPiecewiseFunction *gTFun = vtkPiecewiseFunction::New();
  gTFun->AddSegment(0, 1.0, 255, 1.0);
    
  // Create a transfer function mapping scalar value to color (color)
  vtkColorTransferFunction *cTFun = vtkColorTransferFunction::New();
  cTFun->AddRGBPoint(   0, 1.0, 0.0, 0.0 );
  cTFun->AddRGBPoint(  64, 1.0, 1.0, 0.0 );
  cTFun->AddRGBPoint( 128, 0.0, 1.0, 0.0 );
  cTFun->AddRGBPoint( 192, 0.0, 1.0, 1.0 );
  cTFun->AddRGBPoint( 255, 0.0, 0.0, 1.0 );

  // Create a transfer function mapping magnitude of gradient to opacity
  vtkPiecewiseFunction *goTFun = vtkPiecewiseFunction::New();
  goTFun->AddPoint(   0, 0.0 );
  goTFun->AddPoint(  30, 0.0 );
  goTFun->AddPoint(  40, 1.0 );
  goTFun->AddPoint( 255, 1.0 );

  // Create a set of properties with varying options
  vtkVolumeProperty *prop[16];
  int index = 0;
  for ( l = 0; l < 2; l++ )
    {
    for ( k = 0; k < 2; k++ )
      {
      for ( j = 0; j < 2; j++ )
        {
        for ( i = 0; i < 2; i++ )
          {
          prop[index] = vtkVolumeProperty::New();
          prop[index]->SetShade(k);
          prop[index]->SetAmbient(0.3);
          prop[index]->SetDiffuse(1.0);
          prop[index]->SetSpecular(0.2);
          prop[index]->SetSpecularPower(50.0);
          prop[index]->SetScalarOpacity(oTFun);
          
          if ( l )
            {
            prop[index]->SetGradientOpacity( goTFun );
            }
          
          if ( j )
            {
            prop[index]->SetColor( cTFun );
            }
          else
            {
            prop[index]->SetColor( gTFun );
            }
          
          if ( i )
            {
            prop[index]->SetInterpolationTypeToNearest();
            }
          else
            {
            prop[index]->SetInterpolationTypeToLinear();
            }
          
          index++;
          }
        }
      }
    }

  // Create a set of properties for mip
  vtkVolumeProperty *mipprop[4];
  index = 0;
  for ( j = 0; j < 2; j++ )
    {
    for ( i = 0; i < 2; i++ )
      {
      mipprop[index] = vtkVolumeProperty::New();
      mipprop[index]->SetScalarOpacity(oTFun2);          
          
      if ( j )
        {
        mipprop[index]->SetColor( cTFun );
        }
      else
        {
        mipprop[index]->SetColor( gTFun );
        }
      
      if ( i )
        {
        mipprop[index]->SetInterpolationTypeToNearest();
        }
      else
        {
        mipprop[index]->SetInterpolationTypeToLinear();
        }
          
      index++;
      }
    }

  

  // Create compositing ray functions
  vtkVolumeRayCastCompositeFunction *compositeFunction1 = 
    vtkVolumeRayCastCompositeFunction::New();
  compositeFunction1->SetCompositeMethodToInterpolateFirst();

  vtkVolumeRayCastCompositeFunction *compositeFunction2 = 
    vtkVolumeRayCastCompositeFunction::New();
  compositeFunction2->SetCompositeMethodToClassifyFirst();

  
  // Create mip ray functions
  vtkVolumeRayCastMIPFunction *MIPFunction1 = 
    vtkVolumeRayCastMIPFunction::New();
  MIPFunction1->SetMaximizeMethodToScalarValue();

  vtkVolumeRayCastMIPFunction *MIPFunction2 = 
    vtkVolumeRayCastMIPFunction::New();
  MIPFunction2->SetMaximizeMethodToOpacity();

  // Create an isosurface ray function
  vtkVolumeRayCastIsosurfaceFunction *isosurfaceFunction = 
    vtkVolumeRayCastIsosurfaceFunction::New();
  isosurfaceFunction->SetIsoValue(80);

  vtkFiniteDifferenceGradientEstimator *gradest = 
    vtkFiniteDifferenceGradientEstimator::New();
  
  // Create 56 volumes
  vtkVolume *volume[56];
  index = 0;
  for ( j = 0; j < 7; j++ )
    {
    for ( i = 0; i < 8; i++ )
      {
      volume[index] = vtkVolume::New();
      volume[index]->AddPosition( i*70, j*70, 0 );
      ren->AddViewProp(volume[index]);
      index++;
      }
    }
  
  
  // Create 48 ray cast mappers - 32 composite, 8 mip, 8 isosurface
  vtkVolumeRayCastMapper *raycastMapper[48];
  for ( i = 0; i < 48; i++ )
    {
    raycastMapper[i] = vtkVolumeRayCastMapper::New();
    raycastMapper[i]->SetInputConnection(reader->GetOutputPort());
    raycastMapper[i]->SetGradientEstimator(gradest);
    volume[i]->SetMapper( raycastMapper[i] );

    if ( i < 16 )
      {
      volume[i]->SetProperty( prop[i] );
      raycastMapper[i]->SetVolumeRayCastFunction( compositeFunction1 );
      }
    else if ( i < 32 )
      {
      volume[i]->SetProperty( prop[i-16] );
      raycastMapper[i]->SetVolumeRayCastFunction( compositeFunction2 );
      }
    else
      {
      if ( i < 36 )
        {
        raycastMapper[i]->SetVolumeRayCastFunction( MIPFunction1 );  
        volume[i]->SetProperty( mipprop[i-32] );
        }
      else if ( i < 40 )
        {
        raycastMapper[i]->SetVolumeRayCastFunction( MIPFunction2 );  
        volume[i]->SetProperty( mipprop[i-36] );
        }
      else 
        {
        raycastMapper[i]->SetVolumeRayCastFunction( isosurfaceFunction );  
        volume[i]->SetProperty( prop[i-40] );
        }
      }
    }

  // Create 8 texture mappers
  vtkVolumeTextureMapper2D *textureMapper[8];
  for ( i = 0; i < 8; i++ )
    {
    textureMapper[i] = vtkVolumeTextureMapper2D::New();
    textureMapper[i]->SetInputConnection( reader->GetOutputPort() );    
    volume[i+48]->SetMapper( textureMapper[i] );
    volume[i+48]->SetProperty( prop[i*2] );
    }
  

  renWin->SetSize(400,350);

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);
  
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold( renWin, 70 );

  // Interact with the data at 3 frames per second
  iren->SetDesiredUpdateRate(3.0);
  iren->SetStillUpdateRate(0.001);

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  
  // Clean up
  reader->Delete();
  oTFun->Delete();
  oTFun2->Delete();
  gTFun->Delete();
  cTFun->Delete();
  goTFun->Delete();
  for ( i = 0; i < 16; i++ )
    {
    prop[i]->Delete();
    }
  for ( i = 0; i < 4; i++ )
    {
    mipprop[i]->Delete();
    }
  compositeFunction1->Delete();
  compositeFunction2->Delete();
  isosurfaceFunction->Delete();
  MIPFunction1->Delete();
  MIPFunction2->Delete();
  for ( i = 0; i < 56; i++ )
    {
    volume[i]->Delete();
    }
  gradest->Delete();
  for ( i = 0; i < 48; i++ )
    {
    raycastMapper[i]->Delete();
    }
  for ( i = 0; i < 8; i++ )
    {
    textureMapper[i]->Delete();
    }
  ren->Delete();
  iren->Delete();
  renWin->Delete();
  
  return !retVal;
}



