/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMREnzoReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMREnzoReader.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkDataArraySelection.h"
#include "vtkDataArray.h"
#include "vtkPolyData.h"
#include "vtkAMRUtilities.h"
#include "vtkIndent.h"
#include "vtksys/SystemTools.hxx"

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

#define H5_USE_16_API
#include <hdf5.h>

#include <vtkstd/vector>
#include <vtkstd/string>
#include <cassert>

#include "vtkAMREnzoReaderInternal.hpp"


vtkStandardNewMacro(vtkAMREnzoReader);

//-----------------------------------------------------------------------------
vtkAMREnzoReader::vtkAMREnzoReader()
{
  this->Internal = new vtkEnzoReaderInternal( this );
  this->Initialize();
}

//-----------------------------------------------------------------------------
vtkAMREnzoReader::~vtkAMREnzoReader()
{
  delete this->Internal;
  this->Internal=NULL;

  this->BlockMap.clear();

  if( this->FileName )
    {
      delete [] this->FileName;
      this->FileName = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::SetFileName( const char* fileName )
{
  assert("pre: Internal Enzo AMR Reader is NULL" && (this->Internal != NULL ));

  int isValid=0;

  if( fileName && strcmp( fileName, "" ) &&
    ( (this->FileName==NULL) || (strcmp(fileName,this->FileName ) ) ) )
    {
        vtkstd::string  tempName( fileName );
        vtkstd::string  bExtName( ".boundary" );
        vtkstd::string  hExtName( ".hierarchy" );

        if( tempName.length() > hExtName.length() &&
            tempName.substr(tempName.length()-hExtName.length() )== hExtName )
          {
            this->Internal->MajorFileName =
                tempName.substr( 0, tempName.length() - hExtName.length() );
            this->Internal->HierarchyFileName = tempName;
            this->Internal->BoundaryFileName  =
                this->Internal->MajorFileName + bExtName;
          }
       else if( tempName.length() > bExtName.length() &&
           tempName.substr( tempName.length() - bExtName.length() )==bExtName )
         {
           this->Internal->MajorFileName =
              tempName.substr( 0, tempName.length() - bExtName.length() );
           this->Internal->BoundaryFileName  = tempName;
           this->Internal->HierarchyFileName =
              this->Internal->MajorFileName + hExtName;
         }
      else
        {
          vtkErrorMacro( "Enzo file has invalid extension!");
          return;
        }

        isValid = 1;
        this->Internal->DirectoryName =
            GetEnzoDirectory(this->Internal->MajorFileName.c_str());
    }

  if( isValid )
    {
      this->BlockMap.clear();
      this->Internal->Blocks.clear();
      this->Internal->NumberOfBlocks = 0;

      if ( this->FileName )
      {
        delete [] this->FileName;
        this->FileName = NULL;
        this->Internal->SetFileName( NULL );
      }
      this->FileName = new char[  strlen( fileName ) + 1  ];
      strcpy( this->FileName, fileName );
      this->FileName[ strlen( fileName ) ] = '\0';
      this->Internal->SetFileName( this->FileName );
    }

  this->Internal->ReadMetaData();
  this->SetUpDataArraySelections();
  this->InitializeArraySelections();
  this->Modified();
  return;
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::ReadMetaData()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::GenerateBlockMap()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );

  this->BlockMap.clear();
  this->Internal->ReadMetaData();

  for( int i=0; i < this->Internal->NumberOfBlocks; ++i )
    {

      if( this->GetBlockLevel( i ) <= this->MaxLevel )
        {
          this->BlockMap.push_back( i );
        }

    } // END for all blocks
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetBlockLevel( const int blockIdx )
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );

  this->Internal->ReadMetaData();

  if( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
      vtkErrorMacro( "Block Index (" << blockIdx << ") is out-of-bounds!" );
      return( -1 );
    }
  return( this->Internal->Blocks[ blockIdx+1 ].Level );
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetNumberOfBlocks()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  return( this->Internal->NumberOfBlocks );
}

//-----------------------------------------------------------------------------
int vtkAMREnzoReader::GetNumberOfLevels()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  return( this->Internal->NumberOfLevels );
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::GetBlock(
    int index, vtkHierarchicalBoxDataSet *hbds,
    vtkstd::vector< int > &idxcounter)
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  assert( "pre: Output AMR dataset is NULL" && (hbds != NULL)  );

  this->Internal->ReadMetaData();
  int blockIdx                 = this->BlockMap[ index ];
  int N                        = this->Internal->Blocks.size();
  assert( "block index out-of-bounds!" &&
    (blockIdx+1 >= 0) && (blockIdx+1 < N ) );

  // this->Internal->Blocks includes a pseudo block --- the root as block #0
  vtkEnzoReaderBlock &theBlock = this->Internal->Blocks[ blockIdx+1 ];
  int level                    = theBlock.Level;

  double blockMin[3];
  double blockMax[3];
  double spacings[3];

  for( int i=0; i < 3; ++i )
    {
      blockMin[i] = theBlock.MinBounds[i];
      blockMax[i] = theBlock.MaxBounds[i];
      spacings[i] = ( theBlock.BlockNodeDimensions[i] > 1 )?
       (blockMax[i]-blockMin[i])/(theBlock.BlockNodeDimensions[i]-1.0) : 1.0;
    }

  vtkUniformGrid *ug = vtkUniformGrid::New();
  ug->SetDimensions( theBlock.BlockNodeDimensions );
  ug->SetOrigin( blockMin[0],blockMin[1],blockMin[2] );
  ug->SetSpacing( spacings[0],spacings[1],spacings[2] );

  int numAttrs = static_cast< int >(this->Internal->BlockAttributeNames.size());
  for( int i=0; i < numAttrs; ++i )
    {
      if(this->GetCellArrayStatus(
          this->Internal->BlockAttributeNames[ i ].c_str()))
      {
        this->Internal->GetBlockAttribute(
          this->Internal->BlockAttributeNames[ i ].c_str(),
          blockIdx, ug );
      }

    }

  hbds->SetDataSet(level,idxcounter[level],ug);
  ug->Delete();
  idxcounter[ level ]++;
}

//-----------------------------------------------------------------------------
void vtkAMREnzoReader::SetUpDataArraySelections()
{
  assert( "pre: Internal Enzo Reader is NULL" && (this->Internal != NULL) );
  this->Internal->ReadMetaData();
  this->Internal->GetAttributeNames();

  int numAttrs = static_cast< int >(
      this->Internal->BlockAttributeNames.size() );
  for( int i=0; i < numAttrs; i++ )
    {
      this->CellDataArraySelection->AddArray(
          this->Internal->BlockAttributeNames[i].c_str() );
    } // END for all attributes

}
