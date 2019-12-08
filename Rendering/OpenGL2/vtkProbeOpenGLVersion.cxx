/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOutputWindow.h"
#include "vtkRenderer.h"

#include <sstream>

int main(int, char* /* argv */[])
{
  int result = 0;

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  std::ostringstream toString;

  toString << "Class: " << renderWindow->GetClassName();

  if (!renderWindow->SupportsOpenGL())
  {
    toString << " failed to find a working OpenGL\n\n";
    toString << vtkOpenGLRenderWindow::SafeDownCast(renderWindow)->GetOpenGLSupportMessage();
    result = 1;
  }
  else
  {
    toString << " succeeded in finding a working OpenGL\n\n";
  }

  renderWindow->Render();
  toString << renderWindow->ReportCapabilities();

  vtkOutputWindow::GetInstance()->PromptUserOn();
  vtkOutputWindow::GetInstance()->DisplayText(toString.str().c_str());

  return result;
}
