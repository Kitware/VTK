/*=========================================================================

  Program:   OSCAR 
  Module:    RenderW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include <string.h>
#include "RenderW.hh"

vlRenderWindow::vlRenderWindow()
{
  strcpy(this->name,"Visualization Library");
}

void vlRenderWindow::Render()
{
  this->Start();
  this->Renderers.Render();
  this->Frame();
}

void vlRenderWindow::AddRenderers(vlRenderer *ren)
{
  this->Renderers.AddMember(ren);
}
