/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RenderM.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <iostream.h>
#include <string.h>
#include "RenderM.hh"

#ifdef USE_KGLR
#include "KglrRenW.hh"
#endif

#ifdef USE_SBR
#include "SbrRenW.hh"
#endif

#ifdef USE_GLR
#include "GlrRenW.hh"
#endif

#ifdef USE_OGLR
#include "OglrRenW.hh"
#endif

#ifdef USE_XGLR
#include "XglrRenW.hh"
#endif

vtkRenderMaster::vtkRenderMaster()
{
}

// Description:
// Create named renderer type.
vtkRenderWindow *vtkRenderMaster::MakeRenderWindow(char *type)
{

#ifdef USE_KGLR
  if (!strncmp("kglr",type,4))
    {
    vtkKglrRenderWindow *ren;
    ren = new vtkKglrRenderWindow;
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef USE_SBR
  if (!strncmp("sbr",type,4))
    {
    vtkSbrRenderWindow *ren;
    ren = new vtkSbrRenderWindow;
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef USE_GLR
  if (!strncmp("glr",type,3))
    {
    vtkGlrRenderWindow *ren;
    ren = new vtkGlrRenderWindow;
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef USE_OGLR
  if (!strncmp("oglr",type,4))
    {
    vtkOglrRenderWindow *ren;
    ren = new vtkOglrRenderWindow;
    return (vtkRenderWindow *)ren;
    }
#endif

#ifdef USE_XGLR
  if (!strncmp("xglr",type,4))
    {
    vtkXglrRenderWindow *ren;
    ren = new vtkXglrRenderWindow;
    return (vtkRenderWindow *)ren;
    }
#endif

  vtkErrorMacro(<<"RenderMaster Error: unable to return render window.\n");
  return (vtkRenderWindow *)NULL;
}

// Description:
// Create renderer based on environment variable VTK_RENDERER. If VTK_RENDERER
// not defined, then use default renderer kglr.
vtkRenderWindow *vtkRenderMaster::MakeRenderWindow(void)
{
  char *temp;
  
  // if nothing is set then try kglr
  temp = getenv("VTK_RENDERER");
  if (!temp) temp = "kglr";

  return (this->MakeRenderWindow(temp));
}

void vtkRenderMaster::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}

