/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNonBlockingCommunication.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestNonBlockingCommunication.cxx -- Tests non-blocking MPI comm.
//
// .SECTION Description
//  This test performs a non-blocking communication between 2 processes using
//  the following pattern where:
//  (1) Each process posts its receives
//  (2) The posts its sends
//  (3) Does a wait-all

// VTK includes
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"

// C++ includes
#include <cassert>

// MPI
#include <mpi.h>

void FillArray( const int rank, const int size, double array[] )
{
  for( int i=0; i < size; ++i )
    {
    array[ i ] = (rank+1)*(i+1);
    } // END for
}

//------------------------------------------------------------------------------
int main( int argc, char **argv )
{
  vtkMPIController *myController = vtkMPIController::New();
  myController->Initialize( &argc, &argv, 0);

  double sndarray[10];
  double rcvarray[10];
  double expected[10];

  vtkMPICommunicator::Request requests[2];

  int N        = myController->GetNumberOfProcesses();
  int Rank     = myController->GetLocalProcessId();
  int SendRank = (Rank==0)? 1 : 0;
  if( N != 2 )
    {
    cerr << "This test must be run with 2 MPI processes!\n";
    myController->Finalize();
    myController->Delete();
    return(-1);
    }
  assert("pre: N must be 2" && (N==2));
  assert("pre: Rank is out-of-bounds" && (Rank >= 0) && (Rank < N) );

  cout << "Filling arrays...";
  cout.flush();
  FillArray( Rank, 10, sndarray );
  FillArray( SendRank, 10, expected );
  cout << "[DONE]\n";
  cout.flush();

  // Post receives
  cout << "Posting receives....\n";
  cout.flush();
  myController->NoBlockReceive( rcvarray, 10, SendRank, 0, requests[0] );

  // Post sends
  cout << "Posting sends...\n";
  cout.flush();
  myController->NoBlockSend( sndarray, 10, SendRank, 0, requests[1] );

  // Wait all
  cout << "Do a wait all!\n";
  cout.flush();
  myController->WaitAll(2,requests);

  bool arraysMatch = true;
  for( int i=0; i < 10; ++i )
    {
    if( !vtkMathUtilities::FuzzyCompare(rcvarray[i],expected[i]) )
      {
      arraysMatch = false;
      break;
      }
    }
  if( arraysMatch )
    {
    cout << "RcvArray matches expected data!\n";
    cout.flush();
    }
  else
    {
    cout << "ERROR: rcvarray does not match expected data!\n";
    cout.flush();
    }
  myController->Barrier();

  myController->Finalize();
  myController->Delete();
}

