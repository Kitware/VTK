/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRBaseReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRBaseReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkCallbackCommand.h"

#include "vtkAMRUtilities.h"

#include <cassert>

vtkAMRBaseReader::vtkAMRBaseReader()
{

}

//------------------------------------------------------------------------------
vtkAMRBaseReader::~vtkAMRBaseReader()
{
  this->PointDataArraySelection->RemoveObserver( this->SelectionObserver );
  this->CellDataArraySelection->RemoveObserver( this->SelectionObserver );
  this->SelectionObserver->Delete( );
  this->CellDataArraySelection->Delete( );
  this->PointDataArraySelection->Delete( );
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::FillOutputPortInformation(
    int vtkNotUsed(port),vtkInformation *info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHierarchicalBoxDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRBaseReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRBaseReader::Initialize()
{
  this->SetNumberOfInputPorts( 0 );
  this->FileName      = NULL;
  this->MaxLevel      = 0;
  this->LoadParticles = 1;

  this->CellDataArraySelection  = vtkDataArraySelection::New();
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver       = vtkCallbackCommand::New();
//  this->SelectionObserver->SetCallback(
//      &vtkAMRBaseReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData( this );
  this->CellDataArraySelection->AddObserver(
     vtkCommand::ModifiedEvent,this->SelectionObserver );
  this->PointDataArraySelection->AddObserver(
     vtkCommand::ModifiedEvent, this->SelectionObserver );

}

//----------------------------------------------------------------------------
//void vtkAMRBaseReader::SelectionModifiedCallback(
//    vtkObject*, unsigned long, void* clientdata, void*)
//{
//  static_cast<vtkAMRBaseReader*>(clientdata)->Modified();
//}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::GetNumberOfPointArrays()
{
  return( this->PointDataArraySelection->GetNumberOfArrays() );
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::GetNumberOfCellArrays()
{
  return( this->CellDataArraySelection->GetNumberOfArrays() );
}

//------------------------------------------------------------------------------
const char* vtkAMRBaseReader::GetPointArrayName(int index)
{
  return( this->PointDataArraySelection->GetArrayName( index ) );
}

//------------------------------------------------------------------------------
const char* vtkAMRBaseReader::GetCellArrayName(int index)
{
  return( this->CellDataArraySelection->GetArrayName( index )  );
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::GetPointArrayStatus(const char* name)
{
  return( this->PointDataArraySelection->ArrayIsEnabled( name ) );
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::GetCellArrayStatus(const char* name)
{
  return( this->CellDataArraySelection->ArrayIsEnabled( name ) );
}

//------------------------------------------------------------------------------
void vtkAMRBaseReader::SetPointArrayStatus(const char* name, int status)
{

  if( status )
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }

}

//------------------------------------------------------------------------------
void vtkAMRBaseReader::SetCellArrayStatus(const char* name, int status)
{
  if( status )
    {
    this->CellDataArraySelection->EnableArray( name );
    }
  else
    {
    this->CellDataArraySelection->DisableArray( name );
    }
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::GetBlockProcessId( const int blockIdx )
{
  // If this is reader instance is serial, return Process 0
  // as the Process ID for the corresponding block.
  if( this->Controller == NULL )
    return 0;

  int N = this->Controller->GetNumberOfProcesses();
  return( blockIdx%N );
}

//------------------------------------------------------------------------------
bool vtkAMRBaseReader::IsBlockMine( const int blockIdx )
{
  // If this reader instance does not run in parallel, then,
  // all blocks are owned by this reader.
  if( this->Controller == NULL )
    return true;

  int myRank = this->Controller->GetLocalProcessId();
  if( myRank == this->GetBlockProcessId( blockIdx ) )
    return true;
  return false;
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::RequestData(
        vtkInformation* vtkNotUsed(request),
        vtkInformationVector** vtkNotUsed(inputVector),
        vtkInformationVector* outputVector )
{
  vtkInformation            *outInf = outputVector->GetInformationObject( 0 );
  vtkHierarchicalBoxDataSet *output =
    vtkHierarchicalBoxDataSet::SafeDownCast(
     outInf->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output AMR dataset is NULL" && ( output != NULL ) );

  this->ReadMetaData();
  this->GenerateBlockMap();

  vtkstd::vector< int > idxcounter;
  idxcounter.resize(this->GetNumberOfLevels(), 0);
  for( int block=0; block < this->GetNumberOfBlocks(); ++block )
    {

      if( this->IsBlockMine(block) )
        {
          this->GetBlock( block, output, idxcounter );
        }

    } // END for all blocks

  // Generate all the AMR metadata & the visibility arrays
  vtkAMRUtilities::GenerateMetaData( output, this->Controller );
  output->GenerateVisibilityArrays();

  // If this instance of the reader is not parallel, block until all processes
  // read their blocks.
  if( this->Controller != NULL )
    this->Controller->Barrier();

  return 1;
}
