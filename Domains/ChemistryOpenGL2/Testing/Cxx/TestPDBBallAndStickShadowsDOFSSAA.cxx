/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkOpenGLMoleculeMapper.h"
#include "vtkOpenGLSphereMapper.h"
#include "vtkPDBReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"

#include "vtkCameraPass.h"
#include "vtkDepthOfFieldPass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderStepsPass.h"
#include "vtkSSAAPass.h"
#include "vtkSequencePass.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkShadowMapPass.h"

#include "vtkLookupTable.h"
#include "vtkPeriodicTable.h"

#include "vtkCamera.h"
#include "vtkTimerLog.h"

int TestPDBBallAndStickShadowsDOFSSAA(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/2LYZ.pdb");

  // read protein from pdb
  vtkNew<vtkPDBReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  vtkNew<vtkOpenGLMoleculeMapper> molmapper;
  molmapper->SetInputConnection(reader->GetOutputPort(1));
  molmapper->SetRenderBonds(false);
  molmapper->SetAtomicRadiusType(vtkMoleculeMapper::VDWRadius);
  molmapper->SetAtomicRadiusScaleFactor(0.9);

  // get the default lookup table and desaturate it to be
  // more pleasing
  vtkNew<vtkPeriodicTable> pt;
  vtkNew<vtkLookupTable> lut;
  pt->GetDefaultLUT(lut);
  const unsigned short numColors = lut->GetNumberOfColors();
  double rgb[4];
  for (vtkIdType i = 0; static_cast<unsigned int>(i) < numColors; ++i)
  {
    lut->GetTableValue(i, rgb);
    lut->SetTableValue(i, 0.45 + rgb[0] * 0.55, 0.45 + rgb[1] * 0.55, 0.45 + rgb[2] * 0.55);
  }
  molmapper->SetLookupTable(lut);

  vtkNew<vtkActor> actor;
  actor->SetMapper(molmapper);
  actor->GetProperty()->SetAmbient(0.3);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetSpecular(0.4);
  actor->GetProperty()->SetSpecularPower(40);

  vtkShaderProperty* sp = actor->GetShaderProperty();

  // we override the default shader very slightly so that
  // the ambient color component is scaled off the diffuse
  sp->AddFragmentShaderReplacement("//VTK::Color::Impl",
    true,                                   // before the standard replacements
    "//VTK::Color::Impl\n"                  // we still want the default
    "  ambientColor = diffuseColor*0.2;\n", // but we add this
    false                                   // only do it once
  );

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);
  ren->GetActiveCamera()->SetFocalDisk(ren->GetActiveCamera()->GetDistance() * 0.05);
  ren->SetBackground2(0.2, 0.2, 0.3);
  ren->SetBackground(0.1, 0.1, 0.15);
  ren->GradientBackgroundOn();
  win->SetSize(600, 600);

  // add a plane
  vtkNew<vtkPlaneSource> plane;
  const double* bounds = molmapper->GetBounds();
  plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
  plane->SetPoint1(bounds[1], bounds[2], bounds[4]);
  plane->SetPoint2(bounds[0], bounds[2], bounds[5]);
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  ren->AddActor(planeActor);

  vtkNew<vtkLight> light1;
  light1->SetFocalPoint(0, 0, 0);
  light1->SetPosition(-0.3, 0.9, 0.3);
  light1->SetIntensity(0.5);
  light1->SetShadowAttenuation(0.6);
  ren->AddLight(light1);

  vtkNew<vtkLight> light2;
  light2->SetFocalPoint(0, 0, 0);
  light2->SetPosition(0.3, 0.9, 0.3);
  light2->SetIntensity(0.5);
  light2->SetShadowAttenuation(0.6);
  ren->AddLight(light2);

  vtkNew<vtkShadowMapPass> shadows;

  vtkNew<vtkSequencePass> seq;
  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(shadows->GetShadowMapBakerPass());
  passes->AddItem(shadows);
  seq->SetPasses(passes);

  vtkNew<vtkCameraPass> cameraP;
  cameraP->SetDelegatePass(seq);

  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;

  vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(ren);

  // finally add the DOF passs
  vtkNew<vtkDepthOfFieldPass> dofp;
  // dofp->AutomaticFocalDistanceOff();
  dofp->SetDelegatePass(cameraP);

  // finally blur the resulting image
  // The blur delegates rendering the unblured image
  // to the basicPasses
  vtkNew<vtkSSAAPass> ssaa;
  ssaa->SetDelegatePass(dofp);

  // tell the renderer to use our render pass pipeline
  glrenderer->SetPass(ssaa);
  //  glrenderer->SetPass(dofp);
  //  glrenderer->SetPass(cameraP);

  // tell the renderer to use our render pass pipeline
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  // this example will suck the life out of your fragment shaders
  // until we provide some optimizations. The DOF pass is a brute force
  // approach which takes 81 tlookups per pixel. Combine that with
  // 5x SSAA and you have around 400 texture lookups per final pixel
  // we ust have everything on here to make sure it all works together.
  // We will likely want to provide a second quality setting for the DOF
  // pass that is designed to work with SSAA where we know we can tolerate more
  // DOF noise as the SSAA will be averaging it anyhow.
  int numRenders = 5;
  timer->StartTimer();
  for (int i = 0; i < numRenders; ++i)
  {
    ren->GetActiveCamera()->Azimuth(85.0 / numRenders);
    ren->GetActiveCamera()->Elevation(85.0 / numRenders);
    win->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;

  ren->GetActiveCamera()->SetPosition(0, 0, 1);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Elevation(40.0);
  ren->GetActiveCamera()->Zoom(2.0);

  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();

  int retVal = vtkRegressionTestImage(win);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
