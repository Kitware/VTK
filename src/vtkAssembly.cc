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
  this->Paths = NULL;

  this->CurrentActor.SetProperty(&this->CurrentProperty);
  this->CurrentActor.SetUserMatrix(&this->CurrentMatrix);
}

vtkAssembly::~vtkAssembly()
{
  if ( this->Paths )
    {
    vtkActorCollection *path;
    for ( this->Paths->InitTraversal(); path = this->Paths->GetNextItem(); )
      {
      path->Delete();
      }
    this->Paths->Delete();
    }
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
  if ( this->ApplyTransform ) this->ApplyTransformation();
  if ( this->ApplyProperty ) this->ApplyProperties();

  for (this->Parts.InitTraversal(); (part = this->Parts.GetNextItem()); )
    {
    if ( part->GetVisibility() ) part->Render(ren);
    }
}

void vtkAssembly::InitPartTraversal()
{
  vtkActorCollection *path;

  if ( ! this->Paths )
    {
    this->Paths = new vtkAssemblyPaths;
    }
  else
    {
    this->Paths->RemoveAllItems();
    }

  path = new vtkActorCollection;
  path->AddItem((vtkActor *)this);
  this->Paths->AddItem(path);
  this->BuildPath(path);

  this->Paths->InitTraversal();
}

// Description:
// Return the next part in the hierarchy of assembly parts.  This method will
// properly transform and update the properties of the part (if these are
// enabled by the appropriate boolean flags).
vtkActor *vtkAssembly::GetNextPart()
{
  vtkActorCollection *path;

  if ( (path = Paths->GetNextItem()) == NULL ) return NULL;

  this->ApplyTransformation(path);
  return this->ApplyProperties(path);
}

// Build assembly paths from this current assembly. Paths assumed to consist of
// a series of assemblies terminated by a final "actor" (or subclass of actor).
// The paths represent a breadth first traversal of the assembly tree.
void vtkAssembly::BuildPath(vtkActorCollection *path)
{
  vtkActor *part, *actor=path->GetLastItem();
  vtkActorCollection *childPath;
  vtkAssemblyPaths newPaths;
  vtkAssembly *parent;

  if ( strcmp(actor->GetClassName(),"vtkActor") ) //must be assembly
    {
    parent = (vtkAssembly *)actor;
    for (parent->Parts.InitTraversal(); (part = parent->Parts.GetNextItem()); )
      {
      childPath = new vtkActorCollection;

      // copy incoming path
      for ( path->InitTraversal(); actor = path->GetNextItem(); )
        {
        childPath->AddItem(actor);
        }
      childPath->AddItem(part); //and add child to extend path
      this->Paths->AddItem(childPath); // add this path to list of paths
      newPaths.AddItem(childPath); //keep track for breadth-first traversal
      }

    for ( newPaths.InitTraversal(); childPath = newPaths.GetNextItem(); )
      {
      this->BuildPath(childPath);
      }
    }
}

// Apply transformations (if enabled) along the indicated path. Return a
// pointer to the last actor in the path.
vtkActor *vtkAssembly::ApplyTransformation(vtkActorCollection *path)
{
  vtkAssembly *parent;
  vtkActor *child;
  vtkMatrix4x4 *matrix;

  path->InitTraversal();
  parent = (vtkAssembly *)path->GetNextItem();

  for ( ; child = path->GetNextItem(); parent = (vtkAssembly *)child )
    {
    if ( parent->ApplyTransform )
      {
      if ( (matrix=child->GetUserMatrix()) == NULL )
        {
        matrix = new vtkMatrix4x4;
        child->SetUserMatrix(matrix);
        }

      *matrix = parent->GetMatrix();
      }
  }
  return (vtkActor *)parent;
}

// Apply properties (if enabled) along the indicated path. Return a
// pointer to the last actor in the path.
vtkActor *vtkAssembly::ApplyProperties(vtkActorCollection *path)
{
  vtkAssembly *parent;
  vtkActor *child;

  path->InitTraversal();
  parent = (vtkAssembly *)path->GetNextItem();

  for ( ; child = path->GetNextItem(); parent = (vtkAssembly *)child )
    {
    if ( parent->ApplyProperty )
      {
      *(child->GetProperty()) = *(parent->GetProperty());
      }
  }
  return (vtkActor *)parent;
}

// Description:
// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (As
// a side effect this routine propagates transformations through assembly
// hierarchy.)
float *vtkAssembly::GetBounds()
{
  int i;
  vtkActor *part;
  float *bounds;

  if ( this->Mapper ) //updates local ivar
    {
    vtkActor::GetBounds();
    }
  else
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    }

  if ( this->ApplyTransform ) this->ApplyTransformation();
  for (this->Parts.InitTraversal(); part = this->Parts.GetNextItem(); )
    {
    bounds = part->GetBounds();
    for ( i=0; i < 3; i++ )
      {
      if ( bounds[2*i] < this->Bounds[2*i] )
        {
        this->Bounds[2*i] = bounds[2*i];
        }
      if ( bounds[2*i+1] > this->Bounds[2*i+1] )
        {
        this->Bounds[2*i+1] = bounds[2*i+1];
        }
      }
    }
  return this->Bounds;
}

unsigned long int vtkAssembly::GetMTime()
{
  unsigned long mTime=this->vtkActor::GetMTime();
  unsigned long time;
  vtkActor *part;

  for (this->Parts.InitTraversal(); (part = this->Parts.GetNextItem()); )
    {
    time = part->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

// Description:
// Update the children parts by the current transformation matrix of this
// assembly. 
void vtkAssembly::ApplyTransformation()
{
  vtkActor *part;
  vtkMatrix4x4 *matrix;

  // traverse list of parts, setting parent matrix
  for (this->Parts.InitTraversal(); (part = this->Parts.GetNextItem()); )
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
// Update the children parts by the current property values of this
// assembly. 
void vtkAssembly::ApplyProperties()
{
  vtkActor *part;
  vtkProperty *property, *thisProperty=this->GetProperty();

  // traverse list of parts, setting properties
  for (this->Parts.InitTraversal(); (part = this->Parts.GetNextItem()); )
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

