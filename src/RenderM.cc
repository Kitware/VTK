/*=========================================================================

  Program:   VisLib
  Module:    RenderM.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include <iostream.h>
#include <string.h>
#include "RenderM.h"

#ifdef USE_KGLR
#include "kglrRen.h"
#endif

vlRenderMaster::vlRenderMaster()
{
}

vlRenderer *vlRenderMaster::MakeRenderer(char *type)
{

#ifdef USE_KGLR
  if (!strncmp("kglr",type,4))
    {
    vlKglrRenderer *ren;
    ren = new vlKglrRenderer;
    return (vlRenderer *)ren;
    }
#endif

  cerr <<"RenderMaster Error: unable to return renderer.\n";
  return (vlRenderer *)NULL;
}

vlRenderer *vlRenderMaster::MakeRenderer(void)
{
  return (this->MakeRenderer(getenv("VL_RENDERER")));
}
