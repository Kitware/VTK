/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAssembly.cxx
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
#include "vtkPropAssembly.h"
#include "vtkViewport.h"
#include "vtkAssemblyNode.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkPropAssembly* vtkPropAssembly::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPropAssembly");
  if(ret)
    {
    return (vtkPropAssembly*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPropAssembly;
}

// Construct object with no children.
vtkPropAssembly::vtkPropAssembly()
{
  this->Parts = vtkPropCollection::New();
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
}

vtkPropAssembly::~vtkPropAssembly()
{
  this->Parts->Delete();
  this->Parts = NULL;
}

// Add a part to the list of Parts.
void vtkPropAssembly::AddPart(vtkProp *prop)
{
  if ( ! this->Parts->IsItemPresent(prop) )
    {
    this->Parts->AddItem(prop);
    this->Modified();
    } 
}

// Remove a part from the list of parts,
void vtkPropAssembly::RemovePart(vtkProp *prop)
{
  if ( this->Parts->IsItemPresent(prop) )
    {
    this->Parts->RemoveItem(prop);
    this->Modified();
    } 
}

// Get the list of parts for this prop assembly.
vtkPropCollection *vtkPropAssembly::GetParts() 
{
  return this->Parts;
}

// Render this assembly and all of its Parts. The rendering process is recursive.
int vtkPropAssembly::RenderTranslucentGeometry(vtkViewport *ren)
{
  vtkProp *prop;
  vtkAssemblyPath *path;
  float fraction;
  int renderedSomething=0;

  fraction = this->AllocatedRenderTime / 
             (float)this->Parts->GetNumberOfItems();
  
  // render the Paths
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop = path->GetLastNode()->GetProp();
    if ( prop->GetVisibility() )
      {
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderTranslucentGeometry(ren);
      prop->PokeMatrix(NULL);
      }
    }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int vtkPropAssembly::RenderOpaqueGeometry(vtkViewport *ren)
{
  vtkProp *prop;
  vtkAssemblyPath *path;
  float fraction;
  int   renderedSomething=0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  fraction = this->AllocatedRenderTime / 
             (float)this->Parts->GetNumberOfItems();
  
  // render the Paths
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop = path->GetLastNode()->GetProp();
    if ( prop->GetVisibility() )
      {
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderOpaqueGeometry(ren);
      prop->PokeMatrix(NULL);
      }
    }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int vtkPropAssembly::RenderOverlay(vtkViewport *ren)
{
  vtkProp *prop;
  vtkAssemblyPath *path;
  float fraction;
  int   renderedSomething=0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  fraction = this->AllocatedRenderTime / 
             (float)this->Parts->GetNumberOfItems();
  
  for ( this->Paths->InitTraversal(); (path = this->Paths->GetNextItem()); )
    {
    prop = path->GetLastNode()->GetProp();
    if ( prop->GetVisibility() )
      {
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderOverlay(ren);
      prop->PokeMatrix(NULL);
      }
    }

  return renderedSomething;
}


void vtkPropAssembly::ReleaseGraphicsResources(vtkWindow *renWin)
{
  vtkProp *part;

  vtkProp::ReleaseGraphicsResources(renWin);

  // broadcast the message down the Parts
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    part->ReleaseGraphicsResources(renWin);
    }
}

// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkPropAssembly::GetBounds()
{
  vtkProp *part;
  int i, n;
  float *bounds, bbox[24];
  int partVisible=0;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      bounds = part->GetBounds();

      if ( bounds != NULL )
        {
        //  For the purposes of GetBounds, an object is visisble only if
        //  its visibility is on and it has visible parts.
        partVisible = 1;

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
            }
          }//for each point of box
        }//if bounds
      }//for each part
    }//for each part

  if ( ! partVisible )
    {
    return NULL;
    }
  else
    {
    return this->Bounds;
    }
}

unsigned long int vtkPropAssembly::GetMTime()
{
  unsigned long mTime=this->vtkProp::GetMTime();
  unsigned long time;
  vtkProp *part;

  for (this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    time = part->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

// Shallow copy another vtkPropAssembly.
void vtkPropAssembly::ShallowCopy(vtkProp *prop)
{
  vtkPropAssembly *propAssembly = vtkPropAssembly::SafeDownCast(prop);
  if ( propAssembly != NULL )
    {
    this->Parts->RemoveAllItems();
    propAssembly->Parts->InitTraversal();
    for (int i=0; i<0; i++)
      {
      this->AddPart(propAssembly->Parts->GetNextProp());
      }
    }

  this->vtkProp::ShallowCopy(prop);
}

void vtkPropAssembly::InitPathTraversal()
{
  this->UpdatePaths();
  this->Paths->InitTraversal();
}

vtkAssemblyPath *vtkPropAssembly::GetNextPath()
{
  if ( this->Paths )
    {
    return this->Paths->GetNextItem();
    }
  return NULL;
}

int vtkPropAssembly::GetNumberOfPaths()
{
  this->UpdatePaths();
  return this->Paths->GetNumberOfItems();
}


// Build the assembly paths if necessary.
void vtkPropAssembly::UpdatePaths()
{
  if ( this->GetMTime() > this->PathTime )
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
    path->AddNode(this,NULL);
    
    vtkProp *prop;
    // Add nodes as we proceed down the hierarchy
    for ( this->Parts->InitTraversal(); 
          (prop = this->Parts->GetNextProp()); )
      {
      // add a matrix, if any
      path->AddNode(prop,prop->GetMatrix());

      // dive into the hierarchy
      prop->BuildPaths(this->Paths,path);
      
      // when returned, pop the last node off of the
      // current path
      path->DeleteLastNode();
      }
    
    path->Delete();
    this->PathTime.Modified();
    }
}

void vtkPropAssembly::BuildPaths(vtkAssemblyPaths *paths, 
                                 vtkAssemblyPath *path)
{
  vtkProp *prop;

  for ( this->Parts->InitTraversal(); 
        (prop = this->Parts->GetNextProp()); )
    {
    path->AddNode(prop,NULL);

    // dive into the hierarchy
    prop->BuildPaths(paths,path);

    // when returned, pop the last node off of the
    // current path
    path->DeleteLastNode();
    }
}


void vtkPropAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp::PrintSelf(os,indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems()
     << " parts in this assembly\n";
}
