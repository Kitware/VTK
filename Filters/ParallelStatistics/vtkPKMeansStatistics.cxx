/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPKMeansStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "vtkToolkits.h"

#include "vtkPKMeansStatistics.h"
#include "vtkKMeansStatistics.h"
#include "vtkKMeansDistanceFunctor.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkVariantArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCommunicator.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"


vtkStandardNewMacro(vtkPKMeansStatistics);
vtkCxxSetObjectMacro(vtkPKMeansStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPKMeansStatistics::vtkPKMeansStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPKMeansStatistics::~vtkPKMeansStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPKMeansStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
vtkIdType vtkPKMeansStatistics::GetTotalNumberOfObservations( vtkIdType numObservations )
{
  int np = this->Controller->GetNumberOfProcesses();
  if( np <  2 )
  {
    return numObservations;
  }
  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if ( ! com )
  {
    vtkGenericWarningMacro("No parallel communicator.");
    return numObservations;
  }

  vtkIdType totalNumObservations;
  com->AllReduce( &numObservations, &totalNumObservations, 1, vtkCommunicator::SUM_OP );
  return totalNumObservations;
}

// ----------------------------------------------------------------------
void vtkPKMeansStatistics::UpdateClusterCenters( vtkTable* newClusterElements,
                                                 vtkTable* curClusterElements,
                                                 vtkIdTypeArray* numMembershipChanges,
                                                 vtkIdTypeArray* numDataElementsInCluster,
                                                 vtkDoubleArray* error,
                                                 vtkIdTypeArray* startRunID,
                                                 vtkIdTypeArray* endRunID,
                                                 vtkIntArray* computeRun )
{

  int np = this->Controller->GetNumberOfProcesses();
  if( np < 2 )
  {
    this->Superclass::UpdateClusterCenters( newClusterElements, curClusterElements, numMembershipChanges,
                                                  numDataElementsInCluster, error, startRunID, endRunID, computeRun );
    return;
  }
  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if ( ! com )
  {
    vtkGenericWarningMacro("No parallel communicator.");
    this->Superclass::UpdateClusterCenters( newClusterElements, curClusterElements, numMembershipChanges,
                                                  numDataElementsInCluster, error, startRunID, endRunID, computeRun );
    return;
  }

  // (All) gather numMembershipChanges
  vtkIdType nm = numMembershipChanges->GetNumberOfTuples();
  vtkIdType nd = numDataElementsInCluster->GetNumberOfTuples();
  vtkIdType totalIntElements =  nm+nd ;
  vtkIdType* localIntElements = new vtkIdType[totalIntElements];
  vtkIdType* globalIntElements = new vtkIdType[totalIntElements * np];
  vtkIdType* nmPtr = numMembershipChanges->GetPointer( 0 );
  vtkIdType* ndPtr = numDataElementsInCluster->GetPointer( 0 );
  memcpy( localIntElements, nmPtr, nm*sizeof( vtkIdType ) );
  memcpy( localIntElements + nm, ndPtr, nd*sizeof( vtkIdType ) );
  com->AllGather( localIntElements, globalIntElements, totalIntElements );

  for( vtkIdType runID = 0; runID < nm; runID++ )
  {
    if ( computeRun->GetValue( runID ) )
    {
      vtkIdType numChanges = 0;
      for( int j = 0; j < np; j++ )
      {
        numChanges += globalIntElements[j*totalIntElements+runID];
      }
        numMembershipChanges->SetValue( runID, numChanges );
    }
  }

  vtkIdType numCols = newClusterElements->GetNumberOfColumns();
  vtkIdType numRows = newClusterElements->GetNumberOfRows();
  vtkIdType numElements = numCols*numRows;

  vtkDoubleArray *totalError = vtkDoubleArray::New();
  totalError->SetNumberOfTuples( numRows );
  totalError->SetNumberOfComponents( 1 );
  com->AllReduce( error, totalError, vtkCommunicator::SUM_OP );

  for( vtkIdType runID = 0; runID < startRunID->GetNumberOfTuples(); runID++ )
  {
    if( computeRun->GetValue(runID) )
    {
      for( vtkIdType i = startRunID->GetValue(runID); i < endRunID->GetValue(runID); i++ )
      {
        error->SetValue( i, totalError->GetValue( i ) );
      }
    }
  }
  totalError->Delete();

  vtkTable* allNewClusterElements = vtkTable::New();
  void* localElements = this->DistanceFunctor->AllocateElementArray( numElements );
  void* globalElements = this->DistanceFunctor->AllocateElementArray( numElements*np );
  this->DistanceFunctor->PackElements( newClusterElements, localElements );
  com->AllGatherVoidArray( localElements, globalElements, numElements, this->DistanceFunctor->GetDataType() );
  this->DistanceFunctor->UnPackElements( newClusterElements, allNewClusterElements, localElements, globalElements, np );

  for( vtkIdType runID = 0; runID < startRunID->GetNumberOfTuples(); runID++ )
  {
    if( computeRun->GetValue(runID) )
    {
      for( vtkIdType i = startRunID->GetValue(runID); i < endRunID->GetValue(runID); i++ )
      {
        newClusterElements->SetRow(i,  this->DistanceFunctor->GetEmptyTuple( numCols ) );
        vtkIdType numClusterElements = 0;
        for( int j = 0; j < np; j++ )
        {
          numClusterElements += globalIntElements[j*totalIntElements + nm + i];
          this->DistanceFunctor->PairwiseUpdate( newClusterElements, i, allNewClusterElements->GetRow( j*numRows + i ),
                                                 globalIntElements[j*totalIntElements + nm + i], numClusterElements );
        }
        numDataElementsInCluster->SetValue( i, numClusterElements );

        // check to see if need to perturb
        if( numDataElementsInCluster->GetValue( i ) == 0 )
        {
          vtkWarningMacro("cluster center " << i-startRunID->GetValue(runID)
                                            << " in run " << runID
                                            << " is degenerate. Attempting to perturb");
          this->DistanceFunctor->PerturbElement(newClusterElements,
                                                curClusterElements,
                                                i,
                                                startRunID->GetValue(runID),
                                                endRunID->GetValue(runID),
                                                0.8 ) ;
        }
      }
    }
  }
  delete [] localIntElements;
  delete [] globalIntElements;
  allNewClusterElements->Delete();
}


// ----------------------------------------------------------------------
void vtkPKMeansStatistics::CreateInitialClusterCenters(vtkIdType numToAllocate,
                                                      vtkIdTypeArray* numberOfClusters,
                                                      vtkTable* inData,
                                                      vtkTable* curClusterElements,
                                                      vtkTable* newClusterElements)
{

  int np = this->Controller->GetNumberOfProcesses();
  if( np < 2 )
  {
    this->Superclass::CreateInitialClusterCenters( numToAllocate, numberOfClusters,
                                                   inData, curClusterElements, newClusterElements );
    return;
  }
  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if ( ! com )
  {
    vtkGenericWarningMacro("No parallel communicator.");
    this->Superclass::CreateInitialClusterCenters( numToAllocate, numberOfClusters,
                                                   inData, curClusterElements, newClusterElements );
    return;
  }


  vtkIdType  myRank = com->GetLocalProcessId();
  // use node 0 to broadcast
  vtkIdType broadcastNode = 0;

  // generate data on one node only
  if( myRank == broadcastNode )
  {
    this->Superclass::CreateInitialClusterCenters( numToAllocate, numberOfClusters,
                                                   inData, curClusterElements, newClusterElements );
  }

  int numElements = numToAllocate * curClusterElements->GetNumberOfColumns() ;
  void* localElements = this->DistanceFunctor->AllocateElementArray( numElements );
  this->DistanceFunctor->PackElements( curClusterElements, localElements );
  if( !com->BroadcastVoidArray( localElements, numElements, this->DistanceFunctor->GetDataType(), broadcastNode ) )
  {
    vtkErrorMacro("Could not broadcast initial cluster coordinates");
    return;
  }

  if( myRank != broadcastNode )
  {
    vtkIdType numCols = curClusterElements->GetNumberOfColumns() ;
    this->DistanceFunctor->UnPackElements( curClusterElements, localElements, numToAllocate, numCols );
    this->DistanceFunctor->UnPackElements( newClusterElements, localElements, numToAllocate, numCols );
    for ( vtkIdType i = 0; i < numToAllocate; i++ )
    {
      numberOfClusters->InsertNextValue( numToAllocate );
    }
  }

  this->DistanceFunctor->DeallocateElementArray( localElements ) ;
}
