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
  Size[0] = Size[1] = 300;
  Position[0] = Position[1] = 0;
  Borders = 1;
  FullScreen = 0;
  OldScreen[0] = OldScreen[1] = 0;
  OldScreen[2] = OldScreen[3] = 300;
  OldScreen[4] = 1;
  Mapped = 0;
  DoubleBuffer = 1;

  strcpy(this->Name,"Visualization Library");
}

void vlRenderWindow::Render()
{
  vlDebugMacro(<< "Starting Render Method.\n");
  this->Start();
  this->Renderers.Render();
  this->Frame();
}

void vlRenderWindow::AddRenderers(vlRenderer *ren)
{
  // we are its parent 
  ren->SetRenderWindow(this);
  this->Renderers.AddMember(ren);
}

void vlRenderWindow::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}
