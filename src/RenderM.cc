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
#include "RenderM.hh"

#ifdef USE_KGLR
#include "KglrRenW.hh"
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

  cerr <<"RenderMaster Error: unable to return render window.\n";
  return (vlRenderWindow *)NULL;
}

vlRenderWindow *vlRenderMaster::MakeRenderWindow(void)
{
  return (this->MakeRenderWindow(getenv("VL_RENDERER")));
}
