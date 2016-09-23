/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextureRGBADepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the rendering of an vtkActor with a translucent texture
// with depth peeling.

#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTexture.h"
#include "vtkImageData.h"
#include "vtkPNGReader.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestTextureRGBADepthPeeling(int argc, char *argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/textureRGBA.png");

  vtkPNGReader *PNGReader = vtkPNGReader::New();
  PNGReader->SetFileName(fname);
  PNGReader->Update();

  vtkTexture *texture = vtkTexture::New();
  texture->SetInputConnection(PNGReader->GetOutputPort());
  PNGReader->Delete();
  texture->InterpolateOn();

  vtkPlaneSource *planeSource = vtkPlaneSource::New();
  planeSource->Update();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(planeSource->GetOutputPort());
  planeSource->Delete();

  vtkActor *actor = vtkActor::New();
  actor->SetTexture(texture);
  texture->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renderer->AddActor(actor);
  actor->Delete();
  renderer->SetBackground(0.5,0.7,0.7);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);

  vtkRenderWindowInteractor *interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400,400);
  renWin->Render();
  if(renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout<<"depth peeling was used"<<endl;
  }
  else
  {
    cout<<"depth peeling was not used (alpha blending instead)"<<endl;
  }

  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  renderer->Delete();
  renWin->Delete();
  interactor->Delete();
  delete [] fname;

  return !retVal;
}
