/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test queries the maximum texture size for 1D/2D/3D textures.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include <vtkTestUtilities.h>

#include <vtkNew.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTextureObject.h>

int TestMaximumTextureSize(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin.Get());
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());
  renWin->SetSize(500,500);
  renWin->Render();

  vtkOpenGLRenderWindow* glContext =
    vtkOpenGLRenderWindow::SafeDownCast(renWin.GetPointer());

  if (glContext)
    {
    vtkNew<vtkTextureObject> textureObject;
    int maxTextureSize1D =
      textureObject->GetMaximumTextureSize1D(glContext);
    int maxTextureSize2D =
      textureObject->GetMaximumTextureSize1D(glContext);
    int maxTextureSize3D =
      textureObject->GetMaximumTextureSize1D(glContext);

#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
    if (maxTextureSize1D != -1 && maxTextureSize2D != -1 &&
        maxTextureSize3D != -1)
#else
    if (maxTextureSize2D != -1 && maxTextureSize3D != -1)
#endif
      {
      return 0;
      }

    return 1;
    }
  return 0;
}
