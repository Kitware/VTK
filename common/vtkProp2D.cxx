/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
#include "vtkProp2D.h"
#include "vtkProperty2D.h"

// Creates an Prop2D with the following defaults: 
// position -1, -1 (view coordinates), layer 0, and visibility on.
vtkProp2D::vtkProp2D()
{
  this->LayerNumber = 0;
  this->Visibility = 1;  // ON
  this->SelfCreatedProperty = 0;
  this->Property = (vtkProperty2D*) NULL;
  this->PositionCoordinate = vtkCoordinate::New();
  this->PositionCoordinate->SetCoordinateSystem(VTK_VIEWPORT);
}

// Destroy an Prop2D.  If the Prop2D created it's own
// property, that property is deleted.
vtkProp2D::~vtkProp2D()
{
  if (this->SelfCreatedProperty)
    {
    this->Property->Delete();
    }
  this->PositionCoordinate->Delete();
  this->PositionCoordinate = NULL;
}

// Set the Prop2D's position in display coordinates.  
void vtkProp2D::SetDisplayPosition(int XPos, int YPos)
{
  this->PositionCoordinate->SetCoordinateSystem(VTK_DISPLAY);
  this->PositionCoordinate->SetValue((float)XPos,(float)YPos,0.0);
}

// Returns an Prop2D's property2D.  Creates a property if one
// doesn't already exist.
vtkProperty2D *vtkProp2D::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkProperty2D::New();
    this->SelfCreatedProperty = 1;
    this->Modified();
    }
  return this->Property;
}

unsigned long int vtkProp2D::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;
  
  time = this->PositionCoordinate->GetMTime();
  mTime = ( time > mTime ? time : mTime );
  
  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkProp2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkReferenceCount::PrintSelf(os,indent);

  os << indent << "Layer Number: " << this->LayerNumber << "\n";
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");

  os << indent << "PositionCoordinate: " << this->PositionCoordinate << "\n";
  this->PositionCoordinate->PrintSelf(os, indent.GetNextIndent());
  
  os << indent << "Property: " << this->Property << "\n";
  if (this->Property)
    {
    this->Property->PrintSelf(os, indent.GetNextIndent());
    }
}




