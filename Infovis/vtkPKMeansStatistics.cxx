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
#include "vtkToolkits.h"

#include "vtkPKMeansStatistics.h"
#include "vtkKMeansStatistics.h"
#include "vtkKMeansDistanceFunctor.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCommunicator.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"


vtkStandardNewMacro(vtkPKMeansStatistics);
vtkCxxRevisionMacro(vtkPKMeansStatistics, "1.2");
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
      double numChanges = 0.0;
      for( int j = 0; j < np; j++ )
        {
        numChanges = numChanges +
          (static_cast<double>(globalIntElements[j*totalIntElements+runID])-numChanges)/static_cast<double>(j+1);
        }
        numMembershipChanges->SetValue( runID, static_cast<vtkIdType>( numChanges ) );
      }
    }

  vtkIdType numCols = newClusterElements->GetNumberOfColumns();
  vtkIdType numRows = newClusterElements->GetNumberOfRows();
  vtkIdType numElements = numCols*numRows;

  vtkDoubleArray *totalError = vtkDoubleArray::New(); 
  totalError->SetNumberOfTuples( numRows );
  totalError->SetNumberOfComponents( 1 );
  com->AllReduce( error, totalError, vtkCommunicator::SUM_OP );
  for(unsigned int i=0; i < 4; i++)
    {
    cout << "UpdateClusterCenters::" << com->GetLocalProcessId() << "::" << i << " = " << error->GetValue( i ) << ", " << totalError->GetValue( i ) << endl;
    }

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
