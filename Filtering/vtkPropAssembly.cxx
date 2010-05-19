/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPropAssembly.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyPaths.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkPropAssembly);

// Construct object with no children.
vtkPropAssembly::vtkPropAssembly()
{
  this->Parts = vtkPropCollection::New();
  vtkMath::UninitializeBounds(this->Bounds);
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
int vtkPropAssembly::RenderTranslucentPolygonalGeometry(vtkViewport *ren)
{
  vtkProp *prop;
  vtkAssemblyPath *path;
  double fraction;
  int renderedSomething=0;
  
  // Make sure the paths are up-to-date
  this->UpdatePaths();
  
  double numberOfItems =  static_cast<double>(this->Parts->GetNumberOfItems());
  fraction = numberOfItems >= 1.0 ? 
    this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;
    
  
  // render the Paths
  vtkCollectionSimpleIterator sit;
  for ( this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit)); )
    {
    prop = path->GetLastNode()->GetViewProp();
    if ( prop->GetVisibility() )
      {
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderTranslucentPolygonalGeometry(ren);
      prop->PokeMatrix(NULL);
      }
    }

  return renderedSomething;
}

// Description:
// Does this prop have some translucent polygonal geometry?
int vtkPropAssembly::HasTranslucentPolygonalGeometry()
{
  vtkProp *prop;
  vtkAssemblyPath *path;
  int result=0;
  
  // Make sure the paths are up-to-date
  this->UpdatePaths();
  
  // render the Paths
  vtkCollectionSimpleIterator sit;
  for ( this->Paths->InitTraversal(sit); !result && (path = this->Paths->GetNextPath(sit)); )
    {
    prop = path->GetLastNode()->GetViewProp();
    if ( prop->GetVisibility() )
      {
      result=prop->HasTranslucentPolygonalGeometry();
      }
    }
  return result;
}


// Render this assembly and all of its Parts. The rendering process is recursive.
int vtkPropAssembly::RenderVolumetricGeometry(vtkViewport *ren)
{
  vtkProp *prop;
  vtkAssemblyPath *path;
  double fraction;
  int renderedSomething=0;
  
   // Make sure the paths are up-to-date
  this->UpdatePaths();
  
  double numberOfItems =  static_cast<double>(this->Parts->GetNumberOfItems());
  fraction = numberOfItems >= 1.0 ? 
    this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;
  
  // render the Paths
  vtkCollectionSimpleIterator sit;
  for ( this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit)); )
    {
    prop = path->GetLastNode()->GetViewProp();
    if ( prop->GetVisibility() )
      {
      prop->SetAllocatedRenderTime(fraction, ren);
      prop->PokeMatrix(path->GetLastNode()->GetMatrix());
      renderedSomething += prop->RenderVolumetricGeometry(ren);
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
  double fraction;
  int   renderedSomething=0;
  double numberOfItems = 0.0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  numberOfItems =  static_cast<double>(this->Parts->GetNumberOfItems());
  fraction = numberOfItems >= 1.0 ? 
    this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;
  
  // render the Paths
  vtkCollectionSimpleIterator sit;
  for ( this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit)); )
    {
    prop = path->GetLastNode()->GetViewProp();
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
  double fraction;
  int   renderedSomething=0;
  double numberOfItems = 0.0;

  // Make sure the paths are up-to-date
  this->UpdatePaths();

  numberOfItems =  static_cast<double>(this->Parts->GetNumberOfItems());
  fraction = numberOfItems >= 1.0 ? 
    this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;
  
  vtkCollectionSimpleIterator sit;
  for ( this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit)); )
    {
    prop = path->GetLastNode()->GetViewProp();
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
  vtkCollectionSimpleIterator pit;
  for ( this->Parts->InitTraversal(pit); 
        (part=this->Parts->GetNextProp(pit)); )
    {
    part->ReleaseGraphicsResources(renWin);
    }
}

// Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkPropAssembly::GetBounds()
{
  vtkProp *part;
  int i, n;
  double *bounds, bbox[24];
  int partVisible=0;

  // carefully compute the bounds
  vtkCollectionSimpleIterator pit;
  for ( this->Parts->InitTraversal(pit); 
        (part=this->Parts->GetNextProp(pit)); )
    {
    if ( part->GetVisibility() && part->GetUseBounds() )
      {
      bounds = part->GetBounds();
      
      if ( bounds != NULL )
        {
        //  For the purposes of GetBounds, an object is visisble only if
        //  its visibility is on and it has visible parts.
        if (!partVisible)
          {
          // initialize the bounds
          this->Bounds[0] =this->Bounds[2] =this->Bounds[4] = VTK_DOUBLE_MAX;
          this->Bounds[1] =this->Bounds[3] =this->Bounds[5] = -VTK_DOUBLE_MAX;
          partVisible = 1;
          }
        
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

  vtkCollectionSimpleIterator pit;
  for (this->Parts->InitTraversal(pit); 
       (part=this->Parts->GetNextProp(pit)); )
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
    vtkCollectionSimpleIterator pit;
    propAssembly->Parts->InitTraversal(pit);
    for (int i=0; i<0; i++)
      {
      this->AddPart(propAssembly->Parts->GetNextProp(pit));
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
    vtkCollectionSimpleIterator pit;
    for ( this->Parts->InitTraversal(pit); 
          (prop = this->Parts->GetNextProp(pit)); )
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

  vtkCollectionSimpleIterator pit;
  for ( this->Parts->InitTraversal(pit); 
        (prop = this->Parts->GetNextProp(pit)); )
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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems()
     << " parts in this assembly\n";
}
