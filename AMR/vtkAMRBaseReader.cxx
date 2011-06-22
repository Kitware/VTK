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
#include "vtkIndent.h"
#include "vtkSmartPointer.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkStreamingDemandDrivenPipeline.h"

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
  this->FileName       = NULL;
  this->MaxLevel       = 0;
  this->LoadParticles  = 1;
  this->Controller     = vtkMultiProcessController::GetGlobalController();
  this->InitialRequest = true;

  this->CellDataArraySelection  = vtkDataArraySelection::New();
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->SelectionObserver       = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
      &vtkAMRBaseReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData( this );
  this->CellDataArraySelection->AddObserver(
     vtkCommand::ModifiedEvent,this->SelectionObserver );
  this->PointDataArraySelection->AddObserver(
     vtkCommand::ModifiedEvent, this->SelectionObserver );

}

//----------------------------------------------------------------------------
void vtkAMRBaseReader::SelectionModifiedCallback(
    vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkAMRBaseReader*>(clientdata)->Modified();
}

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
  if( !this->IsParallel() )
    return 0;

  int N = this->Controller->GetNumberOfProcesses();
  return( blockIdx%N );
}

//------------------------------------------------------------------------------
bool vtkAMRBaseReader::IsBlockMine( const int blockIdx )
{
  // If this reader instance does not run in parallel, then,
  // all blocks are owned by this reader.
  if( !this->IsParallel() )
    return true;

  int myRank = this->Controller->GetLocalProcessId();
  if( myRank == this->GetBlockProcessId( blockIdx ) )
    return true;
  return false;
}

//------------------------------------------------------------------------------
void vtkAMRBaseReader::InitializeArraySelections()
{
  if( this->InitialRequest )
    {
      this->PointDataArraySelection->DisableAllArrays();
      this->CellDataArraySelection->DisableAllArrays();
      this->InitialRequest=false;
    }
}

//------------------------------------------------------------------------------
bool vtkAMRBaseReader::IsParallel( )
{
  if( this->Controller == NULL )
    return false;

  if( this->Controller->GetNumberOfProcesses() > 1 )
    return true;

  return false;
}


//------------------------------------------------------------------------------
int vtkAMRBaseReader::RequestInformation(
    vtkInformation *rqst,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector )
{
  this->Superclass::RequestInformation( rqst, inputVector, outputVector );

  std::cout << "FILE: " << __FILE__ << " - RequestInformation\n";
  std::cout.flush();

  vtkSmartPointer<vtkHierarchicalBoxDataSet> metadata =
     vtkSmartPointer<vtkHierarchicalBoxDataSet>::New();

  vtkInformation* info = outputVector->GetInformationObject(0);
  assert( "pre: output information object is NULL" && (info != NULL) );

  this->FillMetaData( metadata );
  info->Set( vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), metadata );

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::RequestUpdateExtent(
        vtkInformation* rqst,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector )
{
  vtkInformation            *outInf = outputVector->GetInformationObject( 0 );
   vtkHierarchicalBoxDataSet *output =
     vtkHierarchicalBoxDataSet::SafeDownCast(
      outInf->Get( vtkDataObject::DATA_OBJECT() ) );
   assert( "pre: output AMR dataset is NULL" && ( output != NULL ) );
  if( outInf->Has(
      vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES() ) )
    {
      std::cout << "Got composite indices in RequestUpdateExtent()!\n";
      std::cout.flush();
    }
  else
    {
      std::cout << "No UPDATE_COMPOSITE_INDICES() in RequestUpdateExtent()!\n";
      std::cout.flush();
    }

  return( 1 );
}

//------------------------------------------------------------------------------
int vtkAMRBaseReader::RequestData(
        vtkInformation* vtkNotUsed(request),
        vtkInformationVector** vtkNotUsed(inputVector),
        vtkInformationVector* outputVector )
{
  std::cout << "FILE: " << __FILE__ << " - RequestData\n";
  std::cout.flush();


  vtkInformation            *outInf = outputVector->GetInformationObject( 0 );
  vtkHierarchicalBoxDataSet *output =
    vtkHierarchicalBoxDataSet::SafeDownCast(
     outInf->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output AMR dataset is NULL" && ( output != NULL ) );

  if( outInf->Has(
      vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES() ) )
    {
      std::cout << "Got composite indices in RequestData()!\n";
      std::cout.flush();
    }
  else
    {
      std::cout << "No UPDATE_COMPOSITE_INDICES() in RequestData()!\n";
      std::cout.flush();
    }

//  if( outInf->Has(
//      vtkStreamingDemandDrivenPipeline::UPDATE_AMR_LEVEL() ) )
//    {
//      this->MaxLevel = outInf->Get(
//          vtkStreamingDemandDrivenPipeline::UPDATE_AMR_LEVEL() );
//    }

  this->ReadMetaData();
  this->GenerateBlockMap();

  // Initialize counter of the number of blocks at each level.
  // This counter is used to compute the block index w.r.t. the
  // hierarchical box data-structure. Note that then number of blocks
  // can change based on user constraints, e.g., the number of levels
  // visible.
  vtkstd::vector< int > idxcounter;
  idxcounter.resize(this->GetNumberOfLevels()+1, 0);

  // Find the number of blocks to be processed. BlockMap.size()
  // has all the blocks that are to be processesed and may be
  // less than or equal to this->GetNumberOfBlocks(), i.e., the
  // total number of blocks.
  int numBlocks = static_cast< int >( this->BlockMap.size() );
  for( int block=0; block < numBlocks; ++block )
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
  if( this->IsParallel() )
    this->Controller->Barrier();

  outInf = NULL;
  output = NULL;
  return 1;
}
