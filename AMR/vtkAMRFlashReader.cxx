/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRFlashReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRFlashReader.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAMRUtilities.h"
#include "vtkByteSwap.h"
#include "vtkUniformGrid.h"
#include "vtkDataArraySelection.h"

#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkLongLongArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"

#include <cassert>
#include <vtkstd/vector>
#include <vtkstd/map>

#define H5_USE_16_API
#include <hdf5.h>

#include "vtkAMRFlashReaderInternal.h"

vtkStandardNewMacro(vtkAMRFlashReader);

//------------------------------------------------------------------------------
vtkAMRFlashReader::vtkAMRFlashReader()
{
  this->Internal = new vtkFlashReaderInternal;
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMRFlashReader::~vtkAMRFlashReader()
{
  delete this->Internal;
  this->Internal = NULL;
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::SetFileName( const char* fileName )
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );

  if( fileName && strcmp(fileName,"") &&
     ( ( this->FileName == NULL ) || strcmp( fileName, this->FileName ) ) )
    {

      if( this->FileName )
        {
          delete [] this->FileName;
          this->FileName = NULL;
          this->Internal->SetFileName( NULL );
        }

      this->FileName = new char[ strlen(fileName)+1 ];
      strcpy( this->FileName, fileName );
      this->FileName[ strlen( fileName ) ] = '\0';

      this->Internal->SetFileName( this->FileName );
    }

  this->SetUpDataArraySelections();
  this->InitializeArraySelections();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::ReadMetaData()
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::GenerateBlockMap()
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal!=NULL) );

  this->BlockMap.clear();
  this->Internal->ReadMetaData();

  for( int i=0; i < this->Internal->NumberOfBlocks; ++i )
    {
      if( this->GetBlockLevel( i ) <= this->MaxLevel )
        {
          this->BlockMap.push_back( i );
        }
    }
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::GetBlockLevel( const int blockIdx )
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );

  this->Internal->ReadMetaData();
  if( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
      vtkErrorMacro( "Block Index (" << blockIdx << ") is out-of-bounds!" );
      return( -1 );
    }
  return( this->Internal->Blocks[ blockIdx ].Level );
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::GetNumberOfBlocks()
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );

  this->Internal->ReadMetaData();
  return( this->Internal->NumberOfBlocks );
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::GetNumberOfLevels()
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  return( this->Internal->NumberOfLevels );
}

//-----------------------------------------------------------------------------
int vtkAMRFlashReader::FillMetaData( vtkHierarchicalBoxDataSet *metadata )
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );
  assert( "pre: metadata object is NULL" && (metadata != NULL) );

  this->Internal->ReadMetaData();

  std::vector< int > b2level;
  b2level.resize( this->Internal->NumberOfLevels, 0 );

  for( int i=0; i < this->Internal->NumberOfBlocks; ++i )
    {
      // Start numbering levels from 0!
      int level = this->Internal->Blocks[ i ].Level-1;

      double blockMin[3];
      double blockMax[3];
      double spacings[3];
      for( int j=0; j < 3; ++j )
        {
          blockMin[j] = this->Internal->Blocks[i].MinBounds[j];
          blockMax[j] = this->Internal->Blocks[i].MaxBounds[j];
          spacings[j] = (this->Internal->BlockGridDimensions[j] > 1)?
          (blockMax[j]-blockMin[j])/(this->Internal->BlockGridDimensions[j]-1.0):1.0;
        }

      vtkUniformGrid *ug = vtkUniformGrid::New();
      ug->SetDimensions( this->Internal->BlockGridDimensions );
      ug->SetOrigin( blockMin[0], blockMin[1], blockMin[2] );
      ug->SetSpacing( spacings );

      metadata->SetDataSet( level, b2level[level], ug );
      ug->Delete();
      b2level[ level ]++;
    } // END for all blocks

  // NOTE: The communicator here is NULL since each process loads all the
  // metadata.
  vtkAMRUtilities::GenerateMetaData( metadata, NULL );
  return( 1 );
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::GetBlock(
    int index, vtkHierarchicalBoxDataSet *hbds,
    vtkstd::vector< int > &idxcounter)
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );
  assert( "pre: Output AMR dataset is NULL" && (hbds != NULL)  );

  this->Internal->ReadMetaData();

  int blockIdx = this->BlockMap[ index ];

  // Start numbering levels from 0!
  int level    = this->Internal->Blocks[ blockIdx ].Level-1;
  double blockMin[3];
  double blockMax[3];
  double spacings[3];
  for( int i=0; i < 3; ++i )
    {
     blockMin[i] = this->Internal->Blocks[ blockIdx ].MinBounds[i];
     blockMax[i] = this->Internal->Blocks[ blockIdx ].MaxBounds[i];
     spacings[i] = (this->Internal->BlockGridDimensions[i] > 1)?
     (blockMax[i]-blockMin[i])/(this->Internal->BlockGridDimensions[i]-1.0):1.0;
    }

  vtkUniformGrid *ug = vtkUniformGrid::New();
  ug->SetDimensions( this->Internal->BlockGridDimensions );
  ug->SetOrigin( blockMin[0], blockMin[1], blockMin[2] );
  ug->SetSpacing( spacings );

  int numAttrs = static_cast< int >( this->Internal->AttributeNames.size() );
  for( int i=0; i < numAttrs; ++i )
    {
      if( this->GetCellArrayStatus(
            this->Internal->AttributeNames[i].c_str() ) )
        {
          this->Internal->GetBlockAttribute(
              this->Internal->AttributeNames[i].c_str(),blockIdx, ug );
        }
    }

  hbds->SetDataSet(level,idxcounter[level],ug);
  ug->Delete();
  idxcounter[ level ]++;
}

//-----------------------------------------------------------------------------
void vtkAMRFlashReader::SetUpDataArraySelections()
{
  assert( "pre: Internal Flash Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();

  int numAttrs = static_cast< int >( this->Internal->AttributeNames.size() );
  for( int i=0; i < numAttrs; ++i )
    {
      this->CellDataArraySelection->AddArray(
          this->Internal->AttributeNames[ i ].c_str()  );
    }

}
