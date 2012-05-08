/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindow.h"

#include "vtkToolkits.h"


//-----------------------------------------------------------------------------
// Construct an instance of  vtkRenderWindow with its screen size
// set to 300x300, borders turned on, positioned at (0,0), double
// buffering turned on.
vtkWindow::vtkWindow()
{
  this->OffScreenRendering = 0;
  this->Size[0] = this->Size[1] = 0;
  this->Position[0] = this->Position[1] = 0;
  this->Mapped = 0;
  const char windowname[] = "Visualization Toolkit";
  this->WindowName = new char[strlen(windowname)+1];
  strcpy( this->WindowName, windowname );
  this->Erase = 1;
  this->DoubleBuffer = 0;
  this->DPI = 120;
  this->TileViewport[0] = 0;
  this->TileViewport[1] = 0;
  this->TileViewport[2] = 1.0;
  this->TileViewport[3] = 1.0;
  this->TileSize[0] = 0;
  this->TileSize[1] = 0;
  this->TileScale[0] = 1;
  this->TileScale[1] = 1;
}

//-----------------------------------------------------------------------------
// Destructor for the vtkWindow object.
vtkWindow::~vtkWindow()
{
  this->SetWindowName( NULL );
}

//-----------------------------------------------------------------------------
int *vtkWindow::GetSize()
{
  this->TileSize[0] = this->Size[0]*this->TileScale[0];
  this->TileSize[1] = this->Size[1]*this->TileScale[1];

  return this->TileSize;
}

//-----------------------------------------------------------------------------
int* vtkWindow::GetActualSize()
{
  // Some subclasses override GetSize() to do some additional magic.
  this->GetSize();
  return this->Size;
}

//-----------------------------------------------------------------------------
void vtkWindow::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}

//-----------------------------------------------------------------------------
void vtkWindow::SetSize(int x, int y)
{
  if ( this->Size[0] != x
    || this->Size[1] != y )
    {
    this->Size[0] = x;
    this->Size[1] = y;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int *vtkWindow::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
void vtkWindow::SetPosition(int a[2])
{
  this->SetPosition(a[0],a[1]);
}

//-----------------------------------------------------------------------------
void vtkWindow::SetPosition(int x, int y)
{
  if ( this->Position[0] != x
    || this->Position[1] != y )
    {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    }
}

//-----------------------------------------------------------------------------
void vtkWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Erase: " << (this->Erase ? "On\n" : "Off\n");
  if ( this->WindowName )
    {
    os << indent << "Window Name: " << this->WindowName << "\n";
    }
  else
    {
    os << indent << "Window Name: (none)\n";
    }

  // Can only print out the ivars because the window may not have been
  // created yet.
  //  temp = this->GetPosition();
  os << indent << "Position: (" << this->Position[0] << ", " << this->Position[1] << ")\n";
  //  temp = this->GetSize();
  os << indent << "Size: (" << this->Size[0] << ", " << this->Size[1] << ")\n";
  os << indent << "Mapped: " << this->Mapped << "\n";
  os << indent << "OffScreenRendering: " << this->OffScreenRendering << "\n";
  os << indent << "Double Buffered: " << this->DoubleBuffer << "\n";
  os << indent << "DPI: " << this->DPI << "\n";
  os << indent << "TileScale: (" << this->TileScale[0] << ", "
     << this->TileScale[1] << ")\n";
  os << indent << "TileViewport: (" << this->TileViewport[0] << ", "
     << this->TileViewport[1] << ", " << this->TileViewport[2] << ", "
     << this->TileViewport[3] << ")\n";
}

