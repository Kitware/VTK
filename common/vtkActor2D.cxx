/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor2D.cxx
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

#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkMapper2D.h"

// Description:
// Creates an actor2D with the following defaults: 
// position -1, -1 (view coordinates)
// orientation 0, scale (1,1), layer 0, visibility on
vtkActor2D::vtkActor2D()
{
  this->Orientation = 0.0;
  this->Scale[0] = 1.0;
  this->Scale[1] = 1.0;
  this->LayerNumber = 0;
  this->Visibility = 1;  // ON
  this->SelfCreatedProperty = 0;
  this->Property = (vtkProperty2D*) NULL;
  this->Mapper = (vtkMapper2D*) NULL;
  this->PositionCoordinate = vtkCoordinate::New();
  this->PositionCoordinate->SetCoordinateSystem(VTK_VIEWPORT);
}

// Description:
// Destroy an actor2D.  If the actor2D created it's own
// property, that property is deleted.
vtkActor2D::~vtkActor2D()
{
  if (this->SelfCreatedProperty) this->Property->Delete();
  this->PositionCoordinate->Delete();
  this->PositionCoordinate = NULL;
}

void vtkActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkReferenceCount::PrintSelf(os,indent);
  os << indent << "Orientation: " << this->Orientation << "\n";
  os << indent << "Scale: (" << this->Scale[0] << ", " << this->Scale[1] << ")\n";
  os << indent << "Layer Number: " << this->LayerNumber << "\n";
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");

  os << indent << "PositionCoordinate: " << this->PositionCoordinate << "\n";
  this->PositionCoordinate->PrintSelf(os, indent.GetNextIndent());
  
  os << indent << "Self Created Property: " << (this->SelfCreatedProperty ? "Yes\n" : "No\n");
  os << indent << "Property: " << this->Property << "\n";
  if (this->Property) this->Property->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper: " << this->Mapper << "\n";
  if (this->Mapper) this->Mapper->PrintSelf(os, indent.GetNextIndent());
}

// Description:
// Set the actor2D's position in display coordinates.  
void vtkActor2D::SetDisplayPosition(int XPos, int YPos)
{
  this->PositionCoordinate->SetCoordinateSystem(VTK_DISPLAY);
  this->PositionCoordinate->SetValue((float)XPos,(float)YPos,0.0);
}

// Description:
// Renders an actor2D's property and then it's mapper.
void vtkActor2D::Render (vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkActor2D::Render");

  if (!this->Property)
    {
    vtkDebugMacro(<< "vtkActor2D::Render - Creating Property2D");
    // Force creation of default property
    this->GetProperty();
    }

  this->Property->Render(viewport);

  if (!this->Mapper) 
    {
    vtkErrorMacro(<< "vtkActor2D::Render - No mapper set");
    return;
    }

  vtkDebugMacro(<<"vtkActor2D::Render - Rendering mapper");
  this->Mapper->Render(viewport, this); 

}

// Description:
// Returns an actor2D's property2D.  Creates a property if one
// doesn't already exist.
vtkProperty2D *vtkActor2D::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkProperty2D::New();
    this->SelfCreatedProperty = 1;
    this->Modified();
    }
  return this->Property;
}


unsigned long int vtkActor2D::GetMTime()
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




