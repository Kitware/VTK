/*=========================================================================

  Program:   Visualization Library
  Module:    RenderW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <string.h>
#include "RenderW.hh"

// Description:
// Construct object with screen size 300x300, borders turned on, position
// at (0,0), and double buffering turned on.
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

// Description:
// Ask each renderer to render an image. Synchronize this process.
void vlRenderWindow::Render()
{
  vlDebugMacro(<< "Starting Render Method.\n");
  this->Start();
  this->Renderers.Render();
  this->Frame();
}

// Description:
// Add a renderer to the list of renderers.
void vlRenderWindow::AddRenderers(vlRenderer *ren)
{
  // we are its parent 
  ren->SetRenderWindow(this);
  this->Renderers.AddItem(ren);
}

// Description:
// Remove a renderer from the list of renderers.
void vlRenderWindow::RemoveRenderers(vlRenderer *ren)
{
  // we are its parent 
  this->Renderers.RemoveItem(ren);
}

// Description:
// Set the size of the window in screen coordinates.
void vlRenderWindow::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}

void vlRenderWindow::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlRenderWindow::GetClassName()))
    {
    int *temp;

    vlObject::PrintSelf(os,indent);
    
    os << indent << "Borders: " << (this->Borders ? "On\n":"Off\n");
    os << indent << "Double Buffer: " << (this->DoubleBuffer ? "On\n":"Off\n");
    os << indent << "Full Screen: " << (this->FullScreen ? "On\n":"Off\n");
    os << indent << "Name: " << this->Name << "\n";
    temp = this->GetPosition();
    os << indent << "Position: (" << temp[0] << ", " << temp[1] << ")\n";
    temp = this->GetSize();
    os << indent << "Renderers:\n";
    this->Renderers.PrintSelf(os,indent.GetNextIndent());
    os << indent << "Size: (" << temp[0] << ", " << temp[1] << ")\n";
    }
}

