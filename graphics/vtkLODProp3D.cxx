/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLODProp3D.cxx
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
#include <stdlib.h>
#include <math.h>

#include "vtkLODProp3D.h"
#include "vtkActor.h"
#include "vtkVolume.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"


//------------------------------------------------------------------------------
vtkLODProp3D* vtkLODProp3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLODProp3D");
  if(ret)
    {
    return (vtkLODProp3D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLODProp3D;
}




#define VTK_INDEX_NOT_IN_USE    -1

#define VTK_INVALID_LOD_INDEX   -2

#define VTK_LOD_ACTOR_TYPE       1
#define VTK_LOD_VOLUME_TYPE      2

// Construct a new vtkLODProp3D. Automatic LOD selection is on, there are
// no LODs.
vtkLODProp3D::vtkLODProp3D()
{
  this->NumberOfEntries               = 0;
  this->NumberOfLODs                  = 0;
  this->LODs                          = NULL;
  this->CurrentIndex                  = 1000;
  this->AutomaticLODSelection         = 1;
  this->SelectedLODID                 = 1000;
  this->SelectedLODIndex              = -1;
  this->SelectedPickLODID             = 1000;
  this->AutomaticPickLODSelection     = 1;
  this->PreviousPickProp              = NULL;
  this->PreviousPickMethod            = NULL;
  this->PreviousPickMethodArg         = NULL;
}

// Destruct the vtkLODProp3D. Delete the vtkProp3Ds that were created
// for the LODs. Delete the array of LODs.
vtkLODProp3D::~vtkLODProp3D()
{
  int i;

  // Delete the vtkProp3D objects that were created
  for ( i = 0; i < this->NumberOfEntries; i++ )
    {
    if ( this->LODs[i].ID != VTK_INDEX_NOT_IN_USE )
      {
      this->LODs[i].Prop3D->Delete();
      }
    }

  // Delete the array of LODs
  if ( this->NumberOfEntries > 0 )
    {
      delete [] this->LODs;
    }
}

int vtkLODProp3D::ConvertIDToIndex( int id )
{
  int    index=0;

  while ( index < this->NumberOfEntries && this->LODs[index].ID != id )
    {
    index++;
    }
  if ( index == this->NumberOfEntries )
    {
    vtkErrorMacro( << "Could not locate ID: " << id );
    index = VTK_INVALID_LOD_INDEX;
    }

  return index;
}


// Get the next available entry index
int vtkLODProp3D::GetNextEntryIndex()
{
  int                 index;
  int                 i;
  int                 amount;
  vtkLODProp3DEntry   *newLODs;

  // Search for an available index
  index = 0;
  while ( index < this->NumberOfEntries && this->LODs[index].ID != 
	  VTK_INDEX_NOT_IN_USE )
    {
    index++;
    }

  // If an available index was not found, we need more entries
  if ( index >= this->NumberOfEntries )
    {
    // If we have no entries, create 10. If we already have some, create
    // twice as many as we already have.
    amount = (this->NumberOfEntries)?(this->NumberOfEntries*2):(10);
    
    // Make the new array
    newLODs = new vtkLODProp3DEntry[amount];

    // Copy the old entries into the new array
    for ( i = 0; i < this->NumberOfEntries; i++ )
      {
      newLODs[i].Prop3D        = this->LODs[i].Prop3D;
      newLODs[i].Prop3DType    = this->LODs[i].Prop3DType;
      newLODs[i].ID            = this->LODs[i].ID;
      newLODs[i].EstimatedTime = this->LODs[i].EstimatedTime;
      newLODs[i].Level         = this->LODs[i].Level;
      newLODs[i].State         = this->LODs[i].State;
      }

    // This is the index that we will return - one past the old entries
    index = i;

    // Initialize the new entries to default values
    for ( ; i < amount; i++ )
      {
      newLODs[i].Prop3D = NULL;
      newLODs[i].ID     = VTK_INDEX_NOT_IN_USE;
      }

    // Delete the old array and set the pointer to the new one
    delete [] this->LODs;
    this->LODs = newLODs;

    // Set the new number of entries that we have
    this->NumberOfEntries = amount;
    }
  return index;
}

// Get the bounds of this prop. This is just the max bounds of all LODs
float *vtkLODProp3D::GetBounds()
{
  float newBounds[6];
  int   i;
  int   first = 1;

  // Loop through all valid entries
  for ( i = 0; i < this->NumberOfEntries; i++ )
    {
    if ( this->LODs[i].ID != VTK_INDEX_NOT_IN_USE )
      {
      vtkProp3D *p = this->LODs[i].Prop3D;
      if ( p->GetMTime() < this->GetMTime() )
	{
	p->SetUserMatrix( this->GetMatrix() );
	}

      // Get the bounds of this entry
      p->GetBounds(newBounds);

      // If this is the first entry, this is the current bounds
      if ( first )
	{
	memcpy( this->Bounds, newBounds, 6*sizeof(float) );
	first = 0;
	}
      // If this is not the first entry, compare these bounds with the
      // current bounds expanding the current ones as necessary
      else
	{
	this->Bounds[0] = 
	  (newBounds[0] < this->Bounds[0])?(newBounds[0]):(this->Bounds[0]);
	this->Bounds[1] = 
	  (newBounds[1] > this->Bounds[1])?(newBounds[1]):(this->Bounds[1]);
	this->Bounds[2] = 
	  (newBounds[2] < this->Bounds[2])?(newBounds[2]):(this->Bounds[2]);
	this->Bounds[3] = 
	  (newBounds[3] > this->Bounds[3])?(newBounds[3]):(this->Bounds[3]);
	this->Bounds[4] = 
	  (newBounds[4] < this->Bounds[4])?(newBounds[4]):(this->Bounds[4]);
	this->Bounds[5] = 
	  (newBounds[5] > this->Bounds[5])?(newBounds[5]):(this->Bounds[5]);
	}
      }
    }

  return this->Bounds;
}

// Method to remove a LOD based on an ID
void vtkLODProp3D::RemoveLOD( int id )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  this->LODs[index].Prop3D->Delete();
  this->LODs[index].ID = VTK_INDEX_NOT_IN_USE;
  this->NumberOfLODs--;
}


// Convenience method to get the ID of the LOD that was used
// during the last render
int vtkLODProp3D::GetLastRenderedLODID( )
{
  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    return -1;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    return -1;
    }

  return this->LODs[this->SelectedLODIndex].ID;
}

// Convenience method to get the estimated render time for a given LOD
// based on an ID (the number returned when the LOD was added)
float vtkLODProp3D::GetLODEstimatedRenderTime( int id )
{
  int index = this->ConvertIDToIndex( id );

  if ( index != VTK_INVALID_LOD_INDEX )
    {
    return this->GetLODIndexEstimatedRenderTime( index );
    }
  else
    {
    return 0.0;
    }
}

float vtkLODProp3D::GetLODIndexEstimatedRenderTime( int index )
{
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    return 0;
    }  

  return this->LODs[index].EstimatedTime;
}

// Convenience method to set an actor LOD without a texture, or a 
// backface property.  Needed from tcl (for example) where null pointers 
// are not possible
int vtkLODProp3D::AddLOD( vtkMapper *m, vtkProperty *p, float time )
{
  return this->AddLOD( m, p, (vtkProperty *) NULL, (vtkTexture *)NULL, time );
}

// Convenience method to set an actor LOD without a texture.
// Needed from tcl (for example) where null pointers are not possible
int vtkLODProp3D::AddLOD( vtkMapper *m, vtkProperty *p, 
                         vtkProperty *back, float time )
{
  return this->AddLOD( m, p, back, (vtkTexture *)NULL, time );
}

// Convenience method to set an actor LOD without a backface property.
// Needed from tcl (for example) where null pointers are not possible
int vtkLODProp3D::AddLOD( vtkMapper *m, vtkProperty *p, 
                         vtkTexture *t, float time )
{
  return this->AddLOD( m, p, (vtkProperty *)NULL, t, time );
}

// Convenience method to set an actor LOD without a property.
// Needed from tcl (for example) where null pointers are not possible
int vtkLODProp3D::AddLOD( vtkMapper *m, vtkTexture *t, float time )
{
  return this->AddLOD( m, (vtkProperty *)NULL, (vtkProperty *)NULL, t, time );
}

// Convenience method to set an actor LOD without a texture or a property.
// Needed from tcl (for example) where null pointers are not possible
int vtkLODProp3D::AddLOD( vtkMapper *m, float time )
{
  return this->AddLOD( m, (vtkProperty *)NULL, (vtkProperty *)NULL, 
      (vtkTexture *)NULL, time );
} 

// The real method for adding an actor LOD.
int vtkLODProp3D::AddLOD( vtkMapper *m, vtkProperty *p, 
			  vtkProperty *back, vtkTexture *t, float time )
{
  int          index;
  vtkActor     *actor;
  vtkMatrix4x4 *matrix;

  index = this->GetNextEntryIndex();

  actor = vtkActor::New();
  matrix = vtkMatrix4x4::New();
  this->GetMatrix( matrix );
  actor->SetUserMatrix( matrix );
  matrix->Delete();
  actor->SetMapper( m );
  if ( p ) 
    {
    actor->SetProperty( p );
    }

  if ( back )
    {
    actor->SetBackfaceProperty(back);
    }

  if ( t )
    {
    actor->SetTexture( t );
    }

  this->LODs[index].Prop3D        = (vtkProp3D *)actor;
  this->LODs[index].Prop3DType    = VTK_LOD_ACTOR_TYPE;
  this->LODs[index].ID            = this->CurrentIndex++;
  this->LODs[index].EstimatedTime = time;
  this->LODs[index].Level         = 0.0;
  this->LODs[index].State         = 1;
  this->NumberOfLODs++;

  actor->SetEstimatedRenderTime(time);
  
  return this->LODs[index].ID;
}

// Convenience method to set a volume LOD without a property.
// Needed from tcl (for example) where null pointers are not possible
int vtkLODProp3D::AddLOD( vtkVolumeMapper *m, float time )
{
  return this->AddLOD( m, (vtkVolumeProperty *)NULL, time );
}

// The real method for adding a volume LOD.
int vtkLODProp3D::AddLOD( vtkVolumeMapper *m, vtkVolumeProperty *p, 
			  float time )
{
  int           index;
  vtkVolume     *volume;
  vtkMatrix4x4  *matrix;

  index = this->GetNextEntryIndex();

  volume = vtkVolume::New();
  matrix = vtkMatrix4x4::New();
  this->GetMatrix(matrix);
  volume->SetUserMatrix( matrix );
  matrix->Delete();
  volume->SetMapper( m );
  if ( p ) 
    {
    volume->SetProperty( p );
    }

  this->LODs[index].Prop3D        = (vtkProp3D *)volume;
  this->LODs[index].Prop3DType    = VTK_LOD_VOLUME_TYPE;
  this->LODs[index].ID            = this->CurrentIndex++;
  this->LODs[index].EstimatedTime = time;
  this->LODs[index].Level         = 0.0;
  this->LODs[index].State         = 1;
  this->NumberOfLODs++;

  volume->SetEstimatedRenderTime(time);
  
  return this->LODs[index].ID;
}

// Set the mapper for an LOD that is an actor
void vtkLODProp3D::SetLODMapper( int id, vtkMapper *m )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot set an actor mapper on a non-actor!");
    return;
    }

  ((vtkActor *)this->LODs[index].Prop3D)->SetMapper( m );
}

// Get the mapper for an LOD that is an actor
void vtkLODProp3D::GetLODMapper( int id, vtkMapper **m )
{
  *m = NULL;
	
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot get an actor mapper on a non-actor!");

    return;
    }

  *m = ((vtkActor *)this->LODs[index].Prop3D)->GetMapper();
}

// Set the mapper for an LOD that is a volume
void vtkLODProp3D::SetLODMapper( int id, vtkVolumeMapper *m )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_VOLUME_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot set a volume mapper on a non-volume!");
    return;
    }

  ((vtkVolume *)this->LODs[index].Prop3D)->SetMapper( m );
}

// Get the mapper for an LOD that is an actor
void vtkLODProp3D::GetLODMapper( int id, vtkVolumeMapper **m )
{
  *m = NULL;
  
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_VOLUME_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot get a volume mapper on a non-volume!");
    return;
    }

  *m = ((vtkVolume *)this->LODs[index].Prop3D)->GetMapper();
}

// Get the mapper for an LOD that is an AbstractMapper3D
vtkAbstractMapper3D *vtkLODProp3D::GetLODMapper( int id )
{
  vtkAbstractMapper3D *m = NULL;
  
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return m;
    }

  if ( this->LODs[index].Prop3DType == VTK_LOD_ACTOR_TYPE )
    {
	m = ((vtkActor *)this->LODs[index].Prop3D)->GetMapper();
    }
  else if ( this->LODs[index].Prop3DType == VTK_LOD_VOLUME_TYPE )
    {
	m = ((vtkVolume *)this->LODs[index].Prop3D)->GetMapper();
    }

  return m;
}


// Set the property for an LOD that is an actor
void vtkLODProp3D::SetLODProperty( int id, vtkProperty *p )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot set an actor property on a non-actor!");
    return;
    }

  ((vtkActor *)this->LODs[index].Prop3D)->SetProperty( p );
}

// Get the property for an LOD that is an actor
void vtkLODProp3D::GetLODProperty( int id, vtkProperty **p )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot get an actor property on a non-actor!");
    return;
    }

  *p = ((vtkActor *)this->LODs[index].Prop3D)->GetProperty();
}

// Set the property for an LOD that is a volume
void vtkLODProp3D::SetLODProperty( int id, vtkVolumeProperty *p )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_VOLUME_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot set a volume property on a non-volume!");
    return;
    }

  ((vtkVolume *)this->LODs[index].Prop3D)->SetProperty( p );
}

// Get the property for an LOD that is an actor
void vtkLODProp3D::GetLODProperty( int id, vtkVolumeProperty **p )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_VOLUME_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot get a volume property on a non-volume!");
    return;
    }

  *p = ((vtkVolume *)this->LODs[index].Prop3D)->GetProperty();
}

// Set the texture for an LOD that is an actor
void vtkLODProp3D::SetLODTexture( int id, vtkTexture *t )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot set an actor texture on a non-actor!");
    return;
    }

  ((vtkActor *)this->LODs[index].Prop3D)->SetTexture( t );
}

// Get the texture for an LOD that is an actor
void vtkLODProp3D::GetLODTexture( int id, vtkTexture **t )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot get an actor texture on a non-actor!");
    return;
    }

  *t = ((vtkActor *)this->LODs[index].Prop3D)->GetTexture();
}

// Set the backface property for an LOD that is an actor
void vtkLODProp3D::SetLODBackfaceProperty( int id, vtkProperty *t )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot set an actor backface property on a non-actor!");
    return;
    }

  ((vtkActor *)this->LODs[index].Prop3D)->SetBackfaceProperty( t );
}

// Get the backface property for an LOD that is an actor
void vtkLODProp3D::GetLODBackfaceProperty( int id, vtkProperty **t )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX )
    {
    return;
    }

  if ( this->LODs[index].Prop3DType != VTK_LOD_ACTOR_TYPE )
    {
    vtkErrorMacro( << "Error: Cannot get an actor backface property on a non-actor!");
    return;
    }

  *t = ((vtkActor *)this->LODs[index].Prop3D)->GetBackfaceProperty();
}

void vtkLODProp3D::EnableLOD( int id )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX || index == VTK_INDEX_NOT_IN_USE )
    {
    return;
    }
  
  this->LODs[index].State = 1;
}

void vtkLODProp3D::DisableLOD( int id )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX || index == VTK_INDEX_NOT_IN_USE )
    {
    return;
    }
  
  this->LODs[index].State = 0;
}

void vtkLODProp3D::SetLODLevel( int id, float level )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX || index == VTK_INDEX_NOT_IN_USE )
    {
    return;
    }
  
  this->LODs[index].Level = level;
}

float vtkLODProp3D::GetLODLevel( int id )
{
  int index = this->ConvertIDToIndex( id );

  if ( index == VTK_INVALID_LOD_INDEX || index == VTK_INDEX_NOT_IN_USE )
    {
    return -1;
    }
  
  return this->LODs[index].Level;
}

float vtkLODProp3D::GetLODIndexLevel( int index )
{

  if ( index == VTK_INVALID_LOD_INDEX || index == VTK_INDEX_NOT_IN_USE )
    {
    return -1;
    }
  
  return this->LODs[index].Level;
}

// Release any graphics resources that any of the LODs might be using
// for a particular window (such as display lists). 
void vtkLODProp3D::ReleaseGraphicsResources(vtkWindow *w)
{
  int i;

  // Loop through all LODs and pass this message along
  for ( i = 0; i < this->NumberOfEntries; i++ )
    {
    if ( this->LODs[i].ID != VTK_INDEX_NOT_IN_USE )
      {
      this->LODs[i].Prop3D->ReleaseGraphicsResources( w );
      }
    }
}

// Does the selected LOD need ray casting?
int vtkLODProp3D::RequiresRayCasting( )
{
  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    vtkErrorMacro( << "Index out of range!" );
    return 0;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    vtkErrorMacro( << "Index not valid!" );
    return 0;
    }

  // Actually ask the question
  return this->LODs[this->SelectedLODIndex].Prop3D->RequiresRayCasting();
}

// Does the selected LOD need to be rendered into an image?
int vtkLODProp3D::RequiresRenderingIntoImage( )
{
  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    vtkErrorMacro( << "Index out of range!" );
    return 0;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    vtkErrorMacro( << "Index not valid!" );
    return 0;
    }

  // Actually ask the question
  return 
    this->LODs[this->SelectedLODIndex].Prop3D->RequiresRenderingIntoImage();
}

// Standard render method - render any opaque geometry in the selected LOD
int vtkLODProp3D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int retval;

  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    vtkErrorMacro( << "Index out of range!" );
    return 0;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    vtkErrorMacro( << "Index not valid!" );
    return 0;
    }

  // Actually do the rendering
  retval = 
    this->LODs[this->SelectedLODIndex].Prop3D->RenderOpaqueGeometry(viewport);

  this->EstimatedRenderTime += 
    this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime();

  return retval;
}

// Standard render method - render any translucent geometry in the selected LOD
int vtkLODProp3D::RenderTranslucentGeometry(vtkViewport *viewport)
{
  int retval;

  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    vtkErrorMacro( << "Index out of range!" );
    return 0;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    vtkErrorMacro( << "Index not valid!" );
    return 0;
    }

  // Actually do the rendering
  retval = this->LODs[this->SelectedLODIndex].Prop3D->
    RenderTranslucentGeometry(viewport);

  this->EstimatedRenderTime += 
    this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime();
  
  return retval;
} 


// Standard render method - render the selected LOD into an image
int vtkLODProp3D::RenderIntoImage(vtkViewport *viewport)
{
  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    vtkErrorMacro( << "Index out of range!" );
    return 0;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    vtkErrorMacro( << "Index not valid!" );
    return 0;
    }

  // Actually do the rendering
  return this->LODs[this->SelectedLODIndex].Prop3D->RenderIntoImage(viewport);
}

// Standard render method - cast a view ray for the selected LOD
int vtkLODProp3D::CastViewRay( VTKRayCastRayInfo *rayInfo )
{
  // Don't do any error checking - assume this won't be called unless
  // RequiresRayCasting() return 1 - error checking was performed there
  return this->LODs[this->SelectedLODIndex].Prop3D->CastViewRay(rayInfo);
}

// Standard render method - initialize ray casting for the selected LOD
int vtkLODProp3D::InitializeRayCasting( vtkViewport *viewport)
{
  // Don't do any error checking - assume this won't be called unless
  // RequiresRayCasting() return 1 - error checking was performed there
  return 
    this->LODs[this->SelectedLODIndex].Prop3D->InitializeRayCasting(viewport);
}

// Override the method from vtkProp - add to both this prop and the prop of
// the selected LOD
void vtkLODProp3D::AddEstimatedRenderTime( float t, vtkViewport *vp )
{
  // Add to this prop's estimated render time
  this->EstimatedRenderTime += t;

  // Check if the selected index is in range
  if ( this->SelectedLODIndex < 0 ||
       this->SelectedLODIndex >= this->NumberOfEntries )
    {
    vtkErrorMacro( << "Index out of range!" );
    return;
    }

  // Check if the selected index is valid
  if ( this->LODs[this->SelectedLODIndex].ID == VTK_INDEX_NOT_IN_USE )
    {
    vtkErrorMacro( << "Index not valid!" );
    return;
    }
  
  // Now that error checking is done, add to the estimated render time
  // of the selected LOD
  this->LODs[this->SelectedLODIndex].Prop3D->AddEstimatedRenderTime(t, vp);
}

void vtkLODProp3D::RestoreEstimatedRenderTime()
{
  // restore the EstimatedTime of the last LOD to be rendered
  if ( this->SelectedLODIndex >= 0 &&
       this->SelectedLODIndex < this->NumberOfEntries )
    {
    this->LODs[this->SelectedLODIndex].Prop3D->RestoreEstimatedRenderTime();
    }
}

// Set the allocated render time - this is where the decision is made
// as to which LOD to select
void vtkLODProp3D::SetAllocatedRenderTime( float t, vtkViewport *vp )
{
  int    i;
  int    index = -1;
  float  bestTime;
  float  bestLevel = 0;
  float  targetTime;
  float  estimatedTime;
  float  newTime;

  // update the EstimatedTime of the last LOD to be rendered
  if ( this->SelectedLODIndex >= 0 &&
       this->SelectedLODIndex < this->NumberOfEntries )
    {
    // For stability, blend in the new time - 25% old + 75% new
    newTime = 
      this->LODs[this->SelectedLODIndex].Prop3D->GetEstimatedRenderTime(vp);
    this->LODs[this->SelectedLODIndex].EstimatedTime = 
      0.25 * this->LODs[this->SelectedLODIndex].EstimatedTime +
      0.75 * newTime;
    }
  
  this->SavedEstimatedRenderTime = this->EstimatedRenderTime;
  
  if ( this->AutomaticLODSelection )
    {
    bestTime = -1.0;

    targetTime = t;

    for ( i = 0; i < this->NumberOfEntries; i++ )
      {
      if ( this->LODs[i].ID != VTK_INDEX_NOT_IN_USE &&
           this->LODs[i].State == 1 )
	{
	// Gather some information
	estimatedTime = this->GetLODIndexEstimatedRenderTime(i);
	
	// If we've never rendered this LOD and we have no info on it,
	// then try it out
	if ( estimatedTime == 0.0 )
	  {
	  index = i;
	  bestTime = 0.0;
	  bestLevel = this->GetLODIndexLevel(i);
	  break;
	  }
	
	// If we do have at least a guess as to the render time, and
	// this seems like the best we have so far, pick it.
	// It is the best we have if 
	//
	// 1) our estimated time is less than what we are looking for, 
	//    but greater than any we have selected so far. 
	//
	// 2) we have not selected anything else yet 
	//    (regardless of what the estimated time is)
	//
	// 3) it is less than the time of the currently selected LOD 
	//    if that LOD's time is greater than the time we are targeting.
	//
	if ( estimatedTime > 0.0 && 
	     ( ( estimatedTime > bestTime && estimatedTime < targetTime ) ||
	       ( bestTime == -1.0 ) ||
	       ( estimatedTime < bestTime && bestTime > targetTime ) ) )
	  {
	  index = i;
	  bestTime = estimatedTime;
	  bestLevel = this->GetLODIndexLevel(i);
	  }
	}
      }

    // If we aren't trying some level for the first time with 0.0 bestTime,
    // make sure there isn't a LOD that can be rendered faster and has a 
    // higher level 
    float level;
    if ( bestTime != 0.0 )
      {
      for ( i = 0; i < this->NumberOfEntries; i++ )
	{
	if ( this->LODs[i].ID != VTK_INDEX_NOT_IN_USE &&
	     this->LODs[i].State == 1 )
	  {
	  // Gather some information
	  estimatedTime = this->GetLODIndexEstimatedRenderTime(i);
	  level = this->GetLODIndexLevel(i);

	  // Update the index and the level, but not the time. This is
	  // so that we find the best level that can be rendered
	  // faster than the LOD selected above.
	  if ( estimatedTime <= bestTime && level < bestLevel )
	    {
	    index = i;
	    bestLevel = level;
	    }
	  }
	}
      }
    }
  else
    {
    index = 0;
    while ( index < this->NumberOfEntries && this->LODs[index].ID != 
	    this->SelectedLODID )
      {
      index++;
      }
    if ( index == this->NumberOfEntries )
      {
      vtkErrorMacro( << "Could not render selected LOD ID: " << 
                     this->SelectedLODID );
      index = 0;
      while ( index < this->NumberOfEntries && this->LODs[index].ID != 
	      VTK_INDEX_NOT_IN_USE )
	{
	index++;
	}
      }
    
    }

  this->SelectedLODIndex = index;
  this->LODs[this->SelectedLODIndex].Prop3D->SetAllocatedRenderTime( t, vp );
  this->EstimatedRenderTime = 0.0;
  this->AllocatedRenderTime = t;

  // Push the matrix down into the selected LOD
  vtkProp3D *p = this->LODs[this->SelectedLODIndex].Prop3D;
  if ( p->GetMTime() < this->GetMTime() )
    {
    p->SetUserMatrix( this->GetMatrix()) ;
    }

}

void vtkLODProp3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp3D::PrintSelf(os,indent);

  os << indent << "Number Of LODs: " << this->NumberOfLODs << endl;

  os << indent << "Selected LOD ID: " << this->SelectedLODID << endl;

  os << indent << "AutomaticLODSelection: " 
     << (this->AutomaticLODSelection ? "On\n" : "Off\n");

  os << indent << "AutomaticPickLODSelection: " 
     << (this->AutomaticPickLODSelection ? "On\n" : "Off\n");

  os << indent << "SelectedPickLODID: " << this->SelectedPickLODID << endl;
  
}

void vtkLODProp3D::GetActors(vtkPropCollection *ac)
{
  vtkDebugMacro(<< "vtkLODProp3D::GetActors");
  int index = 0;
  int lodID;

  lodID = this->GetPickLODID();
  index = this->ConvertIDToIndex(lodID);

  if (index == VTK_INVALID_LOD_INDEX) 
    {
    return;
    }

  if (! this->LODs[index].Prop3D->IsA("vtkVolume"))
    {
    ac->AddItem(this->LODs[index].Prop3D);
    }
}

void vtkLODProp3D::SetPickMethod(void (*f)(void *), void *arg)
{
  for (int i = 0; i < this->NumberOfLODs; i++) 
    {
    this->LODs[i].Prop3D->SetPickMethod(f, arg);
    }
}

void vtkLODProp3D::SetPickMethodArgDelete(void (*f)(void *))
{
  for (int i = 0; i < this->NumberOfLODs; i++) 
    {
    this->LODs[i].Prop3D->SetPickMethodArgDelete(f);
    }
}

int vtkLODProp3D::GetAutomaticPickPropIndex(void)
{
  float bestTime = -1.0;
  int index = 0;
  float targetTime = 0;
  float estimatedTime = 0.0;

    for (int i = 0; i < this->NumberOfEntries; i++ )
      {
      if ( this->LODs[i].ID != VTK_INDEX_NOT_IN_USE )
	{
	// Gather some information
	estimatedTime = this->GetLODIndexEstimatedRenderTime(i);
	
	// If we've never rendered this LOD and we have no info on it,
	// then try it out
	if ( estimatedTime == 0.0 )
	  {
	  index = i;
	  bestTime = 0.0;
	  break;
	  }
	
	// If we do have at least a guess as to the render time, and
	// this seems like the best we have so far, pick it.
	// It is the best we have if 
	//
	// 1) our estimated time is less than what we are looking for, 
	//    but greater than any we have selected so far. 
	//
	// 2) we have not selected anything else yet 
	//    (regardless of what the estimated time is)
	//
	// 3) it is less than the time of the currently selected LOD 
	//    if that LOD's time is greater than the time we are targeting.
	//
	if ( estimatedTime > 0.0 && 
	     ( ( estimatedTime > bestTime && estimatedTime < targetTime ) ||
	       ( bestTime == -1.0 ) ||
	       ( estimatedTime < bestTime && bestTime > targetTime ) ) )
	  {
	  index = i;
	  bestTime = estimatedTime;
	  }
	}
      }
    return index;
}


int vtkLODProp3D::GetPickLODID(void)
{
  int lodID=0;

  vtkDebugMacro(<< "vtkLODProp3D::GetPickLODID");
  int index = 0;
  if (this->AutomaticPickLODSelection)
    {
    if ( this->SelectedLODIndex < 0 ||
	 this->SelectedLODIndex >= this->NumberOfEntries )
      {
      index = this->GetAutomaticPickPropIndex();
      }
    else
      {
      index = this->SelectedLODIndex;
      }
	lodID = this->LODs[index].ID;
    }
  else
    {
    if (this->PreviousPickProp)
      {
      this->PreviousPickProp->SetPickMethod(NULL, NULL);
      }
	  lodID = this->SelectedPickLODID;
    }

    return lodID;
}


void vtkLODProp3D::SetSelectedPickLODID(int id)
{
  this->SelectedPickLODID = id;
  this->Modified();
}

void vtkLODProp3D::ShallowCopy(vtkProp *prop)
{
  vtkLODProp3D *a = vtkLODProp3D::SafeDownCast(prop);

  if ( a != NULL )
    {
    this->SetAutomaticLODSelection(a->GetAutomaticLODSelection());
    this->SetAutomaticPickLODSelection(a->GetAutomaticPickLODSelection());
    this->SetSelectedLODID(a->GetSelectedLODID());
    this->NumberOfLODs = a->NumberOfLODs;
    for(int i=0; i<this->NumberOfLODs; i++)
      {
      }
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}


