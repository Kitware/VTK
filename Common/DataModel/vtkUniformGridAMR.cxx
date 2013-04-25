/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkMath.h"
#include "vtkType.h"
#include "vtkAMRInformation.h"
#include "vtkAMRDataInternals.h"

vtkStandardNewMacro(vtkUniformGridAMR);

//----------------------------------------------------------------------------
vtkUniformGridAMR::vtkUniformGridAMR()
{
  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;
  this->AMRInfo = NULL;
  this->AMRData = vtkAMRDataInternals::New();
}

//----------------------------------------------------------------------------
vtkUniformGridAMR::~vtkUniformGridAMR()
{
  if(this->AMRInfo)
    {
    this->AMRInfo->Delete();
    }
  this->AMRData->Delete();
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::SetAMRInfo(vtkAMRInformation* amrInfo)
{
  if(amrInfo==this->AMRInfo)
    {
    return;
    }
  if(this->AMRInfo)
    {
    this->AMRInfo->Delete();
    }
  this->AMRInfo = amrInfo;
  if(this->AMRInfo)
    {
    this->AMRInfo->Register(this);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGridAMR::GetDataSet(unsigned int level, unsigned int idx)
{
  return this->AMRData->GetDataSet( this->GetCompositeIndex(level,idx));
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkUniformGridAMR::NewIterator()
{
  vtkUniformGridAMRDataIterator* iter = vtkUniformGridAMRDataIterator::New();
  iter->SetDataSet( this );
  return iter;
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::Initialize()
{
  this->Initialize(0,NULL);
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::Initialize(int numLevels, const int * blocksPerLevel)
{
  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;

  vtkSmartPointer<vtkAMRInformation> amrInfo =vtkSmartPointer<vtkAMRInformation>::New();
  this->SetAMRInfo(amrInfo);
  this->AMRInfo->Initialize(numLevels, blocksPerLevel);
  this->AMRData->Initialize();
}

//----------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfLevels()
{
  unsigned int nlev = 0;
  if (this->AMRInfo)
    {
    nlev = this->AMRInfo->GetNumberOfLevels();
    }
  return nlev;
}

//----------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetTotalNumberOfBlocks()
{
  unsigned int nblocks = 0;
  if (this->AMRInfo)
    {
    nblocks = this->AMRInfo->GetTotalNumberOfBlocks();
    }
  return nblocks;
}

//----------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfDataSets(const unsigned int level)
{
  unsigned int ndata = 0;
  if (this->AMRInfo)
    {
    ndata = this->AMRInfo->GetNumberOfDataSets(level);
    }
  return ndata;
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::SetDataSet(
  unsigned int level,
  unsigned int idx,
  vtkUniformGrid *grid)
{
  if(!grid)
    {
    return; //NULL grid, nothing to do
    }
  if(level>=this->GetNumberOfLevels() || idx >=this->GetNumberOfDataSets(level))
    {
    vtkErrorMacro("Invalid data set index: "<<level<<" "<<idx);
    return;
    }

  this->AMRInfo->SetGridDescription(grid->GetGridDescription());
  int index = this->AMRInfo->GetIndex(level,idx);
  this->AMRData->Insert(index, grid);

  //update bounds
  double bb[6];
  grid->GetBounds(bb);
  //update bounds
  for( int i=0; i < 3; ++i )
    {
    if( bb[i*2] < this->Bounds[i*2] )
      {
      this->Bounds[i*2] = bb[i*2];
      }
    if( bb[i*2+1] > this->Bounds[i*2+1])
      {
      this->Bounds[i*2+1] = bb[i*2+1];
      }
    } // END for each dimension
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::SetDataSet(vtkCompositeDataIterator* compositeIter, vtkDataObject* dataObj)
{
  vtkUniformGridAMRDataIterator* itr = vtkUniformGridAMRDataIterator::SafeDownCast(compositeIter);
  vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(dataObj);
  int level = itr->GetCurrentLevel();
  int id = itr->GetCurrentIndex();
  this->SetDataSet(level,id,grid);
};

//----------------------------------------------------------------------------
void vtkUniformGridAMR::SetGridDescription(int gridDescription)
{
  if (this->AMRInfo)
    {
    this->AMRInfo->SetGridDescription(gridDescription);
    }
}

//----------------------------------------------------------------------------
int vtkUniformGridAMR::GetGridDescription()
{
  int desc = 0;
  if (this->AMRInfo)
    {
    desc = this->AMRInfo->GetGridDescription();
    }
  return desc;
}

//------------------------------------------------------------------------------
vtkDataObject* vtkUniformGridAMR::GetDataSet(vtkCompositeDataIterator* compositeIter)
{
  vtkUniformGridAMRDataIterator* itr = vtkUniformGridAMRDataIterator::SafeDownCast(compositeIter);
  if (!itr)
    {
    return NULL;
    }
  int level = itr->GetCurrentLevel();
  int id = itr->GetCurrentIndex();
  return this->GetDataSet(level,id);
}

//----------------------------------------------------------------------------
int vtkUniformGridAMR::GetCompositeIndex(
    const unsigned int level, const unsigned int index )
{

  if(level >= this->GetNumberOfLevels()|| index >= this->GetNumberOfDataSets( level ) )
    {
    vtkErrorMacro("Invalid level-index pair: "<<level<<", "<<index);
    return 0;
    }
  return this->AMRInfo->GetIndex(level,index);
}
//----------------------------------------------------------------------------
void vtkUniformGridAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::GetLevelAndIndex(
    unsigned int flatIdx, unsigned int &level, unsigned int &idx )
{
  this->AMRInfo->ComputeIndexPair(flatIdx,level,idx);
}

//----------------------------------------------------------------------------
vtkUniformGridAMR* vtkUniformGridAMR::GetData(
  vtkInformation* info)
{
  return
    info?vtkUniformGridAMR::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkUniformGridAMR* vtkUniformGridAMR::GetData(
  vtkInformationVector* v, int i)
{
  return vtkUniformGridAMR::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::ShallowCopy( vtkDataObject *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::ShallowCopy( src );

  if(vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
    {
    this->SetAMRInfo(hbds->GetAMRInfo());
    this->AMRData->ShallowCopy(hbds->GetAMRData());
    memcpy(this->Bounds, hbds->Bounds, sizeof(double)*6);
    }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::DeepCopy( vtkDataObject *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::DeepCopy( src );

  if(vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
    {
    this->SetAMRInfo(NULL);
    this->AMRInfo = vtkAMRInformation::New();
    this->AMRInfo->DeepCopy(hbds->GetAMRInfo());
    memcpy(this->Bounds, hbds->Bounds, sizeof(double)*6);
    }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::CopyStructure( vtkCompositeDataSet *src )
{
  if( src == this )
    {
    return;
    }

  if(vtkUniformGridAMR* hbds = vtkUniformGridAMR::SafeDownCast(src))
    {
    this->SetAMRInfo(hbds->GetAMRInfo());
    }

  this->Modified();
}

//----------------------------------------------------------------------------
const double* vtkUniformGridAMR::GetBounds()
{
  return !this->AMRData->Empty()? this->Bounds : this->AMRInfo->GetBounds();
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::GetBounds( double bounds[6] )
{
  const double* bb = this->GetBounds();
  for( int i=0; i < 6; ++i )
    {
    bounds[ i ] = bb[ i ];
    }
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::GetMin(double min[3])
{
  const double* bb =  this->GetBounds();
  min[0] = bb[0];
  min[1] = bb[2];
  min[2] = bb[4];
}

//----------------------------------------------------------------------------
void vtkUniformGridAMR::GetMax(double max[3])
{
  const double* bb =  this->GetBounds();
  max[0] = bb[1];
  max[1] = bb[3];
  max[2] = bb[5];
}
