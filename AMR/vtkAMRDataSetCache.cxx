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
#include "vtkDataArray.h"
#include "vtkUniformGrid.h"
#include "vtkTimerLog.h"
#include <cassert>

vtkStandardNewMacro( vtkAMRDataSetCache );

vtkAMRDataSetCache::vtkAMRDataSetCache()
{
}

//------------------------------------------------------------------------------
vtkAMRDataSetCache::~vtkAMRDataSetCache()
{
  AMRCacheType::iterator iter = this->Cache.begin();
  for( ; iter != this->Cache.end(); ++iter )
    {
    if( iter->second != NULL )
      {
      iter->second->Delete();
      }
    this->Cache.erase( iter );
    }
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlock(
    int compositeIdx, vtkUniformGrid *amrGrid)
{
  assert( "pre: AMR block is NULL" && (amrGrid != NULL) );

  vtkTimerLog::MarkStartEvent( "AMRCache::InsertBlock" );
  if( !this->HasAMRBlock( compositeIdx ) )
    {
    this->Cache[ compositeIdx ] = amrGrid;
    }
  vtkTimerLog::MarkEndEvent( "AMRCache::InsertBlock" );
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlockPointData(
   int compositeIdx, vtkDataArray *dataArray )
{
  assert( "pre: AMR array is NULL" && (dataArray != NULL) );
  assert( "pre: AMR block is cached" && ( this->HasAMRBlock( compositeIdx ) ) );

  vtkTimerLog::MarkStartEvent( "AMRCache::InsertAMRBlockPointData" );

  vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
  assert( "pre: AMR block should not be NULL" && ( amrBlock != NULL ) );

  vtkPointData *PD = amrBlock ->GetPointData();
  assert( "pre: PointData should not be NULL" && ( PD != NULL ) );

  if( !PD->HasArray( dataArray->GetName() ) )
    {
    PD->AddArray( dataArray );
    }

  vtkTimerLog::MarkEndEvent( "AMRCache::InsertAMRBlockPointData" );
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlockCellData(
   int compositeIdx, vtkDataArray *dataArray )
{
  assert( "pre: AMR array is NULL" && (dataArray != NULL) );
  assert( "pre: AMR block is cached" && ( this->HasAMRBlock( compositeIdx ) ) );

  vtkTimerLog::MarkStartEvent( "AMRCache::InsertAMRBlockCellData" );

  vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
  assert( "pre: AMR block should not be NULL" &&
          (this->HasAMRBlock( compositeIdx ) ) );

  vtkCellData *CD = amrBlock ->GetCellData();
  assert( "pre: CellData should not be NULL" && (CD != NULL) );

  if( !CD->HasArray( dataArray->GetName() ) )
    {
    CD->AddArray( dataArray );
    }

  vtkTimerLog::MarkEndEvent( "AMRCache::InsertAMRBlockCellData" );
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMRDataSetCache::GetAMRBlockCellData(
    int compositeIdx, const char *dataName )
{
  if( this->HasAMRBlockCellData( compositeIdx, dataName ) )
    {
    vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
    assert( "pre: AMR block should not be NULL" &&
            (this->HasAMRBlock( compositeIdx ) ) );

    vtkCellData *CD = amrBlock ->GetCellData();
    assert( "pre: CellData should not be NULL" && (CD != NULL) );

    if( CD->HasArray( dataName ) )
      {
      return CD->GetArray( dataName );
      }
    else
      {
      return NULL;
      }
    }
  return NULL;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMRDataSetCache::GetAMRBlockPointData(
    int compositeIdx, const char *dataName )
{

  if( this->HasAMRBlockPointData( compositeIdx, dataName ) )
    {
    vtkUniformGrid *amrBlock = this->GetAMRBlock( compositeIdx );
    assert( "pre: AMR block should not be NULL" && ( amrBlock != NULL ) );

    vtkPointData *PD = amrBlock ->GetPointData();
    assert( "pre: PointData should not be NULL" && ( PD != NULL ) );

    if( PD->HasArray( dataName ) )
      {
      return PD->GetArray( dataName );
      }
    else
      {
      return NULL;
      }
    }
  return NULL;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRDataSetCache::GetAMRBlock( const int compositeIdx )
{
  if( this->HasAMRBlock( compositeIdx ) )
    {
    return this->Cache[ compositeIdx ];
    }
  return NULL;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockCellData(
    int compositeIdx, const char *name)
{
  assert( "pre: array name is NULL" && (name != NULL) );

  if( this->HasAMRBlock( compositeIdx ) )
    {
    vtkUniformGrid *gridPtr = this->GetAMRBlock( compositeIdx );
    assert( "pre: cachedk block is NULL!" && (gridPtr != NULL) );

    vtkCellData *CD = gridPtr->GetCellData();
    assert( "pre: cell data is NULL" && (CD != NULL) );

    if( CD->HasArray( name ) )
      {
      return true;
      }
    else
      {
      return false;
      }
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockPointData(
    int compositeIdx, const char *name)
{
  assert( "pre: array name is NULL" && (name != NULL) );

  if( this->HasAMRBlock( compositeIdx ) )
    {
    vtkUniformGrid *gridPtr = this->GetAMRBlock( compositeIdx );
    assert( "pre: cachedk block is NULL!" && (gridPtr != NULL) );

    vtkPointData *PD = gridPtr->GetPointData();
    assert( "pre: point data is NULL" && (PD != NULL) );

    if( PD->HasArray( name ) )
      {
      return true;
      }
    else
      {
      return false;
      }
    }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlock(int compositeIdx )
{
  vtkTimerLog::MarkStartEvent( "AMRCache::CheckIfBlockExists" );

  if( this->Cache.empty( ) )
    {
    vtkTimerLog::MarkEndEvent( "AMRCache::CheckIfBlockExists" );
    return false;
    }

  if( this->Cache.find( compositeIdx ) != this->Cache.end() )
    {
    vtkTimerLog::MarkEndEvent( "AMRCache::CheckIfBlockExists" );
    return true;
    }

  vtkTimerLog::MarkEndEvent( "AMRCache::CheckIfBlockExists" );
  return false;
}
