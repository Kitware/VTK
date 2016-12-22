/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOffscreenIsOffscreen.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkRenderWindow.h"

int TestOffscreenIsOffscreen(int, char* [])
{
  vtkNew<vtkRenderWindow> renWin;
  // This test is only run if VTK_USE_OFFSCREEN is on. So the default should
  // be to use offscreen rendering
  return !renWin->GetOffScreenRendering();
}
