/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vtkWindow.h"

// Description:
// Construct an instance of  vtkRenderWindow with its screen size 
// set to 300x300, borders turned on, positioned at (0,0), double 
// buffering turned on.
vtkWindow::vtkWindow()
{
  this->Size[0] = this->Size[1] = 300;
  this->Position[0] = this->Position[1] = 0;
  this->Mapped = 0;
  this->WindowName = new char[strlen("Visualization Toolkit")+1];
    strcpy( this->WindowName, "Visualization Toolkit" );
  this->Erase = 1;
  this->DoubleBuffer = 0;
}

vtkWindow::~vtkWindow()
{
  if( WindowName ) 
    delete [] this->WindowName;
}

void vtkWindow::SetWindowName( char * _arg )
{
  vtkDebugMacro("Debug: In " __FILE__ << ", line " << __LINE__ << "\n" 
         << this->GetClassName() << " (" << this << "): setting " 
         << WindowName  << " to " << _arg << "\n\n");

  if ( WindowName && _arg && (!strcmp(WindowName,_arg))) return;
  if (WindowName) delete [] WindowName;
  if( _arg )
    {
    WindowName = new char[strlen(_arg)+1];
    strcpy(WindowName,_arg);
    }
  else
    {
    WindowName = NULL;
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
  int *temp;

  vtkObject::PrintSelf(os,indent);

  os << indent << "Erase: " << (this->Erase ? "On\n" : "Off\n");
  os << indent << "Window Name: " << this->WindowName << "\n";
  temp = this->GetPosition();
  os << indent << "Position: (" << temp[0] << ", " << temp[1] << ")\n";
  temp = this->GetSize();
  os << indent << "Size: (" << temp[0] << ", " << temp[1] << ")\n";


}

