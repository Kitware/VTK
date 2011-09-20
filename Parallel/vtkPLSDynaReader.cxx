/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLSDynaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// NOTE TO DEVELOPERS: ========================================================
//
// This is a parallel version of the LSDynaReader.
// Its primary tasks are to determine which parts should be read on each process
// and to send the relevant information from the master node to all slave nodes


#include "vtkPLSDynaReader.h"
#include "LSDynaMetaData.h"
#include "vtkLSDynaPartCollection.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPLSDynaReader);

struct vtkPLSDynaReader::vtkPLSDynaReaderInternal
{  
  unsigned int MinDataset;
  unsigned int MaxDataset;
  unsigned int UpdatePiece;
  unsigned int UpdateNumPieces;

  vtkIdType NumProcesses;
  vtkIdType ProcessRank;

  vtkPLSDynaReaderInternal():
    MinDataset(0),
    MaxDataset(0),
    UpdatePiece(0),
    UpdateNumPieces(0)
    {}  
};


//-----------------------------------------------------------------------------
vtkPLSDynaReader::vtkPLSDynaReader()
{
  this->Controller = NULL;
  
  //need to construct the internal datastructure before call SetController
  this->Internal = new vtkPLSDynaReader::vtkPLSDynaReaderInternal();  
  this->SetController(vtkMultiProcessController::GetGlobalController());

  
}

//-----------------------------------------------------------------------------
vtkPLSDynaReader::~vtkPLSDynaReader()
{
  this->SetController(NULL);

  delete this->Internal;
}

void vtkPLSDynaReader::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
void vtkPLSDynaReader::SetController(vtkMultiProcessController *c)
{
  if ((c == NULL) || (c->GetNumberOfProcesses() == 0))
    {
    this->Internal->NumProcesses = 1;
    this->Internal->ProcessRank = 0;
    }

  if (this->Controller == c)
    {
    return;
    }

  this->Modified();

  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }

  if (c == NULL)
    {
    return;
    }

  this->Controller = c;

  c->Register(this);
  this->Internal->NumProcesses = c->GetNumberOfProcesses();
  this->Internal->ProcessRank = c->GetLocalProcessId();
}

//-----------------------------------------------------------------------------
int vtkPLSDynaReader::CanReadFile( const char* fname )
{
  return this->Superclass::CanReadFile(fname);
}

//-----------------------------------------------------------------------------
int vtkPLSDynaReader::RequestInformation( vtkInformation* request,
                                         vtkInformationVector** iinfo,
                                         vtkInformationVector* outputVector )
{
  LSDynaMetaData* p = this->P;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  //call the parents request information on all the nodes.
  //This is not optimal, but sooo much information is stored in the
  //metadata that is read during request information that sending it over the wire
  //might not be faster than each node contending for the info. Plus it would
  //be a massive chunk of code
  this->Superclass::RequestInformation(request,iinfo,outputVector);

  //force an override of the serial reader setting the number of pieces to 1
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
              -1);
  return 1;
  
}

//-----------------------------------------------------------------------------
int vtkPLSDynaReader::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  this->Internal->UpdatePiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->Internal->UpdateNumPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  LSDynaMetaData* p = this->P;
  if ( ! p->FileIsValid )
    {
    // This should have been set in RequestInformation()
    return 0;
    }

  vtkMultiBlockDataSet* mbds = NULL;
  mbds = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()) );

  //setup timesteps
  if ( outInfo->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() ) )
    {
    // Only return single time steps for now.
    double* requestedTimeSteps = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() );
    int timeStepLen = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    double* timeSteps = outInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    int cnt = 0;
    while ( cnt < timeStepLen - 1 && timeSteps[cnt] < requestedTimeSteps[0] )
      {
      ++cnt;
      }
    this->SetTimeStep( cnt );
    outInfo->Set( vtkDataObject::DATA_TIME_STEPS(), &p->TimeValues[ p->CurrentState ], 1 );
    }

  //setup the parts that this process will read
  this->UpdateProgress( 0.0 );  
  if(this->ReadStaticNodes())
    {
    vtkErrorMacro( "Problem reading nodal info " << p->CurrentState );
    return 1;
    }

  this->UpdateProgress( 0.2 );
  if(this->ReadTopology())
    {
    vtkErrorMacro( "Problem reading topology info " << p->CurrentState );
    return 1;
    }

  this->UpdateProgress( 0.3 );
  if ( this->ReadState( p->CurrentState ) )
    {
    vtkErrorMacro( "Problem reading state data for time step " << p->CurrentState );
    return 1;
    }
  
  // II. Cell Death State
  this->UpdateProgress( 0.6 );
  if ( this->ReadDeletion() )
    {
    vtkErrorMacro( "Problem reading deletion state." );
    return 1;
    }

  //now that the reading has finished we need to finalize the parts
  //this means get the subset of points for each part, and fixup all the cells
  this->UpdateProgress( 0.8 );
  this->Parts->Finalize(this->CommonPoints,this->RemoveDeletedCells);

  //add all the parts as child blocks to the output
  vtkIdType nextId = 0;
  int size = this->Parts->GetNumberOfParts();
  for(int i=0; i < size;++i)
    {
    if (this->Parts->IsActivePart(i))
      {
      mbds->SetBlock(nextId,this->Parts->GetGridForPart(i));
      mbds->GetMetaData(nextId)->Set(vtkCompositeDataSet::NAME(),
        this->P->PartNames[i].c_str());
      ++nextId;
      }
    }

  this->UpdateProgress( 1.0 );
  return 1;

}

//----------------------------------------------------------------------------
int vtkPLSDynaReader::ReadStaticNodes()
{
  //we only need to read the common points on the first node.
  //we will than transmit this info to the rest of the nodes when
  //we read nodal information on each state section.
  //This is done because if deformed mesh is enabled the points can
  //change for each time step
  if(this->Internal->ProcessRank==0 && !this->Parts)
    {
    if ( this->ReadNodes() )
      {
      vtkErrorMacro( "Could not read nodal coordinates." );
      return 1;
      }
    
    // Do something with user-specified node/element/material numbering
    if ( this->ReadUserIds() )
      {
      vtkErrorMacro( "Could not read user node/element IDs." );
      return 1;
      } 
    }  
  return 0;
}

//----------------------------------------------------------------------------
int vtkPLSDynaReader::ReadTopology()
{
  bool readTopology=false;
  if(!this->Parts)
    {
    readTopology=true;
    this->Parts = vtkLSDynaPartCollection::New();
    this->Parts->SetMetaData(this->P);
    this->GetPartRange(this->Parts->GetNumberOfParts());
    }
  if(!readTopology)
    {
    return 0;
    }

  if(this->Internal->ProcessRank==0)
    {
    //Read connectivity info once per simulation run and cache it.
    //if the filename or a part/material array is modified the cache is deleted
    if ( this->ReadConnectivityAndMaterial() )
      {
      vtkErrorMacro( "Could not read connectivity." );
      return 1;
      }
    }

  if(this->Internal->NumProcesses > 1)
    {
    //currently the master node has all the topology information.
    //what we are going to do next is is send the information for each part
    //to all processes. Than during FinalizeTopology the part will remove
    //any part information it doesn't need to store.
    const int size(this->Parts->GetNumberOfParts());
    vtkIdType cellSizes[2]; //number of cells, and total cell array size

    for(int i=0; i<size; ++i)
      {
      this->Parts->SpaceNeededForBroadcast(i,cellSizes[0],cellSizes[1]);
      this->Controller->Broadcast(cellSizes,2,0);

      //reserve the space on each 
      this->Parts->ReserveSpace(i,cellSizes[0],cellSizes[1]);
    
      //send out the topology info now
      unsigned char* types=NULL;
      vtkIdType *loc=NULL, *structure=NULL;
      this->Parts->GetInfoForBroadcast(i,types,loc,structure);

      //broadcast the topology
      this->Controller->Broadcast(types,cellSizes[0],0);
      this->Controller->Broadcast(loc,cellSizes[0],0);
      this->Controller->Broadcast(structure,cellSizes[1],0);
      }
    }
  
  //finalize the topology on each process, each process will  remove
  //any information it was sent that it doesn't need based on the 
  //parts it is supposed to load
  this->Parts->FinalizeTopology();


  return 0;
}

//----------------------------------------------------------------------------
//determine which parts will be read by this processor
void vtkPLSDynaReader::GetPartRange(const vtkIdType& numParts)
{
  this->Parts->PartMinId = 0;
  this->Parts->PartMaxId = numParts;

  //1 == load the whole data
  if ( this->Internal->UpdateNumPieces > 1 )
    {
    //determine which domains in this mesh this processor is responsible for
    float percent = (1.0 / this->Internal->UpdateNumPieces) * numParts;
    this->Parts->PartMinId = percent * this->Internal->UpdatePiece;
    this->Parts->PartMaxId = (percent * this->Internal->UpdatePiece) + percent;
    }
}
