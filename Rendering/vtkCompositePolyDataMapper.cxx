/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositePolyDataMapper.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkCompositePolyDataMapper);

class vtkCompositePolyDataMapperInternals
{
public:
  vtkstd::vector<vtkPolyDataMapper*> Mappers;
};

vtkCompositePolyDataMapper::vtkCompositePolyDataMapper()
{
  this->Internal = new vtkCompositePolyDataMapperInternals;
}

vtkCompositePolyDataMapper::~vtkCompositePolyDataMapper()
{
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->UnRegister(this);
    }
  this->Internal->Mappers.clear();
  
  delete this->Internal;
}

// Specify the type of data this mapper can handle. If we are
// working with a regular (not hierarchical) pipeline, then we
// need vtkPolyData. For composite data pipelines, then
// vtkCompositeDataSet is required, and we'll check when
// building our structure whether all the part of the composite
// data set are polydata.
int vtkCompositePolyDataMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}    

// When the structure is out-of-date, recreate it by
// creating a mapper for each input data.
void vtkCompositePolyDataMapper::BuildPolyDataMapper()
{
  int warnOnce = 0;
  
  //Delete pdmappers if they already exist.
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->UnRegister(this);
    }
  this->Internal->Mappers.clear();
  
  //Get the composite dataset from the input
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  // If it isn't hierarchical, maybe it is just a plain vtkPolyData
  if(!input) 
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));
    if ( pd )
      {
      // Make a copy of the data to break the pipeline here
      vtkPolyData *newpd = vtkPolyData::New();
      newpd->ShallowCopy(pd);
      vtkPolyDataMapper *pdmapper = this->MakeAMapper();
      pdmapper->Register( this );
      pdmapper->SetInput(newpd);
      this->Internal->Mappers.push_back(pdmapper);
      newpd->Delete();
      pdmapper->Delete();
      }
    else
      {
      vtkDataObject* tmpInp = this->GetExecutive()->GetInputData(0, 0);
      vtkErrorMacro("This mapper cannot handle input of type: "
                    << (tmpInp?tmpInp->GetClassName():"(none)"));
      }
    }
  else
    {
    //for each data set build a vtkPolyDataMapper
    vtkCompositeDataIterator* iter = input->NewIterator();
    iter->GoToFirstItem();  
    while (!iter->IsDoneWithTraversal())
      {
      vtkPolyData* pd = 
        vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());      
      if (pd)
        {
        // Make a copy of the data to break the pipeline here
        vtkPolyData *newpd = vtkPolyData::New();
        newpd->ShallowCopy(pd);
        vtkPolyDataMapper *pdmapper = this->MakeAMapper();
        pdmapper->Register(this);
        pdmapper->SetInput(newpd);
        this->Internal->Mappers.push_back(pdmapper);
        newpd->Delete();
        pdmapper->Delete();
        }
      // This is not polydata - warn the user that there are non-polydata
      // parts to this data set which will not be rendered by this mapper
      else
        {
        if ( !warnOnce )
          {
          vtkErrorMacro("All data in the hierarchical dataset must be polydata.");
          warnOnce = 1;
          }
        }
      iter->GoToNextItem();
      }
    iter->Delete();
    }
  
  this->InternalMappersBuildTime.Modified();
  
}

void vtkCompositePolyDataMapper::Render(vtkRenderer *ren, vtkActor *a)
{
  //If the PolyDataMappers are not up-to-date then rebuild them
  vtkCompositeDataPipeline * executive = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
  
  if(executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
    {
    this->BuildPolyDataMapper();    
    }
  
  this->TimeToDraw = 0;
  //Call Render() on each of the PolyDataMappers
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    if ( this->ClippingPlanes != 
         this->Internal->Mappers[i]->GetClippingPlanes() )
      {
      this->Internal->Mappers[i]->SetClippingPlanes( this->ClippingPlanes );
      }
    
    this->Internal->Mappers[i]->SetLookupTable(
      this->GetLookupTable());
    this->Internal->Mappers[i]->SetScalarVisibility(
      this->GetScalarVisibility());
    this->Internal->Mappers[i]->SetUseLookupTableScalarRange(
      this->GetUseLookupTableScalarRange());
    this->Internal->Mappers[i]->SetScalarRange(
      this->GetScalarRange());
    this->Internal->Mappers[i]->SetImmediateModeRendering(
      this->GetImmediateModeRendering());
    this->Internal->Mappers[i]->SetColorMode(this->GetColorMode());
    this->Internal->Mappers[i]->SetInterpolateScalarsBeforeMapping(
      this->GetInterpolateScalarsBeforeMapping());

    this->Internal->Mappers[i]->SetScalarMode(this->GetScalarMode());
    if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
         this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
      {
      if ( this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID )
        {
        this->Internal->Mappers[i]->ColorByArrayComponent(
          this->ArrayId,ArrayComponent);
        }
      else
        {
        this->Internal->Mappers[i]->ColorByArrayComponent(
          this->ArrayName,ArrayComponent);
        }
      }
  
    this->Internal->Mappers[i]->Render(ren,a);    
    this->TimeToDraw += this->Internal->Mappers[i]->GetTimeToDraw();
    }
}
vtkExecutive* vtkCompositePolyDataMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//Looks at each DataSet and finds the union of all the bounds
void vtkCompositePolyDataMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);
  
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if(!input) 
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));
    if ( pd )
      {
      pd->GetBounds( this->Bounds );
      }
    return;
    }
  
  // We do have hierarchical data - so we need to loop over
  // it and get the total bounds.
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();  
  double bounds[6];
  int i;
  
  while (!iter->IsDoneWithTraversal())
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());    
    if (pd)
      {
      // If this isn't the first time through, expand bounds
      // we've compute so far based on the bounds of this
      // block
      if ( vtkMath::AreBoundsInitialized(this->Bounds) )
        {
        pd->GetBounds(bounds);
        for(i=0; i<3; i++)
          {
          this->Bounds[i*2] = 
            (bounds[i*2]<this->Bounds[i*2])?
            (bounds[i*2]):(this->Bounds[i*2]);
          this->Bounds[i*2+1] = 
            (bounds[i*2+1]>this->Bounds[i*2+1])?
            (bounds[i*2+1]):(this->Bounds[i*2+1]);
          }
        }
      // If this is our first time through, just get the bounds
      // of the data as the initial bounds
      else
        {
        pd->GetBounds(this->Bounds);
        }
      }
    iter->GoToNextItem();
    }
  iter->Delete();
  this->BoundsMTime.Modified();
}

double *vtkCompositePolyDataMapper::GetBounds()
{ 
  if ( ! this->GetExecutive()->GetInputData(0, 0) ) 
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
    }
  else
    {
    this->Update();
    
    //only compute bounds when the input data has changed
    vtkCompositeDataPipeline * executive = vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if( executive->GetPipelineMTime() > this->BoundsMTime.GetMTime() )
      {
      this->ComputeBounds();
      }
    
    return this->Bounds;
    }
}

void vtkCompositePolyDataMapper::ReleaseGraphicsResources( vtkWindow *win )
{
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->ReleaseGraphicsResources( win );
    }
}

void vtkCompositePolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

vtkPolyDataMapper *vtkCompositePolyDataMapper::MakeAMapper()
{
  return vtkPolyDataMapper::New();
}
