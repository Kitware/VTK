/*=========================================================================

  Program:   Visualization Library
  Module:    XRenWin.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <stdlib.h>
#include <iostream.h>
#include "XRenWin.hh"
#include "XInter.hh"

vlXRenderWindow::vlXRenderWindow()
{
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)NULL;
}

// Description:
// Get the size of the screen in pixels
int *vlXRenderWindow::GetScreenSize()
{
  this->ScreenSize[0] = 
    DisplayWidth(this->DisplayId, DefaultScreen(this->DisplayId));
  this->ScreenSize[1] = 
    DisplayHeight(this->DisplayId, DefaultScreen(this->DisplayId));

  return this->ScreenSize;
}

// Description:
// Get the current size of the window.
int *vlXRenderWindow::GetSize(void)
{
  XWindowAttributes attribs;

  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Size);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, 
		       this->WindowId, &attribs);

  this->Size[0] = attribs.width;
  this->Size[1] = attribs.height;
  
  return this->Size;
}

// Description:
// Get the position in screen coordinates of the window.
int *vlXRenderWindow::GetPosition(void)
{
  XWindowAttributes attribs;
  int x,y;
  Window child;
  
  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);
  x = attribs.x;
  y = attribs.y;

  XTranslateCoordinates(this->DisplayId,this->WindowId,
		 RootWindowOfScreen(ScreenOfDisplay(this->DisplayId,0)),
			x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

// Description:
// Get the window display id.
Display *vlXRenderWindow::GetDisplayId()
{
  vlDebugMacro(<< "Returning DisplayId of " << (void *)this->DisplayId << "\n"); 

  return this->DisplayId;
}

// Description:
// Get the window id.
Window vlXRenderWindow::GetWindowId()
{
  vlDebugMacro(<< "Returning WindowId of " << (void *)this->WindowId << "\n"); 

  return this->WindowId;
}

// Description:
// Set the window id to a pre-existing window.
void vlXRenderWindow::SetWindowId(Window arg)
{
  vlDebugMacro(<< "Setting WindowId to " << (void *)arg << "\n"); 

  this->WindowId = arg;
}

// Description:
// Set the window id of the new window once a WindowRemap is done.
void vlXRenderWindow::SetNextWindowId(Window arg)
{
  vlDebugMacro(<< "Setting NextWindowId to " << (void *)arg << "\n"); 

  this->NextWindowId = arg;
}

// Description:
// Set the display id of the window to a pre-exisiting display id.
void vlXRenderWindow::SetDisplayId(Display  *arg)
{
  vlDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
}

// Description:
// Create named interactor type
vlRenderWindowInteractor *vlXRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = (vlRenderWindowInteractor *)new vlXRenderWindowInteractor;
  this->Interactor->SetRenderWindow((vlRenderWindow *)this);
  return this->Interactor;
}

void vlXRenderWindow::PrintSelf(ostream& os, vlIndent indent)
{
  this->vlRenderWindow::PrintSelf(os,indent);

  os << indent << "Color Map: " << this->ColorMap << "\n";
  os << indent << "Display Id: " << this->GetDisplayId() << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->GetWindowId() << "\n";
}
