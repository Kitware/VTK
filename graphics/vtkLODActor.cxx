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
  this->Mappers = vtkMapperCollection::New();
  this->BuildLODs = 1;
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
  if (this->BuildLODs)
    {
    this->DeleteMappers();
    }
  this->Mappers->Delete();

  // delete the filters used to create the LODs
  if (this->PointSource)
    {
    this->PointSource->Delete();
    this->PointSource = NULL;
    }
  if (this->Glyph3D)
    {
    this->Glyph3D->Delete();
    this->Glyph3D = NULL;
    }
  if (this->MaskPoints)
    {
    this->MaskPoints->Delete();
    this->MaskPoints = NULL;
    }
  if (this->OutlineFilter)
    {
    this->OutlineFilter->Delete();
    this->OutlineFilter = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);

  os << indent << "Cloud Points: " << this->NumberOfCloudPoints << "\n";

  os << indent << "BuildLODs: " << this->BuildLODs << endl;
  // how should we print out the mappers?
  os << indent << "NumberOfMappers: " << this->Mappers->GetNumberOfItems() 
     << endl;
}


//----------------------------------------------------------------------------
void vtkLODActor::Render(vtkRenderer *ren)
{
  float myTime, bestTime, tempTime;
  double aTime;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  vtkMapper *mapper, *bestMapper;
  

  // has anything changed ???
  if (this->BuildLODs)
    {
    if (this->GetMTime() > this->BuildTime || 
	this->Mapper->GetMTime() > this->BuildTime)
      {
      this->GenerateLODs();
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
  // cerr << "  Start (" << bestMapper << ") with time: " << bestTime << endl;
  this->Mappers->InitTraversal();
  while ((mapper = this->Mappers->GetNextItem()) != NULL && bestTime != 0.0)
    {
    tempTime = mapper->GetRenderTime();
    // cerr << "    Mapper (" << mapper << ") RenderTime: " << tempTime <<endl;

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
  this->GetMatrix(*matrix);
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
      // cerr << "          SETTING (" << bestMapper << ") RenderTime to " << myTime << endl;
      }
    else
      { // Running average of render time as a temporary fix for
      // openGL buffering.
      bestMapper->SetRenderTime(0.1 * myTime + 0.9 * bestTime);
      }
    }

  delete matrix;
}

      
//----------------------------------------------------------------------------
// does not matter if mapper is in mapper collection.
void vtkLODActor::AddMapper(vtkMapper *mapper)
{
  if (this->Mapper == NULL)
    {
    this->SetMapper(mapper);
    }
  
  this->Mappers->AddItem(mapper);
}


//----------------------------------------------------------------------------
void vtkLODActor::SetBuildLODs(int val)
{
  if (( ! val &&  ! this->BuildLODs) || (val && this->BuildLODs))
    {
    return;
    }
  
  this->Modified();
  this->BuildLODs = val;
  
  if (val)
    { // user now wants this object to manage LOD mappers
    this->Mappers->RemoveAllItems();
    this->GenerateLODs();
    }
  else
    { // delete mappers created by this object.
    this->DeleteMappers();
    // delete the filters too?
    }
}

//----------------------------------------------------------------------------
// Deletes Mappers and there input data.
void vtkLODActor::GenerateLODs()
{
  vtkPolyDataMapper *mediumMapper, *lowMapper;
  
  // cerr << "-------- Building LODs\n";
  
  if ( this->Mapper == NULL)
    {
    vtkErrorMacro("Cannot create LODs with out a mapper.");
    return;
    }

  // delete the old mappers
  this->DeleteMappers();
  
  // create filters if necessary
  if (this->PointSource == NULL)
    {
    this->PointSource = vtkPointSource::New();
    }
  if (this->Glyph3D == NULL)
    {
    this->Glyph3D = vtkGlyph3D::New();
    }
  if (this->MaskPoints == NULL)
    {
    this->MaskPoints = vtkMaskPoints::New();
    }
  if (this->OutlineFilter == NULL)
    {
    this->OutlineFilter = vtkOutlineFilter::New();
    }
  
  // connect the filters
  this->PointSource->SetRadius(0);
  this->PointSource->SetNumberOfPoints(1);
  this->MaskPoints->SetInput(this->Mapper->GetInput());
  this->MaskPoints->SetMaximumNumberOfPoints(this->NumberOfCloudPoints);
  this->MaskPoints->SetRandomMode(1);
  this->Glyph3D->SetInput(this->MaskPoints->GetOutput());
  this->Glyph3D->SetSource(this->PointSource->GetOutput());
  this->OutlineFilter->SetInput(this->Mapper->GetInput());
  
  mediumMapper = vtkPolyDataMapper::New();
  lowMapper = vtkPolyDataMapper::New();
  
  mediumMapper->SetInput(this->Glyph3D->GetOutput());
  mediumMapper->SetScalarRange(this->Mapper->GetScalarRange());
  mediumMapper->SetScalarVisibility(this->Mapper->GetScalarVisibility());
  lowMapper->SetInput(this->OutlineFilter->GetOutput());

  this->AddMapper(mediumMapper);
  this->AddMapper(lowMapper);
  
  this->BuildTime.Modified();
}


//----------------------------------------------------------------------------
// Deletes Mappers and filters used when creating own LODs.
void vtkLODActor::DeleteMappers()
{
  vtkMapper *mapper;
  
  this->Mappers->InitTraversal();
  while ((mapper = this->Mappers->GetNextItem()))
    {
    // deleting and then removing scares me so InitTraversal each time.
    this->Mappers->RemoveItem(mapper);
    this->Mappers->InitTraversal();
    mapper->Delete();
    }  
}

