/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssembly.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkAssembly.hh"

// Description:
// Construct object with ApplyTransform enabled; and ApplyProperty disabled.
vtkAssembly::vtkAssembly()
{
  this->ApplyTransform = 1;
  this->ApplyProperty = 0;
}

// Description:
// Add a part to the list of parts.
void vtkAssembly::AddPart(vtkActor *actor)
{
  this->Parts.AddItem(actor);
}

// Description:
// Remove a part from the list of parts,
void vtkAssembly::RemovePart(vtkActor *actor)
{
  this->Parts.RemoveItem(actor);
}

// Description:
// Render this assembly and all its parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry 
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its parts.
void vtkAssembly::Render(vtkRenderer *ren)
{
  vtkActor *part;

  vtkActor::Render(ren);

  if ( this->Parts.GetNumberOfItems() <= 0 ) return;

  // propogate properties and/or transformation
  if ( this->ApplyTransform && this->RenderTime < this->GetMTime() )
    {
    this->ApplyTransformation();
    }

  if ( this->ApplyProperty && this->RenderTime < this->Property->GetMTime() )
    {
    this->ApplyProperties();
    }

  for (this->Parts.InitTraversal(); part = this->Parts.GetNextItem(); )
    {
    if ( part->GetVisibility() ) part->Render(ren);
    }

  this->RenderTime.Modified();
}

// Description:
// Update the indicated part by the current transformation matrix of this
// assembly.
void vtkAssembly::ApplyTransformation()
{
  vtkActor *part;
  vtkMatrix4x4 *matrix;

  // traverse list of parts, setting parent matrix
  for (this->Parts.InitTraversal(); part = this->Parts.GetNextItem(); )
    {
    if ( (matrix=part->GetUserMatrix()) == NULL )
      {
      matrix = new vtkMatrix4x4;
      part->SetUserMatrix(matrix);
      }

    *matrix = this->GetMatrix();
    }
}

// Description:
// Update the indicated part by the current property values of this
// assembly.
void vtkAssembly::ApplyProperties()
{
  vtkActor *part;
  vtkProperty *property, *thisProperty=this->GetProperty();

  // traverse list of parts, setting properties
  for (this->Parts.InitTraversal(); part = this->Parts.GetNextItem(); )
    {
    property = part->GetProperty();
    *property = *thisProperty;
    }
}

void vtkAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);

  os << indent << "There are: " << this->Parts.GetNumberOfItems()
     << " parts in this assembly\n";
  os << indent << "Apply Transform: " << (this->ApplyTransform ? "On\n" : "Off\n");
  os << indent << "Apply Property: " << (this->ApplyProperty ? "On\n" : "Off\n");
}

