/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderMaster.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include <string.h>

#ifdef USE_SBR
#include "vtkSbrRenderWindow.h"
#endif

#ifdef USE_GLR
#include "vtkGlrRenderWindow.h"
#endif

#ifdef USE_OGLR
#include "vtkOglrRenderWindow.h"
#endif

#ifdef USE_XGLR
#include "vtkXglrRenderWindow.h"
#endif

#ifdef _WIN32
#include "vtkWin32OglrRenderWindow.h"
#endif

#include "vtkRenderMaster.h"

vtkRenderMaster::vtkRenderMaster()
{
}

vtkRenderMaster::~vtkRenderMaster()
{
  vtkRenderWindow *aren;
  
  // we also free all of our renderWindows
  for (this->RenderWindows.InitTraversal(); 
       (aren = this->RenderWindows.GetNextItem()); )
    {
    delete aren;
    }
}

// Description:
// Create a vtkRenderWindow to match the type given. Current
// values for type include sbr for starbasel; glr for SGI's gl;
// oglr for OpenGL and Mesa; and xglr for Sun's XGL.
vtkRenderWindow *vtkRenderMaster::MakeRenderWindow(char *type)
{
#ifdef USE_SBR
  if (!strncmp("sbr",type,4))
    {
    vtkSbrRenderWindow *ren;
    ren = new vtkSbrRenderWindow;
    this->RenderWindows.AddItem(ren);
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef USE_GLR
  if (!strncmp("glr",type,3))
    {
    vtkGlrRenderWindow *ren;
    ren = new vtkGlrRenderWindow;
    this->RenderWindows.AddItem(ren);
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef USE_OGLR
  if (!strncmp("oglr",type,4))
    {
    vtkOglrRenderWindow *ren;
    ren = new vtkOglrRenderWindow;
    this->RenderWindows.AddItem(ren);
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef _WIN32
  if (!strncmp("woglr",type,5))
    {
    vtkWin32OglrRenderWindow *ren;
    ren = new vtkWin32OglrRenderWindow;
    this->RenderWindows.AddItem(ren);
    return (vtkRenderWindow *)ren;
    }
#endif
  
#ifdef USE_XGLR
  if (!strncmp("xglr",type,4))
    {
    vtkXglrRenderWindow *ren;
    ren = new vtkXglrRenderWindow;
    this->RenderWindows.AddItem(ren);
    return (vtkRenderWindow *)ren;
    }
#endif

  vtkErrorMacro(<<"RenderMaster Error: unable to return render window.\n");
  return (vtkRenderWindow *)NULL;
}

// Description:
// Create renderer based on environment variable VTK_RENDERER. If VTK_RENDERER
// is not set then it will try to pick the best renderer it can.
vtkRenderWindow *vtkRenderMaster::MakeRenderWindow(void)
{
  char *temp;
  
  // if nothing is set then work down the list of possible renderers
  temp = getenv("VTK_RENDERER");
  if (!temp) 
    {
#ifdef USE_GLR
    temp = "glr";
#endif
#ifdef USE_OGLR
    temp = "oglr";
#endif
#ifdef USE_SBR
    temp = "sbr";
#endif
#ifdef USE_XGLR
    temp = "xglr";
#endif
#ifdef _WIN32
    temp = "woglr";
#endif
    if (!temp)
      {
      vtkErrorMacro(<<"RenderMaster Error: this version of vtk does not have any rendering libraries built in.\n");
      }
    }
    
  return (this->MakeRenderWindow(temp));
}

void vtkRenderMaster::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}

