/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLODActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLODActor.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"

#include <stdlib.h>
#include <math.h>

vtkCxxRevisionMacro(vtkLODActor, "1.59");
vtkStandardNewMacro(vtkLODActor);

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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cloud Points: " << this->NumberOfCloudPoints << endl;

  // how should we print out the LODMappers?
  os << indent << "Number Of LOD Mappers: " 
     << this->LODMappers->GetNumberOfItems() << endl;
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

  // Figure out which resolution to use 
  // none is a valid resolution. Do we want to have a lowest:
  // bbox, single point, ...
  // There is no order to the list, so it is assumed that mappers that take
  // longer to render are better quality.
  // Timings might become out of date, but we rely on 

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
        bestMapper = mapper;
        bestTime = 0.0;
        }
      else
        {
        if (bestTime > myTime && tempTime < bestTime)
          {
          bestMapper = mapper;
          bestTime = tempTime;
          }
        if (tempTime > bestTime && tempTime < myTime)
          { 
          bestMapper = mapper;
          bestTime = tempTime;
          }
        }
      }
    }
    
  // render the property
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
  
  
  // render the texture
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
  if ( this->LODMappers->GetNumberOfItems() > 0 )
    {
    vtkErrorMacro(<<
          "Cannot generate LOD mappers when some have been added already");
    return;
    }
  
  // create filters and mappers
  this->MaskPoints = vtkMaskPoints::New();
  this->MaskPoints->RandomModeOn();
  this->MaskPoints->GenerateVerticesOn();
  this->MediumMapper = vtkPolyDataMapper::New();

  this->OutlineFilter = vtkOutlineFilter::New();
  this->LowMapper = vtkPolyDataMapper::New();

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
  
  // copy all parameters including LUTs, scalar range, etc.
  this->MediumMapper->ShallowCopy(this->Mapper);
  this->MediumMapper->SetInput(this->MaskPoints->GetOutput());
  this->LowMapper->ShallowCopy(this->Mapper);
  this->LowMapper->ScalarVisibilityOff();
  this->LowMapper->SetInput(this->OutlineFilter->GetOutput());

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

