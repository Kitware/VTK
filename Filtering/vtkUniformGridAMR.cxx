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
#include "vtkUniformGrid.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkTimeStamp.h"
#include "vtkDataObject.h"
#include "vtkCompositeDataIterator.h"
#include "vtkUniformGridAMRDataIterator.h"

#include <cassert>

vtkUniformGridAMR::vtkUniformGridAMR()
{
  this->ScalarRange[0] = VTK_DOUBLE_MAX;
  this->ScalarRange[1] = VTK_DOUBLE_MIN;
}

//------------------------------------------------------------------------------
vtkUniformGridAMR::~vtkUniformGridAMR()
{

}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::SetNumberOfLevels(const unsigned int numLevels)
{
  this->Superclass::SetNumberOfChildren(numLevels);

  // Initialize each level with a vtkMultiPieceDataSet.
  // vtkMultiPieceDataSet is an overkill here, since the datasets with in a
  // level cannot be composite datasets themselves.
  // This will make is possible for the user to set information with each level
  // (in future).
  for (unsigned int cc=0; cc < numLevels; cc++)
    {
    if (!this->Superclass::GetChild(cc))
      {
      vtkMultiPieceDataSet* mds = vtkMultiPieceDataSet::New();
      this->Superclass::SetChild(cc, mds);
      mds->Delete();
      }
    }
}

//------------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfLevels()
{
  return( this->Superclass::GetNumberOfChildren() );
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::SetNumberOfDataSets(
    const unsigned level, const unsigned int N)
{
  if( level >= this->GetNumberOfLevels( ))
    {
    this->SetNumberOfLevels( level+1 );
    }

  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast( this->Superclass::GetChild(level) );
  if( levelDS != NULL )
    {
    levelDS->SetNumberOfPieces( N );
    }
}

//------------------------------------------------------------------------------
vtkCompositeDataIterator* vtkUniformGridAMR::NewIterator()
{
  vtkUniformGridAMRDataIterator* iter = vtkUniformGridAMRDataIterator::New();
  iter->SetDataSet( this );
  return(iter);
}

//------------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetNumberOfDataSets(const unsigned int level)
{
  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast(this->Superclass::GetChild(level));
   if( levelDS != NULL )
     {
     return levelDS->GetNumberOfPieces();
     }
   return 0;
}

//------------------------------------------------------------------------------
unsigned int vtkUniformGridAMR::GetTotalNumberOfBlocks()
{
  unsigned int totalNumberOfBlocks = 0;
  unsigned int numLevels = this->GetNumberOfLevels();
  for( unsigned int levelIdx=0; levelIdx < numLevels; ++levelIdx )
    {
    totalNumberOfBlocks += this->GetNumberOfDataSets( levelIdx );
    } // END for all levels
  return( totalNumberOfBlocks );
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::SetDataSet(
    unsigned int level, unsigned int idx, vtkUniformGrid *grid)
{
  if( level >= this->GetNumberOfLevels() )
    {
    this->SetNumberOfLevels( level+1 );
    }

  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast(this->Superclass::GetChild(level) );
  if( levelDS != NULL )
    {
    levelDS->SetPiece( idx, grid );
    }
  else
    {
    vtkErrorMacro("Multi-piece data-structure is NULL!");
    }
}

//------------------------------------------------------------------------------
vtkInformation* vtkUniformGridAMR::GetMetaData(unsigned level, unsigned int idx)
{
  vtkMultiPieceDataSet *levelMDS =
      vtkMultiPieceDataSet::SafeDownCast(this->GetChild(level));
  if( levelMDS != NULL )
    {
    return( levelMDS->GetMetaData(idx) );
    }
  return NULL;
}

//------------------------------------------------------------------------------
int vtkUniformGridAMR::HasMetaData(unsigned int level, unsigned int idx )
{
  vtkMultiPieceDataSet* levelMDS =
      vtkMultiPieceDataSet::SafeDownCast(this->GetChild(level));

  if(levelMDS != NULL)
    {
    return( levelMDS->HasMetaData(idx) );
    }
  return 0;
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::AppendDataSet(
    unsigned int level, vtkUniformGrid* grid)
{
  unsigned int idx = this->GetNumberOfDataSets( level );
  this->SetDataSet( level,idx,grid );
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGridAMR::GetDataSet(
    unsigned int level, unsigned int idx )
{
  assert("pre: level is out of bounds!" &&
         (level < this->GetNumberOfLevels()));
  assert("pre: idx is out of bounds!" &&
         (idx < this->GetNumberOfDataSets(level)));

  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast(this->Superclass::GetChild(level));
  if( levelDS != NULL )
    {
    return(vtkUniformGrid::SafeDownCast(levelDS->GetPiece(idx)));
    }
  vtkErrorMacro("Multi-piece data-structure is NULL!");
  return NULL;
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::GetScalarRange(double range[2])
{
  this->ComputeScalarRange();
  range[0] = this->ScalarRange[0];
  range[1] = this->ScalarRange[1];
}

//------------------------------------------------------------------------------
double* vtkUniformGridAMR::GetScalarRange()
{
  this->ComputeScalarRange();
  return( this->ScalarRange );
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::ComputeScalarRange()
{
  if( this->GetMTime() > this->ScalarRangeComputeTime )
    {
    double dataSetRange[2];
    this->ScalarRange[0]=VTK_DOUBLE_MAX;
    this->ScalarRange[1]=VTK_DOUBLE_MIN;
    unsigned int level=0;
    unsigned int levels=this->GetNumberOfLevels();
    while(level < levels)
      {
      unsigned int dataset=0;
      unsigned int datasets=this->GetNumberOfDataSets(level);
      while(dataset < datasets)
        {
        vtkUniformGrid *ug =
            vtkUniformGrid::SafeDownCast(this->GetDataSet(level,dataset));
        ug->GetScalarRange(dataSetRange);
        if(dataSetRange[0] < this->ScalarRange[0])
          {
          this->ScalarRange[0]=dataSetRange[0];
          }
        if(dataSetRange[1] > this->ScalarRange[1])
          {
          this->ScalarRange[1]=dataSetRange[1];
          }
        ++dataset;
        } // END loop through all data sets within a level
      ++level;
      } // END loop through all levels
    this->ScalarRangeComputeTime.Modified();
    } // END if cache is invalidated
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::GetBounds( double bounds[6] )
{
  this->ComputeBounds();
  for( int i=0; i < 6; ++i )
    {
    bounds[ i ] = this->Bounds[ i ];
    }
}

//------------------------------------------------------------------------------
double* vtkUniformGridAMR::GetBounds()
{
  this->ComputeBounds();
  return( this->Bounds );
}

//------------------------------------------------------------------------------
void vtkUniformGridAMR::ComputeBounds()
{
  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;

  double tmpbounds[6];
  unsigned int levelIdx = 0;
  for( ; levelIdx < this->GetNumberOfLevels(); ++levelIdx )
    {
    unsigned int dataIdx = 0;
    for( ; dataIdx < this->GetNumberOfDataSets( levelIdx ); ++dataIdx )
      {
      vtkUniformGrid *grd = this->GetDataSet( levelIdx, dataIdx );
      if( grd != NULL )
        {
        grd->GetBounds( tmpbounds );
        for( int i=0; i < 3; ++i )
          {
          if( tmpbounds[i*2] < this->Bounds[i*2] )
            {
            this->Bounds[i*2] = tmpbounds[i*2];
            }
          if( tmpbounds[i*2+1] > this->Bounds[i*2+1] )
            {
            this->Bounds[i*2+1] = tmpbounds[i*2+1];
            }
          } // END for each dimension
        } // END if grid is not NULL
      } // END for all data sets
    } // END for all levels
}
