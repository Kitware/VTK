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
// and rendering a VTK sphere in it. The test also demonstrates the use of
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

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkExternalOpenGLRenderer.h>
#include <vtkExternalOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkTesting.h>

// Global variables used by the glutDisplayFunc and glutIdleFunc
vtkNew<vtkExternalOpenGLRenderer> ren;
vtkNew<vtkExternalOpenGLRenderWindow> renWin;
bool initilaized = false;
int NumArgs;
char** ArgV;

/* Handler for window-repaint event. Call back when the window first appears and
   whenever the window needs to be re-painted. */
void display()
{
  if (!initilaized)
    {
    renWin->SetSize(400,400);
    renWin->AddRenderer(ren.GetPointer());
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.GetPointer());
    ren->AddActor(actor.GetPointer());
    vtkNew<vtkSphereSource> ss;
    ss->SetRadius(0.1);
    mapper->SetInputConnection(ss->GetOutputPort());
    initilaized = true;
    }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the color buffer

  glBegin(GL_TRIANGLES);
    glVertex3f(-1.5,-1.5,0.0);
    glVertex3f(1.5,0.0,0.0);
    glVertex3f(0.0,1.5,1.0);
  glEnd();
  glutSwapBuffers();
  glFlush();  // Render now

  GLdouble mv[16],p[16];
  glGetDoublev(GL_MODELVIEW_MATRIX,mv);
  glGetDoublev(GL_PROJECTION_MATRIX,p);
  renWin->SetSize(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
  renWin->Render();
}

void test()
{
  vtkTesting* t = vtkTesting::New();
  for(int cc = 1; cc < NumArgs; cc++)
    {
    t->AddArgument(ArgV[cc]);
    }
  t->SetRenderWindow(renWin.GetPointer());
  int retVal = t->RegressionTest(0);
  t->Delete();
  // Exit out of the infinitely running loop
  exit(!retVal);
}

/* Main function: GLUT runs as a console application starting at main()  */
int TestGLUTRenderWindow(int argc, char** argv)
{
  NumArgs = argc;
  ArgV = argv;
  glutInit(&argc, argv);                 // Initialize GLUT
  glutInitWindowSize(400, 400);   // Set the window's initial width & height
  glutInitWindowPosition(0, 0); // Position the window's initial top-left corner
  glutCreateWindow("VTK External Window Test"); // Create a window with the given title
  glutDisplayFunc(display); // Register display callback handler for window re-paint
  glutIdleFunc(test); // Register test callback handler for vtkTesting
  glutMainLoop();           // Enter the infinitely event-processing loop
  return 0;
}
