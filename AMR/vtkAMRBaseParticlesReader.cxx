/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRBaseParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRBaseParticlesReader.h"
#include "vtkPolyData.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <cassert>

vtkAMRBaseParticlesReader::vtkAMRBaseParticlesReader()
{

}

//------------------------------------------------------------------------------
vtkAMRBaseParticlesReader::~vtkAMRBaseParticlesReader()
{

}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSEt" );
  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::Initialize( )
{
  this->SetNumberOfInputPorts( 0 );
  this->Frequency      = 1;
  this->FilterLocation = 0;
  this->NumberOfBlocks = 0;
  this->Initialized    = false;
  this->Controller     = vtkMultiProcessController::GetGlobalController();

  for( int i=0; i < 3; ++i )
    {
      this->MinLocation[ i ] = this->MaxLocation[ i ] = 0.0;
    }

}

//------------------------------------------------------------------------------
void vtkAMRBaseParticlesReader::SetFileName( const char *fileName )
{

  if( this->FileName != NULL )
    {

      if( strcmp(this->FileName,fileName) != 0 )
        {
          this->Initialized = false;
          delete [] this->FileName;
          this->FileName = NULL;
        }
      else
        {
          return;
        }
    }

  this->FileName = new char[ strlen(fileName)+1 ];
  strcpy(this->FileName,fileName);

  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkAMRBaseParticlesReader::IsParallel()
{
  if( this->Controller != NULL && this->Controller->GetNumberOfProcesses() > 1 )
    return true;
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRBaseParticlesReader::IsBlockMine( const int blkIdx )
{
  if( !this->IsParallel() )
    return true;

  int myRank = this->Controller->GetLocalProcessId();
  if( myRank == this->GetBlockProcessId( blkIdx ) )
    return true;
  return false;
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::GetBlockProcessId( const int blkIdx )
{
  if( !this->IsParallel() )
    return 0;

  int N = this->Controller->GetNumberOfProcesses();
  return( blkIdx%N );
}

//------------------------------------------------------------------------------
bool vtkAMRBaseParticlesReader::CheckLocation(
    const double x, const double y, const double z )
{
  if( !this->FilterLocation )
    return true;

  double coords[3];
  coords[0] = x;
  coords[1] = y;
  coords[2] = z;

  for( int i=0; i < 3; ++i )
    {
      if( this->MinLocation[i] > coords[i] || coords[i] > this->MaxLocation[i] )
        return false;
    } // END for all dimensions

  return true;
}

//------------------------------------------------------------------------------
int vtkAMRBaseParticlesReader::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector  )
{
  // STEP 0: Get the output object
  vtkInformation *outInf     = outputVector->GetInformationObject( 0 );
  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(
      outInf->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output multi-block dataset object is NULL" && (mbds != NULL) );

  // STEP 1: Read Meta-Data
  this->ReadMetaData();

  // STEP 2: Read blocks
  mbds->SetNumberOfBlocks( this->NumberOfBlocks );
  for( unsigned int blkidx=0; blkidx < this->NumberOfBlocks; ++blkidx )
    {

      if( this->IsBlockMine( blkidx ) )
        {
          vtkPolyData *particles = this->ReadParticles( blkidx );
          assert( "particles dataset should not be NULL!" &&
                   (particles != NULL) );

          mbds->SetBlock( blkidx, particles );
          particles->Delete();
        }
      else
        {
          mbds->SetBlock( blkidx, NULL );
        }

    } // END for all blocks

  // STEP 3: Synchronize
  if( this->IsParallel( ) )
    this->Controller->Barrier();

  return 1;
}


