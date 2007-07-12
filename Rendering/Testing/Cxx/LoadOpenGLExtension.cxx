/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LoadOpenGLExtension.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// This code test to make sure vtkOpenGLExtensionManager can properly get
// extension functions that can be used.  To do this, we convolve an image
// with a kernel for a Laplacian filter.  This requires the use of functions
// defined in OpenGL 1.2, which should be available pretty much everywhere
// but still has functions that can be loaded as extensions.

#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkUnsignedCharArray.h"
#include "vtkRegressionTestImage.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"

vtkUnsignedCharArray *image;

GLfloat laplacian[3][3] = {
  { -0.125f, -0.125f, -0.125f },
  { -0.125f,  1.0f,   -0.125f },
  { -0.125f, -0.125f, -0.125f }
};

static void ImageCallback(vtkObject *__renwin, unsigned long, void *, void *)
{
  static int inImageCallback = 0;
  if (inImageCallback)
    {
    cout << "*********ImageCallback called recursively?" << endl;
    return;
    }
  inImageCallback = 1;

  cout << "In ImageCallback" << endl;

  vtkRenderWindow *renwin = static_cast<vtkRenderWindow *>(__renwin);
  int *size = renwin->GetSize();

  cout << "Turn on convolution." << endl;
  glEnable(vtkgl::CONVOLUTION_2D);

  cout << "Read back image." << endl;
  renwin->GetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, 0, image);

  cout << "Turn off convolution." << endl;
  glDisable(vtkgl::CONVOLUTION_2D);

  cout << "Write image." << endl;
  renwin->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, image, 0);

  cout << "Swap buffers." << endl;
  renwin->SwapBuffersOn();
  renwin->Frame();
  renwin->SwapBuffersOff();

  inImageCallback = 0;
}

int LoadOpenGLExtension(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkRenderWindow *renwin = vtkRenderWindow::New();
  renwin->SetSize(250, 250);

  vtkRenderer *renderer = vtkRenderer::New();
  renwin->AddRenderer(renderer);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  renwin->SetInteractor(iren);

  vtkOpenGLExtensionManager *extensions = vtkOpenGLExtensionManager::New();
  extensions->SetRenderWindow(renwin);

  // Force a Render here so that we can call glGetString reliably:
  //
  renwin->Render();

  const char *gl_vendor = (const char *) glGetString(GL_VENDOR);
  const char *gl_version = (const char *) glGetString(GL_VERSION);
  const char *gl_renderer = (const char *) glGetString(GL_RENDERER);

  cout << endl;
  cout << "GL_VENDOR: " << (gl_vendor ? gl_vendor : "(null)") << endl;
  cout << "GL_VERSION: " << (gl_version ? gl_version : "(null)") << endl;
  cout << "GL_RENDERER: " << (gl_renderer ? gl_renderer : "(null)") << endl;

  cout << endl;
  renwin->Print(cout);

  
  int supported=extensions->ExtensionSupported("GL_VERSION_1_2");
  int loaded=0;
  if(supported)
    {
    cout << "Driver claims to support OpenGL 1.2" <<endl;
    loaded=extensions->LoadSupportedExtension("GL_VERSION_1_2");
    if(loaded)
      {
      cout << "OpenGL 1.2 features loaded." <<endl;
      }
    else
      {
      cout << "Failed to load OpenGL 1.2 features!" <<endl;
      }
    }
  supported=extensions->ExtensionSupported("GL_VERSION_1_3");
  if(supported)
    {
    cout << "Driver claims to support OpenGL 1.3" <<endl;
    loaded=extensions->LoadSupportedExtension("GL_VERSION_1_3");
    if(loaded)
      {
      cout << "OpenGL 1.3 features loaded." <<endl;
      }
    else
      {
      cout << "Failed to load OpenGL 1.3 features!" <<endl;
      }
    }
  supported=extensions->ExtensionSupported("GL_VERSION_1_4");
  if(supported)
    {
    cout << "Driver claims to support OpenGL 1.4" <<endl;
    loaded=extensions->LoadSupportedExtension("GL_VERSION_1_4");
    if(loaded)
      {
      cout << "OpenGL 1.4 features loaded." <<endl;
      }
    else
      {
      cout << "Failed to load OpenGL 1.4 features!" <<endl;
      }
    }
  supported=extensions->ExtensionSupported("GL_VERSION_1_5");
  if(supported)
    {
    cout << "Driver claims to support OpenGL 1.5" <<endl;
    loaded=extensions->LoadSupportedExtension("GL_VERSION_1_5");
    if(loaded)
      {
      cout << "OpenGL 1.5 features loaded." <<endl;
      }
    else
      {
      cout << "Failed to load OpenGL 1.5 features!" <<endl;
      }
    }
  supported=extensions->ExtensionSupported("GL_VERSION_2_0");
  if(supported)
    {
    cout << "Driver claims to support OpenGL 2.0" <<endl;
    loaded=extensions->LoadSupportedExtension("GL_VERSION_2_0");
    if(loaded)
      {
      cout << "OpenGL 2.0 features loaded." <<endl;
      }
    else
      {
      cout << "Failed to load OpenGL 2.0 features!" <<endl;
      }
    }
  supported=extensions->ExtensionSupported("GL_VERSION_2_1");
  if(supported)
    {
    cout << "Driver claims to support OpenGL 2.1" <<endl;
    loaded=extensions->LoadSupportedExtension("GL_VERSION_2_1");
    if(loaded)
      {
      cout << "OpenGL 2.1 features loaded." <<endl;
      }
    else
      {
      cout << "Failed to load OpenGL 2.1 features!" <<endl;
      }
    }
  cout << "LoadSupportedExtension..." << endl;
  if (!extensions->LoadSupportedExtension("GL_VERSION_1_2"))
    {
    cout << "Is it possible that your driver does not support OpenGL 1.2?"
         << endl << endl;
    int forceLoad = 0;
    for (int i = 0; i < argc; i++)
      {
      if (strcmp("-ForceLoad", argv[i]) == 0)
        {
        forceLoad = 1;
        break;
        }
      }
    if (forceLoad)
      {
      cout << "Some drivers report supporting only GL 1.1 even though they\n"
           << "actually support 1.2 (and probably higher).  I'm going to\n"
           << "try to load the extension anyway.  You will definitely get\n"
           << "a warning from vtkOpenGLExtensionManager about it.  If GL 1.2\n"
           << "really is not supported (or something else is wrong), I will\n"
           << "seg fault." << endl << endl;
      }
    else
      {
      cout << "Your OpenGL driver reports that it does not support\n"
           << "OpenGL 1.2.  If this is true, I cannot perform this test.\n"
           << "There are a few drivers that report only supporting GL 1.1\n"
           << "when they in fact actually support 1.2 (and probably higher).\n"
           << "If you think this might be the case, try rerunning this test\n"
           << "with the -ForceLoad flag.  However, if Opengl 1.2 is really\n"
           << "not supported, a seg fault will occur." << endl << endl;

      cout << "GetExtensionsString..." << endl;
      cout << extensions->GetExtensionsString() << endl;

      renderer->Delete();
      renwin->Delete();
      iren->Delete();
      extensions->Delete();
      return 0;
      }
    }

  cout << "GetExtensionsString..." << endl;
  cout << extensions->GetExtensionsString() << endl;
  extensions->Delete();

  cout << "Set up pipeline." << endl;
  vtkConeSource *cone = vtkConeSource::New();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(cone->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  renderer->AddActor(actor);

  renderer->ResetCamera();
  vtkCamera *camera = renderer->GetActiveCamera();
  camera->Elevation(-45);

  cout << "Do a render without convolution." << endl;
  renwin->Render();

  // Set up a convolution filter.  We are using the Laplacian filter, which
  // is basically an edge detector.  Once vtkgl::CONVOLUTION_2D is enabled,
  // the filter will be applied any time an image is transfered in the
  // pipeline.
  cout << "Set up convolution filter." << endl;
  vtkgl::ConvolutionFilter2D(vtkgl::CONVOLUTION_2D, GL_LUMINANCE, 3, 3,
                             GL_LUMINANCE, GL_FLOAT, laplacian);
  vtkgl::ConvolutionParameteri(vtkgl::CONVOLUTION_2D,
                               vtkgl::CONVOLUTION_BORDER_MODE,
                               vtkgl::REPLICATE_BORDER);

  image = vtkUnsignedCharArray::New();
  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(ImageCallback);
  renwin->AddObserver(vtkCommand::EndEvent, cbc);
  cbc->Delete();

  // This is a bit of a hack.  The EndEvent on the render window will swap
  // the buffers.
  renwin->SwapBuffersOff();

  cout << "Do test render with convolution on." << endl;
  renwin->Render();
  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  cone->Delete();
  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renwin->Delete();
  iren->Delete();
  image->Delete();

  return !retVal;
}
