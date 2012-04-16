/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeToUniformGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeToUniformGridFilter.h"

#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkCellArray.h"
#include <assert.h>
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkStreamingDemandDrivenPipeline.h" // WHOLE_EXTENT() key

vtkStandardNewMacro(vtkHyperOctreeToUniformGridFilter);

// merging: locator
// no merging: insertnextpoint

//----------------------------------------------------------------------------
vtkHyperOctreeToUniformGridFilter::vtkHyperOctreeToUniformGridFilter()
{
  this->InputCD=0;
  this->OutputCD=0;
  this->Cursor=0;
  this->YExtent=1;
  this->ZExtent=1;
  this->Output=0;
}

//----------------------------------------------------------------------------
vtkHyperOctreeToUniformGridFilter::~vtkHyperOctreeToUniformGridFilter()
{
}

//----------------------------------------------------------------------------
int vtkHyperOctreeToUniformGridFilter::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  int levels=inInfo->Get(vtkHyperOctree::LEVELS());

  double size[3];
  inInfo->Get(vtkHyperOctree::SIZES(),size);
  double origin[3];
  inInfo->Get(vtkDataObject::ORIGIN(),origin);

  int dim=inInfo->Get(vtkHyperOctree::DIMENSION());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

   // Set dimensions, spacing and origin for the uniform grid.
  int resolutions[3];
  double spacing[3];
  resolutions[0]=(1<<(levels-1))+1;
  assert("check: min_is_2" && resolutions[0]>=2);
  spacing[0]=size[0]/(resolutions[0]-1);

  if(dim>=2)
    {
    resolutions[1]=resolutions[0];
    spacing[1]=size[1]/(resolutions[1]-1);
    this->YExtent=2;
    }
  else
    {
    resolutions[1]=1;
    spacing[1]=0;
    this->YExtent=1;
    }
  if(dim==3)
    {
    resolutions[2]=resolutions[0];
    spacing[2]=size[2]/(resolutions[2]-1);
    this->ZExtent=2;
    }
  else
    {
    resolutions[2]=1;
    spacing[2]=0;
    this->ZExtent=1;
    }

  outInfo->Set(vtkDataObject::SPACING(),spacing,3);
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);

  int extent[6];
  extent[0]=0;
  extent[1]=resolutions[0]-1;
  extent[2]=0;
  extent[3]=resolutions[1]-1;
  extent[4]=0;
  extent[5]=resolutions[2]-1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeToUniformGridFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the upper limit for the number of levels
  int levels=inInfo->Get(vtkHyperOctree::LEVELS());

  // get the input and output
  vtkHyperOctree *input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  assert("check:valid_levels" && levels>=input->GetNumberOfLevels());

  this->Output=output;
  this->InputCD = input->GetPointData();
  this->OutputCD = output->GetCellData();

  int dim=input->GetDimension();
  assert("check: valid_dim" && dim>=1 && dim<=3);

  // Set dimensions, spacing and origin for the uniform grid.
  int resolutions[3];
  double spacing[3];
  cout<<"levels="<<levels<<endl;
  cout<<"inputlevels="<<input->GetNumberOfLevels()<<endl;
  resolutions[0]=(1<<(levels-1))+1;
  assert("check: min_is_2" && resolutions[0]>=2);
  spacing[0]=input->GetSize()[0]/(resolutions[0]-1);

  if(dim>=2)
    {
    resolutions[1]=resolutions[0];
    spacing[1]=input->GetSize()[1]/(resolutions[1]-1);
    this->YExtent=2;
    }
  else
    {
    resolutions[1]=1;
    spacing[1]=0;
    this->YExtent=1;
    }
  if(dim==3)
    {
    resolutions[2]=resolutions[0];
    spacing[2]=input->GetSize()[2]/(resolutions[2]-1);
    this->ZExtent=2;
    }
  else
    {
    resolutions[2]=1;
    spacing[2]=0;
    this->ZExtent=1;
    }
  output->SetDimensions(resolutions);
  output->SetSpacing(spacing);

  output->SetOrigin(input->GetOrigin());

  // Check if our computation is correct.
  cout<<"output="<<output->GetNumberOfPoints()<<endl;
  cout<<"maxinput="<<input->GetMaxNumberOfPoints(0)<<endl;
  assert("check: valid_number_of_points" && output->GetNumberOfPoints()>=input->GetMaxNumberOfPoints(0)); // not equal if LEVELS()>GetNumberOfLevels()
  assert("check valid_y_extent" && (this->YExtent==1 || this->YExtent==2));
  assert("check valid_z_extent" && (this->ZExtent==1 || this->ZExtent==2));
  //A=>B: not A or B
  // yextent==1 => zextent==1
  assert("check valid_z_extent2" && (this->YExtent!=1 || this->ZExtent==1));
  // zextent==2 => yextent==2
  assert("check valid_z_extent3" && (this->ZExtent!=2 || this->YExtent==2));

  cout<<"number of cells="<<output->GetNumberOfCells()<<endl;

  // Prepare copy for cell data.
  this->OutputCD->CopyAllocate(this->InputCD,output->GetNumberOfCells());

  // Copy cell data recursively
  this->Cursor=input->NewCellCursor();
  this->Cursor->ToRoot();
  int extent[6];
  output->GetExtent(extent);
  // the given extent is point-based, we want a cell-based extent:
  if(extent[1]>0)
    {
    extent[1]=extent[1]-1;
    }
  if(extent[3]>0)
    {
    extent[3]=extent[3]-1;
    }
  if(extent[5]>0)
    {
    extent[5]=extent[5]-1;
    }
  this->CopyCellData(extent);
  this->Cursor->UnRegister(this);
  this->Cursor=0;
  this->InputCD=0;
  this->OutputCD=0;
  this->Output=0;

  assert("post: valid_output" && output->CheckAttributes()==0);

  return 1;
}
//-----------------------------------------------------------------------------
void vtkHyperOctreeToUniformGridFilter::CopyCellData(int cellExtent[6])
{
  assert("pre: valid_xextent" && cellExtent[0]<=cellExtent[1]);
  assert("pre: valid_yextent" && cellExtent[2]<=cellExtent[3]);
  assert("pre: valid_zextent" && cellExtent[4]<=cellExtent[5]);

  if(this->Cursor->CurrentIsLeaf())
    {
    vtkIdType inId=this->Cursor->GetLeafId();
    int ijk[3];
    ijk[2]=cellExtent[4];

#ifndef NDEBUG
    int atLeastOne=0;
#endif

    while(ijk[2]<=cellExtent[5]) // k
      {
       ijk[1]=cellExtent[2];
       while(ijk[1]<=cellExtent[3]) // j
         {
         ijk[0]=cellExtent[0];
         while(ijk[0]<=cellExtent[1]) // i
           {
#ifndef NDEBUG
           atLeastOne=1;
#endif
           vtkIdType outId=this->Output->ComputeCellId(ijk);
           this->OutputCD->CopyData(this->InputCD,inId,outId);
           ++ijk[0];
           }
         ++ijk[1];
         }
      ++ijk[2];
      }
    assert("check: make sure we entered into the loop" && atLeastOne);
    }
  else
    {
    // traverse children (zi|yi|xi)
    int zmid=(cellExtent[4]+cellExtent[5])>>1; // /2
    int ymid=(cellExtent[2]+cellExtent[3])>>1; // /2
    int xmid=(cellExtent[0]+cellExtent[1])>>1; // /2

    int newExtent[6];
    int zi=0;
    int zchild=0;
    newExtent[4]=cellExtent[4];
    newExtent[5]=zmid;
    while(zi<this->ZExtent)
      {
      int yi=0;
      int ychild=zchild;
      newExtent[2]=cellExtent[2];
      newExtent[3]=ymid;
      while(yi<this->YExtent)
        {
        int xi=0;
        int child=ychild;
        newExtent[0]=cellExtent[0];
        newExtent[1]=xmid;
        while(xi<2)
          {
          this->Cursor->ToChild(child);
          this->CopyCellData(newExtent);
          this->Cursor->ToParent();
          ++child;
          ++xi;
          newExtent[0]=xmid+1;
          newExtent[1]=cellExtent[1];
          }
        ++yi;
        ychild+=2;
        newExtent[2]=ymid+1;
        newExtent[3]=cellExtent[3];
        }
      ++zi;
      zchild+=4;
      newExtent[4]=zmid+1;
      newExtent[5]=cellExtent[5];
      }
    }
}

//-----------------------------------------------------------------------------
int vtkHyperOctreeToUniformGridFilter::FillInputPortInformation(int,
                                                          vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeToUniformGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
