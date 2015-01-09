/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGLUTRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example tests the vtkRenderingExternal module by drawing a GLUT window
// and rendering a VTK cube in it. It uses an ExternalVTKWidget and sets a
// vtkExternalOpenGLRenderWindow to it.
//
// The test also demonstrates the use of
// PreserveColorBuffer and PreserveDepthBuffer flags on the
// vtkExternalOpenGLRenderer by drawing a GL_TRIANGLE in the scene before
// drawing the vtk sphere.

// GLUT includes
#if defined(__APPLE__)
# include <GLUT/glut.h> // Include GLUT API.
#else
# include "vtkWindows.h" // Needed to include OpenGL header on Windows.
# include <GL/glut.h> // Include GLUT API.
#endif

// STD includes
#include <iostream>

// VTK includes
#include <ExternalVTKWidget.h>
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkExternalOpenGLRenderWindow.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkTesting.h>

// Global variables used by the glutDisplayFunc and glutIdleFunc
vtkNew<ExternalVTKWidget> externalVTKWidget;
static bool initilaized = false;
static int NumArgs;
char** ArgV;
static bool tested = false;
static int retVal = 0;
static int windowId = -1;
static int windowH = 301;
static int windowW = 300;

static void MakeCurrentCallback(vtkObject* caller,
                                long unsigned int eventId,
                                void * clientData,
                                void * callData)
{
  if (initilaized)
    {
    glutSetWindow(windowId);
    }
}

/* Handler for window-repaint event. Call back when the window first appears and
   whenever the window needs to be re-painted. */
void display()
{
  if (!initilaized)
    {
    vtkNew<vtkExternalOpenGLRenderWindow> renWin;
    renWin->SetSize(windowW, windowH);
    externalVTKWidget->SetRenderWindow(renWin.GetPointer());
    vtkNew<vtkCallbackCommand> callback;
    callback->SetCallback(MakeCurrentCallback);
    renWin->AddObserver(vtkCommand::WindowMakeCurrentEvent,
                        callback.GetPointer());
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.GetPointer());
    vtkRenderer* ren = externalVTKWidget->AddRenderer();
    ren->AddActor(actor.GetPointer());
    vtkNew<vtkCubeSource> cs;
    mapper->SetInputConnection(cs->GetOutputPort());
    actor->RotateX(45.0);
    actor->RotateY(45.0);
    ren->ResetCamera();

    vtkNew<vtkLight> light;
    light->SetLightTypeToSceneLight();
    light->SetPosition(0, 0, 1);
    light->SetConeAngle(25.0);
    light->SetPositional(true);
    light->SetFocalPoint(0, 0, 0);
    light->SetDiffuseColor(1, 0, 0);
    light->SetAmbientColor(0, 1, 0);
    light->SetSpecularColor(0, 0, 1);
    renWin->Render();
    // Make sure light is added after first render call
    ren->AddLight(light.GetPointer());

    initilaized = true;
    }

  // Enable depth testing. Demonstrates OpenGL context being managed by external
  // application i.e. GLUT in this case.
  glEnable(GL_DEPTH_TEST);

  // Buffers being managed by external application i.e. GLUT in this case.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
  glClearDepth(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the color buffer

  glFlush();  // Render now
  glBegin(GL_TRIANGLES);
    glVertex3f(-1.5,-1.5,0.0);
    glVertex3f(1.5,0.0,0.0);
    glVertex3f(0.0,1.5,1.0);
  glEnd();

  externalVTKWidget->GetRenderWindow()->Render();
  glutSwapBuffers();
}

void test()
{
  bool interactiveMode = false;
  vtkTesting* t = vtkTesting::New();
  for(int cc = 1; cc < NumArgs; cc++)
    {
    t->AddArgument(ArgV[cc]);
    if (strcmp(ArgV[cc], "-I") == 0)
      {
      interactiveMode = true;
      }
    }
  t->SetRenderWindow(externalVTKWidget->GetRenderWindow());
  if (!tested)
    {
    retVal = t->RegressionTest(0);
    tested = true;
    }
  t->Delete();
  if (!interactiveMode)
    {
    // Exit out of the infinitely running loop
    exit(!retVal);
    }
}

void handleResize(int w, int h)
{
  externalVTKWidget->GetRenderWindow()->SetSize(w, h);
  glutPostRedisplay();
}

void onexit(void)
{
  initilaized = false;
}

/* Main function: GLUT runs as a console application starting at main()  */
int TestGLUTRenderWindow(int argc, char** argv)
{
  NumArgs = argc;
  ArgV = argv;
  glutInit(&argc, argv);                 // Initialize GLUT
  glutInitWindowSize(windowW, windowH);   // Set the window's initial width & height
  glutInitWindowPosition(0, 0); // Position the window's initial top-left corner
  windowId = glutCreateWindow("VTK External Window Test"); // Create a window with the given title
  glutDisplayFunc(display); // Register display callback handler for window re-paint
  glutIdleFunc(test); // Register test callback handler for vtkTesting
  glutReshapeFunc(handleResize); // Register resize callback handler for window resize
  atexit(onexit);  // Register callback to uninitialize on exit
  glutMainLoop();  // Enter the infinitely event-processing loop
  return 0;
}
