/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFinalColorWindowLevel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCamera.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// Create an 8x7 grid of render windows in a renderer and render a volume
// using various techniques for testing purposes
int TestFinalColorWindowLevel( int argc, char *argv[] )
{

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

  // Create a transfer function mapping scalar value to color (color)
  vtkColorTransferFunction *cTFun = vtkColorTransferFunction::New();
  cTFun->AddRGBPoint(   0, 1.0, 0.0, 0.0 );
  cTFun->AddRGBPoint(  64, 1.0, 1.0, 0.0 );
  cTFun->AddRGBPoint( 128, 0.0, 1.0, 0.0 );
  cTFun->AddRGBPoint( 192, 0.0, 1.0, 1.0 );
  cTFun->AddRGBPoint( 255, 0.0, 0.0, 1.0 );


  vtkVolumeProperty *property = vtkVolumeProperty::New();
  property->SetShade(0);
  property->SetAmbient(0.3);
  property->SetDiffuse(1.0);
  property->SetSpecular(0.2);
  property->SetSpecularPower(50.0);
  property->SetScalarOpacity(oTFun);
  property->SetColor( cTFun );
  property->SetInterpolationTypeToLinear();

  vtkFixedPointVolumeRayCastMapper *mapper = vtkFixedPointVolumeRayCastMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkVolume *volume = vtkVolume::New();
  volume->SetProperty(property);
  volume->SetMapper(mapper);
  ren->AddViewProp(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  mapper->SetFinalColorWindow(.5);
  mapper->SetFinalColorLevel(.75);

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
  cTFun->Delete();
  property->Delete();
  mapper->Delete();
  volume->Delete();

  ren->Delete();
  iren->Delete();
  renWin->Delete();

  return !retVal;
}



