/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * This header file is designed to be included into your program
 * to support delayed loading of opengl and if needed use of Mesa
 * in cases where the users computer does not support OpenGL 3.2
 * natively.
 *
 * To use this class you must
 *
 * 1) Build VTK with the advanced cmake option VTK_USE_OPENGL_DELAYED_LOAD
 *    turned on.
 *
 * 2) Include this file in your application and call HandleOpenGL32Issues
 *    before you do any rendering or anything that would cause opengl
 *    to be used. Ideally do this right at the beginning of your program.
 *
 * 3) Make sure you include vtkTestOpenGLVersion.exe with your application
 *    and pass the fullpath to it as the first argument to HandleOpenGL32Issues
 *
 * 4) Make sure you include the Mesa libraries with your application. Typically
 *    this means opengl32.dll swrAVX.dll swrAVX2.dll and graw.dll. Pass the path
 *    to these libraries as the second argument to HandleOpenGL32Issues
 */

#include <windows.h>

// returns an int, zero indicates a problem though right now
// all paths return 1.
int HandleOpenGL32Issues(const char* pathToTestOpenGLExecutable, const char* mesaLibPath)
{
  // run the test executable and collect the result
  int result = system(pathToTestOpenGLExecutable);

  // if the default works then just return
  if (result == 0)
  {
    return 1;
  }

  // otherwise set the dll path so that mesa willbe loaded
  SetDllDirectory(mesaLibPath);

  return 1;
}

// VTK-HeaderTest-Exclude: vtkTestOpenGLVersion.h
