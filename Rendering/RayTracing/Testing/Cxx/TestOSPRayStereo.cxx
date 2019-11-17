/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayStereo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test verifies that OSPRay can rendering in stereo modes.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkMatrix4x4.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

int TestOSPRayStereo(int argc, char* argv[])
{
  bool useGL = false;
  int stereoType = VTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp("-GL", argv[i]))
    {
      cerr << "GL" << endl;
      useGL = true;
    }
    if (!strcmp("VTK_STEREO_CRYSTAL_EYES", argv[i]))
    {
      cerr << "VTK_STEREO_CRYSTAL_EYES" << endl;
      stereoType = VTK_STEREO_CRYSTAL_EYES;
    }
    if (!strcmp("VTK_STEREO_INTERLACED", argv[i]))
    {
      cerr << "VTK_STEREO_INTERLACED" << endl;
      stereoType = VTK_STEREO_INTERLACED;
    }
    if (!strcmp("VTK_STEREO_RED_BLUE", argv[i]))
    {
      cerr << "VTK_STEREO_RED_BLUE" << endl;
      stereoType = VTK_STEREO_RED_BLUE;
    }
    if (!strcmp("VTK_STEREO_LEFT", argv[i]))
    {
      cerr << "VTK_STEREO_LEFT" << endl;
      stereoType = VTK_STEREO_LEFT;
    }
    if (!strcmp("VTK_STEREO_RIGHT", argv[i]))
    {
      cerr << "VTK_STEREO_RIGHT" << endl;
      stereoType = VTK_STEREO_RIGHT;
    }
    if (!strcmp("VTK_STEREO_DRESDEN", argv[i]))
    {
      cerr << "VTK_STEREO_DRESDEN" << endl;
      stereoType = VTK_STEREO_DRESDEN;
    }
    if (!strcmp("VTK_STEREO_ANAGLYPH", argv[i]))
    {
      cerr << "VTK_STEREO_ANAGLYPH" << endl;
      stereoType = VTK_STEREO_ANAGLYPH;
    }
    if (!strcmp("VTK_STEREO_CHECKERBOARD", argv[i]))
    {
      cerr << "VTK_STEREO_CHECKERBOARD" << endl;
      stereoType = VTK_STEREO_CHECKERBOARD;
    }
    if (!strcmp("VTK_STEREO_SPLITVIEWPORT_HORIZONTAL", argv[i]))
    {
      cerr << "VTK_STEREO_SPLITVIEWPORT_HORIZONTAL" << endl;
      stereoType = VTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
    }
    if (!strcmp("VTK_STEREO_FAKE", argv[i]))
    {
      cerr << "VTK_STEREO_FAKE" << endl;
      stereoType = VTK_STEREO_FAKE;
    }
    if (!strcmp("NOSTEREO", argv[i]))
    {
      cerr << "NO STEREO" << endl;
      stereoType = 0;
    }
  }
  double bottomLeft[3] = { -1.0, -1.0, -10.0 };
  double bottomRight[3] = { 1.0, -1.0, -10.0 };
  double topRight[3] = { 1.0, 1.0, -10.0 };

  VTK_CREATE(vtkSphereSource, sphere1);
  sphere1->SetCenter(0.2, 0.0, -7.0);
  sphere1->SetRadius(0.5);
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);

  VTK_CREATE(vtkPolyDataMapper, mapper1);
  mapper1->SetInputConnection(sphere1->GetOutputPort());

  VTK_CREATE(vtkActor, actor1);
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(0.8, 0.8, 0.0);

  VTK_CREATE(vtkConeSource, cone1);
  cone1->SetCenter(0.0, 0.0, -6.0);
  cone1->SetResolution(100);

  VTK_CREATE(vtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(cone1->GetOutputPort());

  VTK_CREATE(vtkActor, actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetAmbient(0.1);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->SetAmbient(1.0, 1.0, 1.0);

  vtkSmartPointer<vtkOSPRayPass> osprayPass = vtkSmartPointer<vtkOSPRayPass>::New();
  if (!useGL)
  {
    renderer->SetPass(osprayPass);

    for (int i = 0; i < argc; ++i)
    {
      if (!strcmp(argv[i], "--OptiX"))
      {
        vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
        break;
      }
    }
  }

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  renwin->SetSize(400, 400);
  if (stereoType)
  {
    if (stereoType == VTK_STEREO_CRYSTAL_EYES)
    {
      renwin->StereoCapableWindowOn();
    }
    renwin->SetStereoType(stereoType);
    renwin->SetStereoRender(1);
  }
  else
  {
    cerr << "NOT STEREO" << endl;
    renwin->SetStereoRender(0);
  }
  renwin->SetMultiSamples(0);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  double eyePosition[3] = { 0.0, 0.0, 2.0 };

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetScreenBottomLeft(bottomLeft);
  camera->SetScreenBottomRight(bottomRight);
  camera->SetScreenTopRight(topRight);
  camera->SetUseOffAxisProjection(1);
  camera->SetEyePosition(eyePosition);
  camera->SetEyeSeparation(0.05);
  camera->SetPosition(0.0, 0.0, 2.0);
  camera->SetFocalPoint(0.0, 0.0, -6.6);
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->SetViewAngle(30.0);

  renwin->Render();

  int retVal = vtkRegressionTestImageThreshold(renwin, 25);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (!retVal);
}
