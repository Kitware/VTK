/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkLODActor.cxx
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
#include "vtkLODActor.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkLODActor* vtkLODActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLODActor");
  if(ret)
    {
    return (vtkLODActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLODActor;
}

//----------------------------------------------------------------------------
vtkLODActor::vtkLODActor()
{
  vtkMatrix4x4 *m;
  
  // get a hardware dependent actor and mappers
  this->Device = vtkActor::New();
  m = vtkMatrix4x4::New();
  this->Device->SetUserMatrix(m);
  m->Delete();
  
  this->LODMappers = vtkMapperCollection::New();
  // stuff for creating own LODs
  this->MaskPoints = NULL;
  this->OutlineFilter = NULL;
  this->NumberOfCloudPoints = 150;
  this->LowMapper = NULL;
  this->MediumMapper = NULL;
}

//----------------------------------------------------------------------------
vtkLODActor::~vtkLODActor()
{
  this->Device->Delete();
  this->DeleteOwnLODs();
  this->LODMappers->Delete();
}


//----------------------------------------------------------------------------
void vtkLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);

  os << indent << "Cloud Points: " << this->NumberOfCloudPoints << "\n";

  // how should we print out the LODMappers?
  os << indent << "NumberOfLODMappers: " << this->LODMappers->GetNumberOfItems() 
     << endl;
}


//----------------------------------------------------------------------------
void vtkLODActor::Render(vtkRenderer *ren, vtkMapper *vtkNotUsed(m))
{
  float myTime, bestTime, tempTime;
  vtkMatrix4x4 *matrix;
  vtkMapper *mapper, *bestMapper;
  
  if (this->Mapper == NULL)
    {
    vtkErrorMacro("No mapper for actor.");
    return;
    }
  
  // first time through create lods if non have been added
  if (this->LODMappers->GetNumberOfItems() == 0)
    {
    this->CreateOwnLODs();
    }
  
  
  // If the actor has changed or the primary mapper has changed ...
  // Is this the correct test?
  if (this->MediumMapper)
    {
    if (this->GetMTime() > this->BuildTime || 
	this->Mapper->GetMTime() > this->BuildTime)
      {
      this->UpdateOwnLODs();
      }
    }
  
  // figure out how much time we have to render
  myTime = this->AllocatedRenderTime;

  //   Figure out which resolution to use 
  // none is a valid resolution. Do we want to have a lowest:
  // bbox, single point, ...
  //   There is no order to the list, so it is assumed that mappers that take
  // longer to render are better quality.
  //   Timings might become out of date, but we rely on 

  bestMapper = this->Mapper;
  bestTime = bestMapper->GetTimeToDraw();
  if (bestTime > myTime)
    {
    this->LODMappers->InitTraversal();
    while ((mapper = this->LODMappers->GetNextItem()) != NULL && 
	   bestTime != 0.0)
      {
      tempTime = mapper->GetTimeToDraw();
      
      // If the LOD has never been rendered, select it!
      if (tempTime == 0.0)
	{ 
	// cerr << "      Has never been rendererd\n";
	bestMapper = mapper;
	bestTime = 0.0;
	}
      else
	{
	if (bestTime > myTime && tempTime < bestTime)
	  {
	  // cerr << "      Less than best in violation\n";
	  bestMapper = mapper;
	  bestTime = tempTime;
	  }
	if (tempTime > bestTime && tempTime < myTime)
	  { 
	  // cerr << "      Larger than best\n";
	  bestMapper = mapper;
	  bestTime = tempTime;
	  }
	}
      }
    }
    
  /* render the property */
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }
  this->Device->SetProperty(this->Property);
  
  
  /* render the texture */
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
  
  // make sure the device has the same matrix
  matrix = this->Device->GetUserMatrix();
  this->GetMatrix(matrix);
  
  // Store information on time it takes to render.
  // We might want to estimate time from the number of polygons in mapper.
  this->Device->Render(ren,bestMapper);
  this->EstimatedRenderTime = bestMapper->GetTimeToDraw();
}

int vtkLODActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  int          renderedSomething = 0; 
  vtkRenderer  *ren = (vtkRenderer *)vp;

  if ( ! this->Mapper )
    {
    return 0;
    }

  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque ?
  if (this->GetIsOpaque())
    {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
      {
      this->BackfaceProperty->BackfaceRender(this, ren);
      }
    
    // render the texture 
    if (this->Texture)
      {
      this->Texture->Render(ren);
      }
    this->Render(ren,this->Mapper);

    renderedSomething = 1;
    }

  return renderedSomething;
}

void vtkLODActor::ReleaseGraphicsResources(vtkWindow *renWin)
{
  vtkMapper *mapper;

  vtkActor::ReleaseGraphicsResources(renWin);
  
  // broadcast the message down to the individual LOD mappers
  for ( this->LODMappers->InitTraversal();
	(mapper = this->LODMappers->GetNextItem()); )
    {
    mapper->ReleaseGraphicsResources(renWin);
    }
}


//----------------------------------------------------------------------------
// does not matter if mapper is in mapper collection.
void vtkLODActor::AddLODMapper(vtkMapper *mapper)
{
  if (this->MediumMapper)
    {
    this->DeleteOwnLODs();
    }
  
  if (this->Mapper == NULL)
    {
    this->SetMapper(mapper);
    }
  
  this->LODMappers->AddItem(mapper);
}


//----------------------------------------------------------------------------
// Can only be used if no LOD mappers have been added.
// Maybe we should remove this exculsive feature.
void vtkLODActor::CreateOwnLODs()
{
  int num;

  if (this->MediumMapper)
    {
    return;
    }
  
  if ( this->Mapper == NULL)
    {
    vtkErrorMacro("Cannot create LODs with out a mapper.");
    return;
    }
  
  // There are ways of getting arround this limitation ...
  num = this->LODMappers->GetNumberOfItems();
  if (num > 0)
    {
    vtkErrorMacro(<<
	  "Cannot generate LOD mappers when some have been added already");
    return;
    }
  
  // create filters and mappers
  this->MaskPoints = vtkMaskPoints::New();
  this->MaskPoints->RandomModeOn();
  this->MaskPoints->GenerateVerticesOn();
  this->OutlineFilter = vtkOutlineFilter::New();
  this->LowMapper = vtkPolyDataMapper::New();
  this->MediumMapper = vtkPolyDataMapper::New();
  
  // connect the filters
  this->MediumMapper->SetInput(this->MaskPoints->GetOutput());
  this->LowMapper->SetInput(this->OutlineFilter->GetOutput());
  this->LODMappers->AddItem(this->MediumMapper);
  this->LODMappers->AddItem(this->LowMapper);
  
  this->UpdateOwnLODs();
}


//----------------------------------------------------------------------------
void vtkLODActor::UpdateOwnLODs()
{
  if ( this->Mapper == NULL)
    {
    vtkErrorMacro("Cannot create LODs with out a mapper.");
    return;
    }

  if (this->MediumMapper == NULL)
    {
    this->CreateOwnLODs();
    if (this->MediumMapper == NULL)
      { // could not create the LODs
      return;
      }
    }
  
  // connect the filters to the mapper, and set parameters
  this->MaskPoints->SetInput(this->Mapper->GetInput());
  this->MaskPoints->SetMaximumNumberOfPoints(this->NumberOfCloudPoints);
  this->OutlineFilter->SetInput(this->Mapper->GetInput());
  this->MediumMapper->SetScalarRange(this->Mapper->GetScalarRange());
  this->MediumMapper->SetScalarVisibility(this->Mapper->GetScalarVisibility());
  
  this->BuildTime.Modified();
}


//----------------------------------------------------------------------------
// Deletes Mappers and filters created by this object.
// (number two and three)
void vtkLODActor::DeleteOwnLODs()
{
  if (this->MediumMapper == NULL)
    {
    return;
    }

  // remove the mappers from the LOD collection
  this->LODMappers->RemoveItem(this->LowMapper);
  this->LODMappers->RemoveItem(this->MediumMapper);
  
  // delete the filters used to create the LODs ...
  // The NULL check should not be necessary, but for sanity ...
  this->MaskPoints->Delete();
  this->MaskPoints = NULL;
  this->OutlineFilter->Delete();
  this->OutlineFilter = NULL;
  this->LowMapper->Delete();
  this->LowMapper = NULL;
  this->MediumMapper->Delete();
  this->MediumMapper = NULL;
}



//----------------------------------------------------------------------------
void vtkLODActor::Modified()
{
  this->Device->Modified();
  this->vtkActor::Modified();
}

void vtkLODActor::ShallowCopy(vtkProp *prop)
{
  vtkLODActor *a = vtkLODActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetNumberOfCloudPoints(a->GetNumberOfCloudPoints());
    vtkMapperCollection *c = a->GetLODMappers();
    vtkMapper *map;
    for ( c->InitTraversal(); (map=c->GetNextItem()); )
      {
      this->AddLODMapper(map);
      }
    }

  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}

