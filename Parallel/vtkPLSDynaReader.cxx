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
#include "LSDynaFamily.h"
#include "vtkLSDynaPartCollection.h"

#include "vtkIntArray.h"
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
  int size = static_cast<int>(this->P->PartIds.size());
  for(int i=0; i < size;++i,++nextId)
    {
    if (this->Parts->IsActivePart(i))
      {
      mbds->SetBlock(nextId,this->Parts->GetGridForPart(i));
      }
    else
      {
      //some other process is reading it
      mbds->SetBlock(nextId,NULL);
      }
    mbds->GetMetaData(nextId)->Set(vtkCompositeDataSet::NAME(),
        this->P->PartNames[i].c_str());
    }
  this->P->Fam.ClearBuffer();

  if(this->CacheTopology==0)
    {
    this->Parts->Delete();
    this->Parts=NULL;

    this->CommonPoints->Delete();
    this->CommonPoints=NULL;
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
  else if(!this->CommonPoints)
    {
    this->CommonPoints = vtkPoints::New();
      if (this->P->Fam.GetWordSize() == 4 )
      {
      this->CommonPoints->SetDataTypeToFloat();
      }
   else
      {
      this->CommonPoints->SetDataTypeToDouble();
      }  
    this->CommonPoints->SetNumberOfPoints( this->P->NumberOfNodes );
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
    vtkIdType* minCellIds = new vtkIdType[LSDynaMetaData::NUM_CELL_TYPES];
    vtkIdType* maxCellIds = new vtkIdType[LSDynaMetaData::NUM_CELL_TYPES];
    this->GetPartRanges(minCellIds,maxCellIds);

    this->Parts->InitCollection(this->P,minCellIds,maxCellIds);
    delete[] minCellIds;
    delete[] maxCellIds;
    }
  if(!readTopology)
    {
    return 0;
    }

  //Read connectivity info once per simulation run and cache it.
  //if the filename or a part/material array is modified the cache is deleted
  //Each node will only read the subset of the files that it needs to construct
  //the topology it needs
  if ( this->ReadConnectivityAndMaterial() )
    {
    vtkErrorMacro( "Could not read connectivity." );
    return 1;
    }

  //finalize the topology on each process, each process will  remove
  //any part that it doesn't have a cell for.
  this->Parts->FinalizeTopology();

  return 0;
}

//-----------------------------------------------------------------------------
void vtkPLSDynaReader::ReadPointProperty(vtkDataArray *arr,
  const vtkIdType& numTuples, const vtkIdType& numComps, const bool &valid,
  const bool& isDeflectionArray)
{
  if(this->Internal->NumProcesses == 1)
    {
    //we only have the root so just call the serial code
    this->Superclass::ReadPointProperty(arr,numTuples,numComps,valid,
                                        isDeflectionArray);
    }
  else if ( valid || isDeflectionArray)
    {
    arr->SetNumberOfTuples( numTuples );  
    
    const vtkIdType numPointsToRead(262144);
    const vtkIdType loopTimes(numTuples/numPointsToRead);
    const vtkIdType leftOver(numTuples%numPointsToRead);
    const vtkIdType len (this->P->Fam.GetWordSize()*numPointsToRead*numComps);

    //setup the buffer on the non reading node
    unsigned char* buffer = NULL;
    if(this->Internal->ProcessRank>0)
      {
      buffer=new unsigned char[len];
      }
    
    if(this->P->Fam.GetWordSize() == 8)
      {
      this->ReadPointPropertyChunks((double*)buffer,arr,
           numComps,loopTimes,numPointsToRead,leftOver);
      }
    else
      {
      this->ReadPointPropertyChunks((float*)buffer,arr,
           numComps,loopTimes,numPointsToRead,leftOver);
      }
    
    if(this->Internal->ProcessRank>0)
      {
      delete[] buffer;
      }
    
    if (isDeflectionArray)
      {
      // Replace point coordinates with deflection (don't add to points).
      // The name "deflection" is misleading.
      this->CommonPoints->SetData(arr);
      }
    if(valid)
      {
      this->Parts->AddPointArray(arr);
      }    
    }
  else
    {
    this->P->Fam.SkipWords(numTuples * numComps);
    }
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkPLSDynaReader::ReadPointPropertyChunks(T* buffer, vtkDataArray *arr,
  const vtkIdType& numComps, const vtkIdType& loopTimes,
  const vtkIdType& numPointsToRead, const vtkIdType& leftOver)
{

  const vtkIdType bufferSize(numPointsToRead*numComps);
  vtkIdType offset=0;
  for(vtkIdType i=0;i<loopTimes;++i,offset+=bufferSize)
    {
    if(this->Internal->ProcessRank==0)
      {
      this->P->Fam.BufferChunk(LSDynaFamily::Float,bufferSize);
      buffer = (T*)this->P->Fam.GetRawBuffer();
      }
    else
      {
      this->P->Fam.SkipWords(bufferSize);
      }
    //broadcast the buffer from the root node to all other nodes
    this->Controller->Broadcast(buffer,bufferSize,0);
    this->FillArray(buffer,arr,offset,numPointsToRead,numComps);
    }
    
  //read the last amount
  if(this->Internal->ProcessRank==0)
    { 
    this->P->Fam.BufferChunk(LSDynaFamily::Float, leftOver*numComps);
    buffer = (T*)this->P->Fam.GetRawBuffer();
    }
  else
    {
    this->P->Fam.SkipWords(leftOver*numComps);
    }
  this->Controller->Broadcast(buffer,leftOver*numComps,0);
  this->FillArray(buffer,arr,offset,leftOver,numComps);
}


//-----------------------------------------------------------------------------
template<typename T>
void vtkPLSDynaReader::FillArray(T *buffer, vtkDataArray* arr, const vtkIdType& offset,
  const vtkIdType& numTuples, const vtkIdType& numComps)
{
  LSDynaMetaData *p = this->P;
  if(numComps==arr->GetNumberOfComponents())
    {    
    //if the numComps of the binary file and the number of components
    //of the destination in memory match ( ie we aren't a 2d binary d3plot file)
    //we can do a direct memcpy
    void *dest = arr->GetVoidPointer(offset);
    void *src = buffer;
    vtkIdType size = (numTuples * numComps) * p->Fam.GetWordSize();
    memcpy(dest,src,size);    
    }
  else
    {
    double tuple[3] = {0.0,0.0,0.0};
    const vtkIdType startPos(offset/numComps);
    const vtkIdType size(numTuples*numComps);
    
    for (vtkIdType pt=0; pt<size; pt+=numComps)
      {
      //in some use cases we have a 3 dimension tuple that we need to fill
      //that is coming from a 2 dimension buffer
      for ( int c=0; c<numComps; ++c )
        {
        tuple[c] = buffer[pt+c];
        }
      arr->SetTuple(pt+startPos, tuple );
      }
    }
}

//----------------------------------------------------------------------------
//determine which parts will be read by this processor
void vtkPLSDynaReader::GetPartRanges(vtkIdType* mins, vtkIdType* maxs)
{
  //1 == load the whole data
  //determine which domains in this mesh this processor is responsible for
  if ( this->Internal->UpdateNumPieces > 1 )
    {
    vtkIdType numCells;
    for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
      {
      numCells = this->P->NumberOfCells[i];
      float percent = (1.0 / this->Internal->UpdateNumPieces) * numCells;
      mins[i] = percent * this->Internal->UpdatePiece;
      maxs[i] = (percent * this->Internal->UpdatePiece) + percent;
      }
    }
  else
    {
    for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
      {
      mins[i] = 0;
      maxs[i] = this->P->NumberOfCells[i];
      }
    }
}
