/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkProperty.h"

#include "vtkRegressionTestImage.h"

int main( int argc, char *argv[] )
{
  // Create the renderers, render window, and interactor
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  vtkRenderer *ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  
  renWin->SetSize( 200, 200 );
  
  vtkSphereSource *sphereSource = vtkSphereSource::New();
  sphereSource->SetThetaResolution(30);
  sphereSource->SetPhiResolution(30);
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput( sphereSource->GetOutput() );
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  ren->AddProp( actor );
  renWin->Render();
  
  actor->AddPosition( 0.3, 0.3, 0.0 );
  renWin->Render();
  vtkUnsignedCharArray *ucCharArray = vtkUnsignedCharArray::New();
  renWin->GetPixelData( 0, 0, 199, 199, 1, ucCharArray );  
  
  float *fdata = renWin->GetZbufferData( 10, 10, 10, 10 );
  delete [] fdata;
  
  vtkFloatArray *floatArray = vtkFloatArray::New();
  renWin->GetZbufferData( 30, 30, 199, 199, floatArray );

  ren->RemoveProp( actor );
  renWin->Render();
  renWin->Render();
  renWin->EraseOff();
  
  renWin->SetZbufferData(  0, 0,  169, 169, floatArray );
  
  ren->AddProp( actor );
  actor->GetProperty()->SetColor(1,0,1);
  renWin->Render();

  actor->GetProperty()->SetColor(0,1,0);
  actor->AddPosition( -0.1, -0.1, 0.0 );
  renWin->Render();
  renWin->GetRGBAPixelData( 120, 120, 174, 174, 1, floatArray );

  renWin->EraseOn();
  renWin->Render();
  
  ren->RemoveProp( actor );
  renWin->Render();
  
  renWin->SetPixelData( 0, 0, 199, 199, ucCharArray, 1 );
  
  unsigned char *checks = new unsigned char [200*200*3];
  
  int i, j;
  
  for ( i = 0; i < 200; i++ )
    for ( j = 0; j < 200; j++ )
      {
      checks[i*200*3 + j*3] = 0;
      checks[i*200*3 + j*3 + 1] = i;
      checks[i*200*3 + j*3 + 2] = j;      
      }
  
  renWin->SetPixelData( 0, 0, 199, 199, checks, 1 );
  
  renWin->SetRGBAPixelData(  0,  0,  54,  54, floatArray, 1, 0 );

  renWin->SetRGBAPixelData( 20, 20,  74,  74, floatArray, 1, 0 );
  
  renWin->SetRGBAPixelData( 40, 40,  94,  94, floatArray, 1, 0 );
  
  renWin->SetRGBAPixelData( 60, 60, 114, 114, floatArray, 1, 0 );
    
  fdata = renWin->GetRGBAPixelData( 0, 0, 99, 99, 1 );
  renWin->SetRGBAPixelData( 100, 100, 199, 199, fdata, 1, 0 );
  renWin->ReleaseRGBAPixelData( fdata );

  vtkUnsignedCharArray *ucArray = vtkUnsignedCharArray::New();
  renWin->GetRGBACharPixelData( 20, 150, 40, 170, 1, ucArray );
  renWin->SetRGBACharPixelData( 160, 31, 180, 51, ucArray, 1, 0 );
  
  renWin->SwapBuffersOff();
  
  int retVal = vtkRegressionTestImage( renWin );

  // Interact with the data at 3 frames per second
  iren->SetDesiredUpdateRate(3.0);
  iren->SetStillUpdateRate(0.001);

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  floatArray->Delete();
  ucArray->Delete();
  sphereSource->Delete();
  actor->Delete();
  mapper->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  
  return !retVal;
}


