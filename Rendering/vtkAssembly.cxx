/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssembly.cxx
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
#include "vtkAssembly.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkAssemblyPaths.h"
#include "vtkAssemblyNode.h"
#include "vtkProp3DCollection.h"
#include "vtkActor.h"
#include "vtkVolume.h"

//-----------------------------------------------------------------------------
vtkAssembly* vtkAssembly::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAssembly");
  if(ret)
    {
    return (vtkAssembly*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAssembly;
}

// Construct object with no children.
vtkAssembly::vtkAssembly()
{
  this->Parts = vtkProp3DCollection::New();
}

vtkAssembly::~vtkAssembly()
{
  this->Parts->Delete();
  this->Parts = NULL;
}

// Add a part to the list of Parts.
void vtkAssembly::AddPart(vtkProp3D *prop)
{
  if ( ! this->Parts->IsItemPresent(prop) )
    {
    this->Parts->AddItem(prop);
    this->Modified();
    } 
}

// Remove a part from the list of parts,
void vtkAssembly::RemovePart(vtkProp3D *prop)
{
  if ( this->Parts->IsItemPresent(prop) )
    {
    this->Parts->RemoveItem(prop);
    this->Modified();
    } 
}

// Shallow copy another assembly.
void vtkAssembly::ShallowCopy(vtkProp *prop)
{
  vtkAssembly *a = vtkAssembly::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->Parts->RemoveAllItems();
    a->Parts->InitTraversal();
    for (int i=0; i<0; i++)
      {
      this->Parts->AddItem(a->Parts->GetNextProp3D());
      }
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry 
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int vtkAssembly::RenderTranslucentGeometry(vtkViewport *ren)
{
  vtkProp3D *prop3D;
  vtkAssemblyPath *path;
  float fraction;
  int renderedSomething = 0;

  this->UpdatePaths();

  // for allocating render time between components
  // simple equal allocation
  fraction = this->AllocatedRenderTime 
    / (float)(this->Paths->GetNumberOfItems());
  
  // render the Paths
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop3D = (vtkProp3D *)path->GetLastNode()->GetProp();
    if ( prop3D->GetVisibility() )
      {
      prop3D->SetAllocatedRenderTime(fraction, ren);
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop3D->RenderTranslucentGeometry(ren);
      prop3D->PokeMatrix(NULL);
      }
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}


// Render this assembly and all its Parts. The rendering process is recursive.
// Note that a mapper need not be defined. If not defined, then no geometry 
// will be drawn for this assembly. This allows you to create "logical"
// assemblies; that is, assemblies that only serve to group and transform
// its Parts.
int vtkAssembly::RenderOpaqueGeometry(vtkViewport *ren)
{
  vtkProp3D *prop3D;
  vtkAssemblyPath *path;
  float fraction;
  int   renderedSomething = 0;

  this->UpdatePaths();

  // for allocating render time between components
  // simple equal allocation
  fraction = this->AllocatedRenderTime 
    / (float)(this->Paths->GetNumberOfItems());
  
  // render the Paths
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop3D = (vtkProp3D *)path->GetLastNode()->GetProp();
    if ( prop3D->GetVisibility() )
      {
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      prop3D->SetAllocatedRenderTime(fraction, ren);
      renderedSomething += prop3D->RenderOpaqueGeometry(ren);
      prop3D->PokeMatrix(NULL);
      }
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

void vtkAssembly::ReleaseGraphicsResources(vtkWindow *renWin)
{
  vtkProp3D *prop3D;

  for ( this->Parts->InitTraversal(); 
        (prop3D = this->Parts->GetNextProp3D()); )
    {
    prop3D->ReleaseGraphicsResources(renWin);
    }
}

void vtkAssembly::GetActors(vtkPropCollection *ac)
{
  vtkProp3D *prop3D;
  vtkActor *actor;
  vtkAssemblyPath *path;

  this->UpdatePaths();
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop3D = (vtkProp3D *)path->GetLastNode()->GetProp();
    if ( (actor = vtkActor::SafeDownCast(prop3D)) != NULL )
      {
      ac->AddItem(actor);
      }
    }
}

void vtkAssembly::GetVolumes(vtkPropCollection *ac)
{
  vtkProp3D *prop3D;
  vtkVolume *volume;
  vtkAssemblyPath *path;

  this->UpdatePaths();
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop3D = (vtkProp3D *)path->GetLastNode()->GetProp();
    if ( (volume = vtkVolume::SafeDownCast(prop3D)) != NULL )
      {
      ac->AddItem(volume);
      }
    }
}

void vtkAssembly::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

// Return the next part in the hierarchy of assembly Parts.  This method 
// returns a properly transformed and updated actor.
vtkAssemblyPath *vtkAssembly::GetNextPath()
{
  if ( this->Paths )
    {
    return this->Paths->GetNextItem();
    }
  else
    {
    return NULL;
    }
}

int vtkAssembly::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}

// Build the assembly paths if necessary. UpdatePaths()
// is only called when the assembly is at the root
// of the hierarchy; otherwise UpdatePaths() is called.
void vtkAssembly::UpdatePaths()
{
  if ( this->GetMTime() > this->PathTime ||
    (this->Paths != NULL && this->Paths->GetMTime() > this->PathTime) )
    {
    if ( this->Paths != NULL )
      {
      this->Paths->Delete();
      this->Paths = NULL;
      }

    // Create the list to hold all the paths
    this->Paths = vtkAssemblyPaths::New();
    vtkAssemblyPath *path = vtkAssemblyPath::New();

    //add ourselves to the path to start things off
    path->AddNode(this,this->GetMatrix());
    
    // Add nodes as we proceed down the hierarchy
    vtkProp3D *prop3D;
    for ( this->Parts->InitTraversal(); 
          (prop3D = this->Parts->GetNextProp3D()); )
      {
      path->AddNode(prop3D,prop3D->GetMatrix());

      // dive into the hierarchy
      prop3D->BuildPaths(this->Paths,path);
      
      // when returned, pop the last node off of the
      // current path
      path->DeleteLastNode();
      }

    path->Delete();
    this->PathTime.Modified();
    }
}

// Build assembly paths from this current assembly. A path consists of
// an ordered sequence of props, with transformations properly concatenated.
void vtkAssembly::BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path)
{
  vtkProp3D *prop3D;

  for ( this->Parts->InitTraversal(); 
        (prop3D = this->Parts->GetNextProp3D()); )
    {
    path->AddNode(prop3D,prop3D->GetMatrix());

    // dive into the hierarchy
    prop3D->BuildPaths(paths,path);

    // when returned, pop the last node off of the
    // current path
    path->DeleteLastNode();
    }
}

// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkAssembly::GetBounds()
{
  vtkProp3D *prop3D;
  vtkAssemblyPath *path;
  int i, n;
  float *bounds, bbox[24];
  int propVisible=0;

  this->UpdatePaths();

  // now calculate the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;

  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop3D = (vtkProp3D *)path->GetLastNode()->GetProp();
    if ( prop3D->GetVisibility() )
      {
      propVisible = 1;
      prop3D->PokeMatrix(path->GetLastNode()->GetMatrix());
      bounds = prop3D->GetBounds();
      prop3D->PokeMatrix(NULL);

      // fill out vertices of a bounding box
      bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
      bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
      bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
      bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
      bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
      bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
      bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
      bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];

      for (i = 0; i < 8; i++)
        {
        for (n = 0; n < 3; n++)
          {
          if (bbox[i*3+n] < this->Bounds[n*2])
            {
            this->Bounds[n*2] = bbox[i*3+n];
            }
          if (bbox[i*3+n] > this->Bounds[n*2+1])
            {
            this->Bounds[n*2+1] = bbox[i*3+n];
            }
          }//for each coordinate axis
        }//for each point of box
      }//if visible && prop3d
    }//for each path

  if ( ! propVisible )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] =  1.0;
    }

  return this->Bounds;
}

unsigned long int vtkAssembly::GetMTime()
{
  unsigned long mTime=this->vtkProp3D::GetMTime();
  unsigned long time;
  vtkProp3D *prop;

  for (this->Parts->InitTraversal(); (prop = this->Parts->GetNextProp3D()); )
    {
    time = prop->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkProp3D::PrintSelf(os,indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems()
     << " parts in this assembly\n";
}

