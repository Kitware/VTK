/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTDx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the 3DConnexion device interface with earth navigation
// interactor style.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"

#include "vtkPolyDataMapper.h"

#include "vtkTexturedSphereSource.h"
#include "vtkPNMReader.h"
#include "vtkTexture.h"
#include "vtkEarthSource.h"

#include "vtkProperty.h"
#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"

#include "vtkInteractorStyle.h"
#include "vtkTDxInteractorStyleCamera.h"
#include "vtkTDxInteractorStyleGeo.h"
#include "vtkTDxInteractorStyleSettings.h"
#include "vtkInteractorStyleTrackballCamera.h"

const double angleSensitivity=0.02;
const double translationSensitivity=0.001;

int TestTDxGeo(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetUseTDx(true);
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  // textured earth
  vtkActor *earthActor=vtkActor::New();

  char *fname=vtkTestUtilities::ExpandDataFileName(argc,argv,"Data/earth.ppm");
  vtkPNMReader *reader=vtkPNMReader::New();
  reader->SetFileName(fname);
  delete[] fname;

  vtkTexture *earthTexture=vtkTexture::New();
  earthTexture->SetInputConnection(reader->GetOutputPort());
  reader->Delete();
  earthTexture->SetInterpolate(true);
  earthActor->SetTexture(earthTexture);
  earthTexture->Delete();

  vtkPolyDataMapper *earthMapper=vtkPolyDataMapper::New();
  earthActor->SetMapper(earthMapper);
  earthMapper->Delete();

  vtkTexturedSphereSource *tss=vtkTexturedSphereSource::New();
  tss->SetThetaResolution(36); // longitudes
  tss->SetPhiResolution(18); // latitudes

  earthMapper->SetInputConnection(tss->GetOutputPort());
  tss->Delete();

  // earth contour
  vtkEarthSource *es=vtkEarthSource::New();
  es->SetRadius(0.501);
  es->SetOnRatio(2);
  vtkPolyDataMapper *earth2Mapper=vtkPolyDataMapper::New();
  earth2Mapper->SetInputConnection(es->GetOutputPort());
  es->Delete();
  vtkActor *earth2Actor=vtkActor::New();
  earth2Actor->SetMapper(earth2Mapper);
  earth2Mapper->Delete();

  renderer->AddActor(earthActor);
  earthActor->Delete();
  renderer->AddActor(earth2Actor);
  earth2Actor->Delete();

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(200,200);

  renWin->Render();

  renderer->ResetCamera();
  renWin->Render();

  vtkInteractorStyleTrackballCamera *s=
    vtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(s);
  s->Delete();

  vtkTDxInteractorStyleGeo *t=vtkTDxInteractorStyleGeo::New();

  s->SetTDxStyle(t);
  t->Delete();

  t->GetSettings()->SetAngleSensitivity(angleSensitivity);
  t->GetSettings()->SetTranslationXSensitivity(translationSensitivity);
  t->GetSettings()->SetTranslationYSensitivity(translationSensitivity);
  t->GetSettings()->SetTranslationZSensitivity(translationSensitivity);

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  iren->Delete();

  return !retVal;
}
