/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkLODActor.cxx
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
#include <stdlib.h>
#include <math.h>
#include "vtkLODActor.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkLODActor::vtkLODActor()
{
  // get a hardware dependent actor and mappers
  this->Device = vtkActor::New();
  this->LODMappers = vtkMapperCollection::New();
  this->SelfCreatedLODs = 0;
  // stuff for creating own LODs
  this->PointSource = NULL;
  this->MaskPoints = NULL;
  this->Glyph3D = NULL;
  this->OutlineFilter = NULL;
  this->NumberOfCloudPoints = 150;
}

//----------------------------------------------------------------------------
vtkLODActor::~vtkLODActor()
{
  this->Device->Delete();
  this->DeleteSelfCreatedLODs();
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
void vtkLODActor::Render(vtkRenderer *ren)
{
  float myTime, bestTime, tempTime;
  double aTime;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  vtkMapper *mapper, *bestMapper;
  
  // first time through create lods if non have been added
  if (this->LODMappers->GetNumberOfItems() == 0)
    {
    this->CreateLODs();
    }
  
  
  // If the actor has changed or the primary mapper has changed ...
  // Is this the correct test?
  if (this->SelfCreatedLODs)
    {
    if (this->GetMTime() > this->BuildTime || 
	this->Mapper->GetMTime() > this->BuildTime)
      {
      this->CreateLODs();
      }
    }
  
  // put culling here for now. (to test set inside frustrum)
  if (this->AllocatedRenderTime == 0.0)
    {
    return;
    }
  
  // figure out how much time we have to render
  myTime = this->AllocatedRenderTime;

  // cerr << "AllocatedRenderTime: " << myTime << endl;
  
  //   Figure out which resolution to use 
  // none is a valid resolution. Do we want to have a lowest:
  // bbox, single point, ...
  //   There is no order to the list, so it is assumed that mappers that take
  // longer to render are better quality.
  //   Timings might become out of date, but we rely on 

  bestMapper = this->Mapper;
  bestTime = bestMapper->GetRenderTime();
  if (bestTime > myTime)
    {
    this->LODMappers->InitTraversal();
    while ((mapper = this->LODMappers->GetNextItem()) != NULL && bestTime != 0.0)
      {
      tempTime = mapper->GetRenderTime();
      
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
    
  // record start rendering time
  aTime = vtkTimerLog::GetCurrentTime();
  
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
  if (this->Texture) this->Texture->Render(ren);
  
  // make sure the device has the same matrix
  this->GetMatrix(matrix);
  this->Device->SetUserMatrix(matrix);
  
  // Store information on time it takes to render.
  // We might want to estimate time from the number of polygons in mapper.
  this->Device->Render(ren,bestMapper);
  // Aborted render will give incorrect times
  if (!(ren->GetRenderWindow()->GetAbortRender()))
    {
    myTime = (float)(vtkTimerLog::GetCurrentTime() - aTime);
    // combine time with a moving average
    if (bestTime == 0.0)
      { // This is the first render. 
      bestMapper->SetRenderTime(myTime);
      }
    else
      { // Running average of render time as a temporary fix for
      // openGL buffering.  The only problem is that the first render takes
      // a long time, so unless forced renders are frequent, 
      // an LOD can be locked out.
      bestMapper->SetRenderTime(0.2 * myTime + 0.8 * bestTime);
      }
    }

  matrix->Delete();
}

      
//----------------------------------------------------------------------------
// does not matter if mapper is in mapper collection.
void vtkLODActor::AddLODMapper(vtkMapper *mapper)
{
  if (this->SelfCreatedLODs)
    {
    this->DeleteSelfCreatedLODs();
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
void vtkLODActor::CreateLODs()
{
  int num;

  if (this->SelfCreatedLODs)
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
    vtkErrorMacro(
	  "Cannot generate LOD mappers when some have been added already");
    return;
    }
  
  // create filters and mappers
  this->PointSource = vtkPointSource::New();
  this->Glyph3D = vtkGlyph3D::New();
  this->MaskPoints = vtkMaskPoints::New();
  this->OutlineFilter = vtkOutlineFilter::New();
  this->LowMapper = vtkPolyDataMapper::New();
  this->MediumMapper = vtkPolyDataMapper::New();
  
  // connect the filters
  this->Glyph3D->SetInput(this->MaskPoints->GetOutput());
  this->Glyph3D->SetSource(this->PointSource->GetOutput());
  this->MediumMapper->SetInput(this->Glyph3D->GetOutput());
  this->LowMapper->SetInput(this->OutlineFilter->GetOutput());
  this->AddLODMapper(this->MediumMapper);
  this->AddLODMapper(this->LowMapper);
  
  this->SelfCreatedLODs = 1;
  this->UpdateSelfCreatedLODs();
}


//----------------------------------------------------------------------------
void vtkLODActor::UpdateSelfCreatedLODs()
{
  if ( this->Mapper == NULL)
    {
    vtkErrorMacro("Cannot create LODs with out a mapper.");
    return;
    }

  if ( ! this->SelfCreatedLODs)
    {
    this->CreateLODs();
    if ( ! this->SelfCreatedLODs)
      { // could not create the LODs
      return;
      }
    }
  
  // connect the filters to the mapper, and set parameters
  this->PointSource->SetRadius(0);
  this->PointSource->SetNumberOfPoints(1);
  this->MaskPoints->SetInput(this->Mapper->GetInput());
  this->MaskPoints->SetMaximumNumberOfPoints(this->NumberOfCloudPoints);
  this->MaskPoints->SetRandomMode(1);
  this->OutlineFilter->SetInput(this->Mapper->GetInput());
  this->MediumMapper->SetScalarRange(this->Mapper->GetScalarRange());
  this->MediumMapper->SetScalarVisibility(this->Mapper->GetScalarVisibility());
  
  this->BuildTime.Modified();
}


//----------------------------------------------------------------------------
// Deletes Mappers and filters created by this object.
// (number two and three)
void vtkLODActor::DeleteSelfCreatedLODs()
{
  if ( ! this->SelfCreatedLODs)
    {
    return;
    }

  // remove the mappers from the LOD collection
  this->LODMappers->RemoveItem(this->LowMapper);
  this->LODMappers->RemoveItem(this->MediumMapper);
  
  // delete the filters used to create the LODs ...
  // The NULL check should not be necessary, but for sanity ...
  this->PointSource->Delete();
  this->PointSource = NULL;
  this->Glyph3D->Delete();
  this->Glyph3D = NULL;
  this->MaskPoints->Delete();
  this->MaskPoints = NULL;
  this->OutlineFilter->Delete();
  this->OutlineFilter = NULL;
  this->LowMapper->Delete();
  this->LowMapper = NULL;
  this->MediumMapper->Delete();
  this->MediumMapper = NULL;
  
  this->SelfCreatedLODs = 0;
}

