/*=========================================================================

  Program:   Visualization Library
  Module:    RenderM.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
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

vlRenderMaster::vlRenderMaster()
{
}

vlRenderWindow *vlRenderMaster::MakeRenderWindow(char *type)
{

#ifdef USE_KGLR
  if (!strncmp("kglr",type,4))
    {
    vlKglrRenderWindow *ren;
    ren = new vlKglrRenderWindow;
    return (vlRenderWindow *)ren;
    }
#endif

#ifdef USE_SBR
  if (!strncmp("sbr",type,4))
    {
    vlSbrRenderWindow *ren;
    ren = new vlSbrRenderWindow;
    return (vlRenderWindow *)ren;
    }
#endif

  vlErrorMacro(<<"RenderMaster Error: unable to return render window.\n");
  return (vlRenderWindow *)NULL;
}

vlRenderWindow *vlRenderMaster::MakeRenderWindow(void)
{
  char *temp;
  
  // if nothing is set then try kglr
  temp = getenv("VL_RENDERER");
  if (!temp) temp = "kglr";

  return (this->MakeRenderWindow(temp));
}
