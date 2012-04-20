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
  os << indent << "Controller: " << this->Controller <<  endl;
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
  //get the information needed to determine which subsection of the full
  //data set we need to load
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  this->Internal->UpdatePiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->Internal->UpdateNumPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  return this->Superclass::RequestData(request,inputVector,outputVector);
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

  if( this->ReadPartSizes())
    {
    vtkErrorMacro( "Could not read cell sizes." );
    return 1;
    }

  if ( this->ReadConnectivityAndMaterial() )
    {
    vtkErrorMacro( "Could not read connectivity." );
    return 1;
    }

  //finalize the topology on each process, each process will  remove
  //any part that it doesn't have a cell for.
  this->Parts->FinalizeTopology();

  if(this->ReadNodes())
    {
    vtkErrorMacro("Could not read static node values.");
    return 1;
    }


  // we need to read the user ids after we have read the topology
  // so we know how many cells are in each part
  if ( this->ReadUserIds() )
    {
    vtkErrorMacro( "Could not read user node/element IDs." );
    return 1;
    }


  return 0;
}

//----------------------------------------------------------------------------
//determine which parts will be read by this processor
void vtkPLSDynaReader::GetPartRanges(vtkIdType* mins, vtkIdType* maxs)
{
  //1 == load the whole data
  //determine which domains in this mesh this processor is responsible for
  if ( this->Internal->UpdateNumPieces > 1 )
    {
    double numCells;
    for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
      {
      numCells = static_cast<double>(this->P->NumberOfCells[i]);
      if(numCells > 1000)
        {
        double percent = (1.0 / this->Internal->UpdateNumPieces) * numCells;
        mins[i] = static_cast<vtkIdType>(
                    percent * this->Internal->UpdatePiece);
        maxs[i] = static_cast<vtkIdType>(
                    percent * (this->Internal->UpdatePiece+1));
        }
      else
        {
        //else not enough cells to worth dividing the reading
        mins[i]=0;
        maxs[i]=static_cast<vtkIdType>(
                  (this->Internal->ProcessRank==0)?numCells:0);
        }
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
