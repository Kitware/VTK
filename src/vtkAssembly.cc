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
// Construct object with no children.
vtkAssembly::vtkAssembly()
{
  this->Paths = NULL;
}

vtkAssembly::~vtkAssembly()
{
  this->DeletePaths();
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
// Copy another assembly.
vtkAssembly& vtkAssembly::operator=(const vtkAssembly& assembly)
{
  *((vtkActor *)this) = assembly;
  this->DeletePaths();

  return *this;
}

// Description:
// Render this assembly and all its parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry 
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its parts.
void vtkAssembly::Render(vtkRenderer *ren)
{
  vtkActor *actor;
  vtkActorCollection *path;

  this->UpdatePaths();

  // render the Paths
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    actor = path->GetLastItem();
    if ( actor->GetVisibility() ) actor->Render(ren);
    }
}

void vtkAssembly::InitPartTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

// Description:
// Return the next part in the hierarchy of assembly parts.  This method 
// returns a properly transformed and updated actor.
vtkActor *vtkAssembly::GetNextPart()
{
  vtkActorCollection *path;

  if ( (path = Paths->GetNextItem()) == NULL ) 
    {
    return NULL;
    }
  else
    {
    return path->GetLastItem();
    }
}

// Build the assembly paths if necessary.
void vtkAssembly::UpdatePaths()
{
  if ( this->GetMTime() > this->PathTime )
    {
    this->DeletePaths();
    }

  if ( ! this->Paths )
    {
    this->Paths = new vtkAssemblyPaths;
    vtkActorCollection *path = new vtkActorCollection;
    this->Paths->AddItem(path);

    this->BuildPaths(this->Paths,path);
    this->PathTime.Modified();
    }
}

// Build assembly paths from this current assembly. Paths consist of
// an ordered sequence of actors, with transformations properly concatenated.
void vtkAssembly::BuildPaths(vtkAssemblyPaths *paths, vtkActorCollection *path)
{
  vtkActor *part, *actor, *copy=new vtkActor, *previous;
  vtkMatrix4x4 *matrix;
  vtkActorCollection *childPath;

  *copy = *this;

  if ( path->GetNumberOfItems() < 1 ) //we're starting at the top of the hierarchy
    {
    if ( this->GetUserMatrix() )
      {
      matrix = new vtkMatrix4x4;
      *matrix = *(this->GetUserMatrix());
      copy->SetUserMatrix(matrix);
      }
    }
  else //somewhere in the middle of the assembly hierarchy
    {
    previous = path->GetLastItem();
    matrix = new vtkMatrix4x4;
    *matrix = previous->GetMatrix();
    copy->SetUserMatrix(matrix);
    }

  path->AddItem(copy);

  // add our children to the path (if we're visible). 
  if ( this->Visibility )
    {
    for ( this->Parts.InitTraversal(); (part = this->Parts.GetNextItem()); )
      {
      //a new path is created for each child
      childPath = new vtkActorCollection;
      paths->AddItem(childPath);

      // copy incoming path
      for ( path->InitTraversal(); (actor = path->GetNextItem()); )
        {
        copy = new vtkActor;
        *copy = *actor;

        if ( actor->GetUserMatrix() )
          {
          matrix = new vtkMatrix4x4;
          *matrix = *(actor->GetUserMatrix());
          copy->SetUserMatrix(matrix);
          }

        childPath->AddItem(copy);
        }

      // create new paths
      part->BuildPaths(paths,childPath);
      }
    }//if visible
}

// Delete the paths asscoiated with this assembly hierarchy.
void vtkAssembly::DeletePaths()
{
  if ( this->Paths )
    {
    vtkActor *actor;
    vtkActorCollection *path;

    for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
      {
      for ( path->InitTraversal(); (actor = path->GetNextItem()); )
        {
        if ( actor->GetUserMatrix() ) actor->GetUserMatrix()->Delete();
        actor->Delete();
        }
      path->Delete();
      }
    this->Paths->Delete();
    this->Paths = NULL;
    }
}

// Description:
// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkAssembly::GetBounds()
{
  vtkActor *actor;
  vtkMapper *mapper;
  vtkActorCollection *path;
  int i,n;
  float *bounds, bbox[24], *fptr;
  float *result;
  vtkMatrix4x4 matrix;

  this->UpdatePaths();

  // now calculate the new bounds
  if ( this->Mapper ) 
    {
    vtkActor::GetBounds();
    }    
  else
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    }

  this->Transform.PostMultiply();  
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    actor = path->GetLastItem();

    if ( (mapper = actor->GetMapper()) )
      {
      bounds = mapper->GetBounds();

      // fill out vertices of a bounding box
      bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
      bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
      bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
      bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
      bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
      bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
      bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
      bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];

      // save the old transform
      actor->GetMatrix(matrix);
      this->Transform.Push(); 
      this->Transform.Identity();
      this->Transform.Concatenate(matrix);

      // and transform into actors coordinates
      fptr = bbox;
      for (n = 0; n < 8; n++) 
        {
        this->Transform.SetPoint(fptr[0],fptr[1],fptr[2],1.0);

        // now store the result
        result = this->Transform.GetPoint();
        fptr[0] = result[0] / result[3];
        fptr[1] = result[1] / result[3];
        fptr[2] = result[2] / result[3];
        fptr += 3;
        }

      this->Transform.Pop();  

      for (i = 0; i < 8; i++)
        {
        for (n = 0; n < 3; n++)
          {
          if (bbox[i*3+n] < this->Bounds[n*2]) this->Bounds[n*2] = bbox[i*3+n];
          if (bbox[i*3+n] > this->Bounds[n*2+1]) this->Bounds[n*2+1] = bbox[i*3+n];
          }
        }//for each point of box
      }//if mapper
    }//for each path

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

void vtkAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);

  os << indent << "There are: " << this->Parts.GetNumberOfItems()
     << " parts in this assembly\n";
}

