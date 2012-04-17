/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeSampleFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeSampleFunction.h"

#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include <assert.h>
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkImplicitFunction.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkHyperOctreeSampleFunction);
vtkCxxSetObjectMacro(vtkHyperOctreeSampleFunction,ImplicitFunction,
                     vtkImplicitFunction);

//----------------------------------------------------------------------------
vtkHyperOctreeSampleFunction::vtkHyperOctreeSampleFunction()
{
  this->SetNumberOfInputPorts(0);
  this->Dimension=3;
  int i=0;
  while(i<3)
    {
    this->Size[i]=1;
    this->Origin[i]=0;
    ++i;
    }
  this->Levels=5;
  this->MinLevels=1;
  this->ImplicitFunction=0;
  this->OutputScalarType=VTK_DOUBLE;
  this->Threshold=0.1;
}

//----------------------------------------------------------------------------
vtkHyperOctreeSampleFunction::~vtkHyperOctreeSampleFunction()
{
  this->SetImplicitFunction(0);
}

//----------------------------------------------------------------------------
// Description:
// Return the maximum number of levels of the hyperoctree.
// \post positive_result: result>=1
int vtkHyperOctreeSampleFunction::GetLevels()
{
  assert("post: positive_result" && this->Levels>=1);
  return this->Levels;
}

//----------------------------------------------------------------------------
// Description:
// Set the maximum number of levels of the hyperoctree. If
// GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
// \pre positive_levels: levels>=1
// \post is_set: this->GetLevels()==levels
// \post min_is_valid: this->GetMinLevels()<this->GetLevels()
void vtkHyperOctreeSampleFunction::SetLevels(int levels)
{
  assert("pre: positive_levels" && levels>=1);
  this->Levels=levels;
  if(this->MinLevels>=levels)
    {
    this->MinLevels=levels-1;
    }

  assert("post: is_set" && this->GetLevels()==levels);
  assert("post: min_is_valid" && this->GetMinLevels()<this->GetLevels());
}


//----------------------------------------------------------------------------
// Description:
// Return the minimal number of levels of systematic subdivision.
// \post positive_result: result>=0
int vtkHyperOctreeSampleFunction::GetMinLevels()
{
  assert("post: positive_result" && this->MinLevels>=0);
  return this->MinLevels;
}

//----------------------------------------------------------------------------
// Description:
// Set the minimal number of levels of systematic subdivision.
// \pre positive_minLevels: minLevels>=0 && minLevels<this->GetLevels()
// \post is_set: this->GetMinLevels()==minLevels
void vtkHyperOctreeSampleFunction::SetMinLevels(int minLevels)
{
  assert("pre: positive_minLevels" && minLevels>=0 && minLevels<this->GetLevels());
  this->MinLevels=minLevels;
  assert("post: is_set" && this->GetMinLevels()==minLevels);
}

//----------------------------------------------------------------------------
// Description:
// Return the threshold over which a subdivision is required.
// \post positive_result: result>0
double vtkHyperOctreeSampleFunction::GetThreshold()
{
  assert("post: positive_result" && this->Threshold>0);
  return this->Threshold;
}

//----------------------------------------------------------------------------
// Description:
// Set the threshold over which a subdivision is required.
// \pre positive_threshold: threshold>=0
// \post is_set: this->GetThreshold()==threshold
void vtkHyperOctreeSampleFunction::SetThreshold(double threshold)
{
  assert("pre: positive_threshold" && threshold>=0);
  this->Threshold=threshold;
  assert("post: is_set" && this->GetThreshold()==threshold);
}

//-----------------------------------------------------------------------------
// Description:
// Return the dimension of the tree (1D:binary tree(2 children), 2D:quadtree
// (4 children), 3D:octree (8 children))
// \post valid_result: result>=1 && result<=3
int vtkHyperOctreeSampleFunction::GetDimension()
{
  assert("post: valid_result" && this->Dimension>=1 && this->Dimension<=3);
  return this->Dimension;
}

//-----------------------------------------------------------------------------
// Set the dimension of the tree with `dim'. See GetDimension() for details.
// \pre valid_dim: dim>=1 && dim<=3
// \post dimension_is_set: GetDimension()==dim
void vtkHyperOctreeSampleFunction::SetDimension(int dim)
{
  assert("pre: valid_dim" && dim>=1 && dim<=3);
  if(this->Dimension!=dim)
    {
    this->Dimension=dim;
    this->Modified();
    }
  assert("post: dimension_is_set" && this->GetDimension()==dim);
}

//-----------------------------------------------------------------------------
// Description:
// Return the length along the x-axis.
// \post positive_result: result>0
double vtkHyperOctreeSampleFunction::GetWidth()
{
  assert("post: positive_result" && this->Size[0]>0);
  return this->Size[0];
}

//-----------------------------------------------------------------------------
// Description:
// Set the length along the x-axis.
// \pre positive_width: width>0
// \post width_is_set: GetWidth()==width
void vtkHyperOctreeSampleFunction::SetWidth(double width)
{
  assert("pre: positive_width" && width>0);
  if(this->Size[0]!=width)
    {
    this->Size[0]=width;
    this->Modified();
    }
  assert("post: width_is_set" && this->GetWidth()==width);
}

//-----------------------------------------------------------------------------
// Description:
// Return the length along the y-axis.
// Relevant only if GetDimension()>=2
// \post positive_result: result>0
double vtkHyperOctreeSampleFunction::GetHeight()
{
  assert("post: positive_result" && this->Size[1]>0);
  return this->Size[1];
}

//-----------------------------------------------------------------------------
// Description:
// Set the length along the y-axis.
// Relevant only if GetDimension()>=2
// \pre positive_height: height>0
// \post height_is_set: GetHeight()==height
void vtkHyperOctreeSampleFunction::SetHeight(double height)
{
  assert("pre: positive_height" && height>0);
  if(this->Size[1]!=height)
    {
    this->Size[1]=height;
    this->Modified();
    }
  assert("post: height_is_set" && this->GetHeight()==height);
}


//-----------------------------------------------------------------------------
// Description:
// Return the length along the z-axis.
// Relevant only if GetDimension()>=3
// \post positive_result: result>0
double vtkHyperOctreeSampleFunction::GetDepth()
{
  assert("post: positive_result" && this->Size[2]>0);
  return this->Size[2];
}

//-----------------------------------------------------------------------------
// Description:
// Return the length along the z-axis.
// Relevant only if GetDimension()>=3
// \pre positive_depth: depth>0
// \post depth_is_set: GetDepth()==depth
void vtkHyperOctreeSampleFunction::SetDepth(double depth)
{
   assert("pre: positive_depth" && depth>0);
  if(this->Size[2]!=depth)
    {
    this->Size[2]=depth;
    this->Modified();
    }
  assert("post: depth_is_set" && this->GetDepth()==depth);
}

//----------------------------------------------------------------------------
int vtkHyperOctreeSampleFunction::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // We cannot give the exact number of levels of the hyperoctree
  // because it is not generated yet and this process is random-based.
  // Just send an upper limit.
  // Used by the vtkHyperOctreeToUniformGrid to send some
  // whole extent in RequestInformation().
  outInfo->Set(vtkHyperOctree::LEVELS(),this->Levels);
  outInfo->Set(vtkHyperOctree::DIMENSION(),this->Dimension);
  outInfo->Set(vtkHyperOctree::SIZES(),this->Size,3);
  outInfo->Set(vtkDataObject::ORIGIN(),this->Origin,3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeSampleFunction::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkHyperOctree *output = vtkHyperOctree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if(this->ImplicitFunction==0)
    {
    vtkErrorMacro(<<"No implicit function specified");
    return 0;
    }

  output->SetDimension(this->Dimension);
  output->SetSize(this->Size);
  output->SetOrigin(this->Origin);

  vtkDataArray *scalars=vtkDataArray::CreateDataArray(this->OutputScalarType);
  scalars->SetNumberOfComponents(1);

  vtkIdType fact=(1<<(this->Levels-1));
  vtkIdType maxNumberOfCells=fact;
  if(this->GetDimension()>=2)
    {
    maxNumberOfCells*=fact;
    if(this->GetDimension()==3)
      {
      maxNumberOfCells*=fact;
      }
    }
  scalars->Allocate(maxNumberOfCells);
  scalars->SetNumberOfTuples(1); // the root
  scalars->SetName("ImplicitFunction");
  output->GetLeafData()->SetScalars(scalars);
  scalars->UnRegister(this);

  vtkHyperOctreeCursor *cursor=output->NewCellCursor();
  cursor->ToRoot();
  this->Subdivide(cursor,1,output);
  cursor->UnRegister(this);

  scalars->Squeeze();
  assert("post: valid_levels" && output->GetNumberOfLevels()<=this->GetLevels());
  assert("post: dataset_and_data_size_match" && output->CheckAttributes()==0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeSampleFunction::Subdivide(vtkHyperOctreeCursor *cursor,
                                             int level,
                                             vtkHyperOctree *output)
{
  int subdivide=level<=this->MinLevels;
  double p[3];
  double ratio=1.0/(1<<(level-1));
  int indices[3];
  int target[3];

  indices[0]=cursor->GetIndex(0);
  indices[1]=0;
  indices[2]=0;
  p[0]=(indices[0]+0.5)*ratio*this->Size[0]+this->Origin[0];
  if(this->Dimension>1)
    {
    indices[1]=cursor->GetIndex(1);
    p[1]=(indices[1]+0.5)*ratio*this->Size[1]+this->Origin[1];
    }
  else
    {
    p[1]=this->Origin[1];
    }
  if(this->Dimension==3)
    {
    indices[2]=cursor->GetIndex(2);
    p[2]=(indices[2]+0.5)*ratio*this->Size[2]+this->Origin[2];
    }
  else
    {
    p[2]=this->Origin[2];
    }

  double value=this->ImplicitFunction->FunctionValue(p);

  if(!subdivide)
    {
    subdivide=level<this->Levels;
    if(subdivide)
      {
      subdivide=0;
      ratio=1.0/(1<<level);
      indices[0]<<=1; // children level
      indices[1]<<=1; // children level
      indices[2]<<=1; // children level

      int kc;
      int jc;
      if(this->Dimension==3)
        {
        kc=2;
        }
      else
        {
        kc=1;
        }
      if(this->Dimension>=2)
        {
        jc=2;
        }
      else
        {
        jc=1;
        }
      int k=0;
      while(!subdivide && k<kc)
        {
        target[2]=indices[2]+k;
        if(this->Dimension==3)
          {
          p[2]=(target[2]+0.5)*ratio*this->Size[2]+this->Origin[2];
          }
        int j=0;
        while(!subdivide && j<jc)
          {
          if(this->Dimension>1)
            {
            target[1]=indices[1]+j;
            p[1]=(target[1]+0.5)*ratio*this->Size[1]+this->Origin[1];
            }
          int i=0;
          while(!subdivide && i<2)
            {
            target[0]=indices[0]+i;
            p[0]=(target[0]+0.5)*ratio*this->Size[0]+this->Origin[0];
            double childValue=this->ImplicitFunction->FunctionValue(p);
            subdivide=fabs(value-childValue)>=this->Threshold;
            ++i;
            }
          ++j;
          }
        ++k;
        }
      }
    }
  if(subdivide)
    {
    output->SubdivideLeaf(cursor);
    int c=cursor->GetNumberOfChildren();
    int i=0;
    while(i<c)
      {
      cursor->ToChild(i);
      this->Subdivide(cursor,level+1,output);
      cursor->ToParent();
      ++i;
      }
    }
  else
    {
    vtkIdType id=cursor->GetLeafId();
    output->GetLeafData()->GetScalars()->InsertTuple1(id,value);
    }
}

//-----------------------------------------------------------------------------
unsigned long vtkHyperOctreeSampleFunction::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeSampleFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os<<indent<<"Dimension: "<<Dimension<<endl;
  os<<indent<<"Width: "<<this->Size[0]<<endl;
  os<<indent<<"Height: "<<this->Size[1]<<endl;
  os<<indent<<"Depth: "<<this->Size[2]<<endl;

  os<<indent<<"origin: "<<this->Origin[0]<<","<<this->Origin[1]<<",";
  os<<this->Origin[2]<<endl;


  os<<indent<<"Levels: "<<this->Levels<<endl;
  os<<indent<<"MinLevels: "<<this->MinLevels<<endl;
  os<<indent<<"Threshold: "<<this->Threshold<<endl;
  os<<indent<<"OutputScalarType: "<<this->OutputScalarType<<endl;

  if(this->ImplicitFunction!=0)
    {
    os<<indent<<"Implicit Function: "<<this->ImplicitFunction<<endl;
    }
  else
    {
    os<<indent<<"No Implicit function defined"<<endl;
    }
}
