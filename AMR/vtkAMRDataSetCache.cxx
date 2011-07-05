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

#include <cassert>


vtkStandardNewMacro( vtkAMRDataSetCache );

vtkAMRDataSetCache::vtkAMRDataSetCache()
{
  this->Size = 100; // Cache 100 blocks by default
}

//------------------------------------------------------------------------------
vtkAMRDataSetCache::~vtkAMRDataSetCache()
{
  vtkstd::map< int, vtkUniformGrid* >::iterator iter;
  for( iter = this->Cache.begin(); iter != this->Cache.end(); ++iter )
    {
      if( iter->second != NULL )
        iter->second->Delete();
    }
  this->Cache.clear();
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
      this->Cache[ compositeIdx ] = amrGrid;
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
    return this->Cache[ compositeIdx ];
  return NULL;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockCellData(
    const int compositeIdx, const char *name)
{
  assert( "pre: array name is NULL" && (name != NULL) );

  if( this->HasAMRBlock( compositeIdx ) )
    {
      vtkCellData *CD = this->Cache[ compositeIdx ]->GetCellData();
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
      vtkPointData *PD = this->Cache[ compositeIdx ]->GetPointData();
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
  if( this->Cache.find( compositeIdx) != this->Cache.end() )
   return true;
  return false;
}
