/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRDataSetCache.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRDataSetCache.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"

#include <cassert>


vtkStandardNewMacro( vtkAMRDataSetCache );

vtkAMRDataSetCache::vtkAMRDataSetCache()
{
  this->Cache = vtkMultiBlockDataSet::New();
}

//------------------------------------------------------------------------------
vtkAMRDataSetCache::~vtkAMRDataSetCache()
{
  this->Cache->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlock(
    const int compositeIdx, vtkUniformGrid *amrGrid)
{
  assert( "pre: AMR block is NULL" && (amrGrid != NULL) );

  if( !this->HasAMRBlock( compositeIdx ) )
    {
      this->Cache->SetBlock( compositeIdx, amrGrid );
      this->history.insert( compositeIdx );
    }
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlockPointData(
   const int compositeIdx, vtkDataArray *dataArray )
{
  assert( "pre: AMR array is NULL" && (dataArray != NULL) );
  assert( "pre: AMR block is cached" && ( this->HasAMRBlock( compositeIdx ) ) );

  vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
  assert( "pre: AMR block should not be NULL" && ( amrBlock != NULL ) );

  vtkPointData *PD = amrBlock ->GetPointData();
  assert( "pre: PointData should not be NULL" && ( PD != NULL ) );

  if( !PD->HasArray( dataArray->GetName() ) )
    PD->AddArray( dataArray );
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlockCellData(
   const int compositeIdx, vtkDataArray *dataArray )
{
  assert( "pre: AMR array is NULL" && (dataArray != NULL) );
  assert( "pre: AMR block is cached" && ( this->HasAMRBlock( compositeIdx ) ) );

  vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
  assert( "pre: AMR block should not be NULL" &&
          (this->HasAMRBlock( compositeIdx ) ) );

  vtkCellData *CD = amrBlock ->GetCellData();
  assert( "pre: CellData should not be NULL" && (CD != NULL) );

  if( !CD->HasArray( dataArray->GetName() ) )
    CD->AddArray( dataArray );
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMRDataSetCache::GetAMRBlockCellData(
    const int compositeIdx, const char *dataName )
{
  if( this->HasAMRBlockCellData( compositeIdx, dataName ) )
    {
      vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
      assert( "pre: AMR block should not be NULL" &&
              (this->HasAMRBlock( compositeIdx ) ) );

      vtkCellData *CD = amrBlock ->GetCellData();
      assert( "pre: CellData should not be NULL" && (CD != NULL) );

      if( CD->HasArray( dataName ) )
        return CD->GetArray( dataName );
      else
        return NULL;
    }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMRDataSetCache::GetAMRBlockPointData(
    const int compositeIdx, const char *dataName )
{

  if( this->HasAMRBlockPointData( compositeIdx, dataName ) )
    {
      vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
      assert( "pre: AMR block should not be NULL" && ( amrBlock != NULL ) );

      vtkPointData *PD = amrBlock ->GetPointData();
      assert( "pre: PointData should not be NULL" && ( PD != NULL ) );

      if( PD->HasArray( dataName ) )
        return PD->GetArray( dataName );
      else
        return NULL;
    }
  return NULL;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRDataSetCache::GetAMRBlock( const int compositeIdx )
{
  if( this->HasAMRBlock( compositeIdx ) )
    {
      return( vtkUniformGrid::SafeDownCast(
                  this->Cache->GetBlock( compositeIdx ) ) );
    }
  return NULL;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockCellData(
    const int compositeIdx, const char *name)
{
  assert( "pre: array name is NULL" && (name != NULL) );

  if( this->HasAMRBlock( compositeIdx ) )
    {
      vtkUniformGrid *gridPtr = this->GetAMRBlock( compositeIdx );
      assert( "pre: cachedk block is NULL!" && (gridPtr != NULL) );

      vtkCellData *CD = gridPtr->GetCellData();
      assert( "pre: cell data is NULL" && (CD != NULL) );

      if( CD->HasArray( name ) )
        return true;
      else
        return false;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockPointData(
    const int compositeIdx, const char *name)
{
  assert( "pre: array name is NULL" && (name != NULL) );

  if( this->HasAMRBlock( compositeIdx ) )
    {
      vtkUniformGrid *gridPtr = this->GetAMRBlock( compositeIdx );
      assert( "pre: cachedk block is NULL!" && (gridPtr != NULL) );

      vtkPointData *PD = gridPtr->GetPointData();
      assert( "pre: point data is NULL" && (PD != NULL) );

      if( PD->HasArray( name ) )
        return true;
      else
        return false;
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlock(const int compositeIdx )
{
  if( this->history.empty( ) )
    return false;

  if( this->history.find( compositeIdx) != this->history.end() )
   return true;
  return false;
}
