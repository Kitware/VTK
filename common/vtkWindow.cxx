/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vtkWindow.h"

// Construct an instance of  vtkRenderWindow with its screen size 
// set to 300x300, borders turned on, positioned at (0,0), double 
// buffering turned on.
vtkWindow::vtkWindow()
{
  this->OffScreenRendering = 0;
  this->Size[0] = this->Size[1] = 0;
  this->Position[0] = this->Position[1] = 0;
  this->Mapped = 0;
  this->WindowName = new char[strlen("Visualization Toolkit")+1];
    strcpy( this->WindowName, "Visualization Toolkit" );
  this->Erase = 1;
  this->DoubleBuffer = 0;
  this->DPI = 120;
}

// Destructor for the vtkWindow object.
vtkWindow::~vtkWindow()
{
  if( this->WindowName )
    {
    delete [] this->WindowName;
	this->WindowName = NULL;
    }
}

void vtkWindow::SetWindowName( char * _arg )
{
  vtkDebugMacro("Debug: In " __FILE__ << ", line " << __LINE__ << "\n" 
         << this->GetClassName() << " (" << this << "): setting " 
         << this->WindowName  << " to " << _arg << "\n\n");

  if ( this->WindowName && _arg && (!strcmp(this->WindowName,_arg)))
    {
    return;
    }
  if (this->WindowName)
    {
    delete [] this->WindowName;
    }
  if( _arg )
    {
    this->WindowName = new char[strlen(_arg)+1];
    strcpy(this->WindowName,_arg);
    }
  else
    {
    this->WindowName = NULL;
    }
  this->Modified();
}

int *vtkWindow::GetSize()
{
  return this->Size;
}

void vtkWindow::SetSize(int a[2])
{
  this->SetSize(a[0],a[1]);
}

void vtkWindow::SetSize(int x, int y)
{
  if ((this->Size[0] != x)||(this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    }
}

int *vtkWindow::GetPosition()
{
  return this->Position;
}

void vtkWindow::SetPosition(int a[2])
{
  this->SetPosition(a[0],a[1]);
}
void vtkWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x)||(this->Position[1] != y))
    {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    }
}

void vtkWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

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

}

