/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAssembly.cxx
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
#include "vtkPropAssembly.h"
#include "vtkViewport.h"

// Construct object with no children.
vtkPropAssembly::vtkPropAssembly()
{
  this->Parts = vtkPropCollection::New();
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
  vtkProp *part;
  float fraction;
  int renderedSomething=0;

  fraction = this->AllocatedRenderTime / 
             (float)this->Parts->GetNumberOfItems();
  
  // render the Paths
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      part->SetAllocatedRenderTime(fraction);
      renderedSomething |= part->RenderTranslucentGeometry(ren);
      }
    }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int vtkPropAssembly::RenderOpaqueGeometry(vtkViewport *ren)
{
  vtkProp *part;
  float fraction;
  int   renderedSomething=0;

  fraction = this->AllocatedRenderTime / 
             (float)this->Parts->GetNumberOfItems();
  
  // render the Paths
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      part->SetAllocatedRenderTime(fraction);
      renderedSomething |= part->RenderOpaqueGeometry(ren);
      }
    }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int vtkPropAssembly::RenderOverlay(vtkViewport *ren)
{
  vtkProp *part;
  float fraction;
  int   renderedSomething=0;

  fraction = this->AllocatedRenderTime / 
             (float)this->Parts->GetNumberOfItems();
  
  // render the Paths
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      part->SetAllocatedRenderTime(fraction);
      renderedSomething |= part->RenderOverlay(ren);
      }
    }

  return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int vtkPropAssembly::InitializeRayCasting(vtkViewport *ren)
{
  vtkProp *part;
  int needsToCast=0;

  // render the Paths
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      needsToCast |= part->InitializeRayCasting(ren);
      }
    }

  return needsToCast;
}

int vtkPropAssembly::CastViewRay(VTKRayCastRayInfo *ray)
{
  vtkProp *part;
  int rayHit=0;

  // render the Paths
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      rayHit |= part->CastViewRay(ray);
      }
    }

  return rayHit;
}

int vtkPropAssembly::RenderIntoImage(vtkViewport *ren)
{
  vtkProp *part;
  int success=0;

  // render the Paths
  for ( this->Parts->InitTraversal(); (part=this->Parts->GetNextProp()); )
    {
    if ( part->GetVisibility() )
      {
      success |= part->RenderIntoImage(ren);
      }
    }

  return success;
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

void vtkPropAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp::PrintSelf(os,indent);

  os << indent << "There are: " << this->Parts->GetNumberOfItems()
     << " parts in this assembly\n";
}
