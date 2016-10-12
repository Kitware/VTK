/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredAMRNeighbor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestStructuredAMRNeighbor.cxx -- Test vtkStructuredAMRNeighbor
//
// .SECTION Description
//  A simple test for structured AMR neighbor

// C++ includes
#include <iostream>
#include <sstream>
#include <string>

// VTK includes
#include "vtkStructuredAMRNeighbor.h"

#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

std::string StringExtent(int ext[6])
{
  std::ostringstream oss;
  oss.clear(); oss.str("");
  for( int i=0; i < 3; ++i )
  {
    oss << "[" << ext[i*2] << " " << ext[i*2+1] << "] ";
  }
  return( oss.str() );
}

//------------------------------------------------------------------------------
int CheckExtents(int ext[6], int exp[6])
{
  std::cout << "CHECKING EXTENT: " << StringExtent(ext);
  std::cout << "EXPECTED EXTENT: " << StringExtent(exp);
  std::cout << "...";
  std::cout.flush();
  for( int i=0; i < 6; ++i )
  {
    if( ext[i] != exp[i ] )
    {
      std::cout << "[ERROR]\n";
      std::cout.flush();
      return 1;
    } // END if
  } // END for
  std::cout << "[OK]\n";
  std::cout.flush();
  return 0;
}

//------------------------------------------------------------------------------
int TestParentNeighbor()
{
  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::PARENT;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 1;
  int neiLevel  = 0;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent) = JMIN(gridExtent) = 4;
  IMAX(gridExtent) = JMAX(gridExtent) = 10;

  IMIN(neiExtent)  = JMIN(neiExtent) = 0;
  IMAX(neiExtent)  = JMAX(neiExtent) = 8;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap) = JMIN(gridOverlap) = 4;
  IMAX(gridOverlap) = JMAX(gridOverlap) = 10;

  IMIN(neiOverlap)  = JMIN(neiOverlap) = 2;
  IMAX(neiOverlap)  = JMAX(neiOverlap) = 5;

  // STEP 3: Setup orientation tuple
  orientation[0] = orientation[1] = vtkStructuredNeighbor::SUBSET_BOTH;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent) = JMIN(ExpectedSendExtent) = 4;
  IMAX(ExpectedSendExtent) = JMAX(ExpectedSendExtent) = 10;

  IMIN(ExpectedRcvExtent)  = JMIN(ExpectedRcvExtent)  = 1;
  IMAX(ExpectedRcvExtent)  = JMAX(ExpectedRcvExtent)  = 6;

   // STEP 5: Setup AMR neighbor
   vtkStructuredAMRNeighbor amrnei(
     gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

   // STEP 6: Compute send/receive extents
   amrnei.ComputeSendAndReceiveExtent(
       gridExtent,gridExtent,neiExtent,wholeExtent,NG);

   // STEP 7: Check expected extents
   rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
   rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );
}

//------------------------------------------------------------------------------
int TestPartiallyOverlappingParent()
{
  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_PARENT;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 1;
  int neiLevel  = 0;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent) = JMIN(gridExtent) = 4;
  IMAX(gridExtent) = JMAX(gridExtent) = 10;

  IMIN(neiExtent)  = JMIN(neiExtent) = 0;
  IMAX(neiExtent)  = 3; JMAX(neiExtent) = 8;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap) = JMIN(gridOverlap) = 4;
  IMAX(gridOverlap) = 6; JMAX(gridOverlap) = 10;

  IMIN(neiOverlap)  = JMIN(neiOverlap) = 2;
  IMAX(neiOverlap)  = 3; JMAX(neiOverlap) = 5;

  // STEP 3: Setup orientation tuple
  orientation[0] = vtkStructuredNeighbor::SUBSET_LO;
  orientation[1] = vtkStructuredNeighbor::SUBSET_BOTH;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent) = JMIN(ExpectedSendExtent) = 4;
  IMAX(ExpectedSendExtent) = 7; JMAX(ExpectedSendExtent) = 10;

  IMIN(ExpectedRcvExtent)  = JMIN(ExpectedRcvExtent)  = 1;
  IMAX(ExpectedRcvExtent)  = 3; JMAX(ExpectedRcvExtent)  = 6;

  // STEP 5: Setup AMR neighbor
  vtkStructuredAMRNeighbor amrnei(
    gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

  // STEP 6: Compute send/receive extents
  amrnei.ComputeSendAndReceiveExtent(
      gridExtent,gridExtent,neiExtent,wholeExtent,NG);

  // STEP 7: Check expected extents
  rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
  rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );
}

//------------------------------------------------------------------------------
int TestChildNeighbor()
{
  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::CHILD;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 0;
  int neiLevel  = 1;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent) = JMIN(gridExtent) = 0;
  IMAX(gridExtent) = JMAX(gridExtent) = 8;

  IMIN(neiExtent)  = JMIN(neiExtent) = 4;
  IMAX(neiExtent)  = JMAX(neiExtent) = 10;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap) = JMIN(gridOverlap) = 2;
  IMAX(gridOverlap) = JMAX(gridOverlap) = 5;

  IMIN(neiOverlap)  = JMIN(neiOverlap) = 4;
  IMAX(neiOverlap)  = JMAX(neiOverlap) = 10;

  // STEP 3: Setup orientation tuple
  orientation[0] = orientation[1] = vtkStructuredNeighbor::SUPERSET;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent) = JMIN(ExpectedSendExtent) = 1;
  IMAX(ExpectedSendExtent) = JMAX(ExpectedSendExtent) = 6;

  IMIN(ExpectedRcvExtent)  = JMIN(ExpectedRcvExtent)  = 4;
  IMAX(ExpectedRcvExtent)  = JMAX(ExpectedRcvExtent)  = 10;

   // STEP 5: Setup AMR neighbor
   vtkStructuredAMRNeighbor amrnei(
     gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

   // STEP 6: Compute send/receive extents
   amrnei.ComputeSendAndReceiveExtent(
       gridExtent,gridExtent,neiExtent,wholeExtent,NG);

   // STEP 7: Check expected extents
   rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
   rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );

}

//------------------------------------------------------------------------------
int TestPartiallyOverlappingChild()
{
  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_CHILD;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 0;
  int neiLevel  = 1;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent)  = JMIN(gridExtent) = 0;
  IMAX(gridExtent)  = 3; JMAX(gridExtent) = 8;

  IMIN(neiExtent) = JMIN(neiExtent) = 4;
  IMAX(neiExtent) = JMAX(neiExtent) = 10;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap)  = JMIN(gridOverlap) = 2;
  IMAX(gridOverlap)  = 3; JMAX(gridOverlap) = 5;

  IMIN(neiOverlap) = JMIN(neiOverlap) = 4;
  IMAX(neiOverlap) = 6; JMAX(neiOverlap) = 10;

  // STEP 3: Setup orientation tuple
  orientation[0] = vtkStructuredNeighbor::SUBSET_HI;
  orientation[1] = vtkStructuredNeighbor::SUPERSET;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent)  = JMIN(ExpectedSendExtent)  = 1;
  IMAX(ExpectedSendExtent)  = 3; JMAX(ExpectedSendExtent)  = 6;

  IMIN(ExpectedRcvExtent) = JMIN(ExpectedRcvExtent) = 4;
  IMAX(ExpectedRcvExtent) = 7; JMAX(ExpectedRcvExtent) = 10;

  // STEP 5: Setup AMR neighbor
  vtkStructuredAMRNeighbor amrnei(
    gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

  // STEP 6: Compute send/receive extents
  amrnei.ComputeSendAndReceiveExtent(
      gridExtent,gridExtent,neiExtent,wholeExtent,NG);

  // STEP 7: Check expected extents
  rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
  rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );
}

//------------------------------------------------------------------------------
int TestSameLevelSibling()
{
  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::SAME_LEVEL_SIBLING;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 1;
  int neiLevel  = 1;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent)  = 10; JMIN(gridExtent) = 8;
  IMAX(gridExtent)  = JMAX(gridExtent) = 14;

  IMIN(neiExtent) = JMIN(neiExtent) = 4;
  IMAX(neiExtent) = JMAX(neiExtent) = 10;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap) = IMAX(gridOverlap) = 10;
  JMIN(gridOverlap) = 8; JMAX(gridOverlap) = 10;

  IMIN(neiOverlap) = IMAX(neiOverlap) = 10;
  JMIN(neiOverlap) = 8; JMAX(neiOverlap) = 10;


  // STEP 3: Setup orientation tuple
  orientation[0] = vtkStructuredNeighbor::LO;
  orientation[1] = vtkStructuredNeighbor::SUBSET_LO;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent)  = 10; JMIN(ExpectedSendExtent)  = 8;
  IMAX(ExpectedSendExtent)  = 11; JMAX(ExpectedSendExtent)  = 11;

  IMIN(ExpectedRcvExtent) = 9; JMIN(ExpectedRcvExtent) = 7;
  IMAX(ExpectedRcvExtent) = JMAX(ExpectedRcvExtent) = 10;

  // STEP 5: Setup AMR neighbor
  vtkStructuredAMRNeighbor amrnei(
    gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

  // STEP 6: Compute send/receive extents
  amrnei.ComputeSendAndReceiveExtent(
      gridExtent,gridExtent,neiExtent,wholeExtent,NG);

  // STEP 7: Check expected extents
  rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
  rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );
}

//------------------------------------------------------------------------------
int TestCoarseToFineNeighbor()
{

  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::COARSE_TO_FINE_SIBLING;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 0;
  int neiLevel  = 1;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent)  = JMIN(gridExtent) = 0;
  IMAX(gridExtent)  = JMAX(gridExtent) = 2;

  IMIN(neiExtent) = 4; JMIN(neiExtent) = 0;
  IMAX(neiExtent) = 8; JMAX(neiExtent) = 4;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap) = IMAX(gridOverlap) = 2;
  JMIN(gridOverlap) = 0; JMAX(gridOverlap) = 2;

  IMIN(neiOverlap) = IMAX(neiOverlap) = 4;
  JMIN(neiOverlap) = 0; JMAX(neiOverlap) = 4;


  // STEP 3: Setup orientation tuple
  orientation[0] = vtkStructuredNeighbor::HI;
  orientation[1] = vtkStructuredNeighbor::ONE_TO_ONE;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent)  = 1; JMIN(ExpectedSendExtent)  = 0;
  IMAX(ExpectedSendExtent)  = 2; JMAX(ExpectedSendExtent)  = 2;

  IMIN(ExpectedRcvExtent) = 4; JMIN(ExpectedRcvExtent) = 0;
  IMAX(ExpectedRcvExtent) = 5; JMAX(ExpectedRcvExtent) = 4;

  // STEP 5: Setup AMR neighbor
  vtkStructuredAMRNeighbor amrnei(
    gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

  // STEP 6: Compute send/receive extents
  amrnei.ComputeSendAndReceiveExtent(
      gridExtent,gridExtent,neiExtent,wholeExtent,NG);

  // STEP 7: Check expected extents
  rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
  rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );
}

//------------------------------------------------------------------------------
int TestFineToCoarseNeighbor()
{
  int rc = 0;

  int relationship = vtkStructuredAMRNeighbor::FINE_TO_COARSE_SIBLING;

  // STEP 0: initialize AMR neighbor parameters
  const int NG = 1;

  int wholeExtent[6];
  int gridExtent[6];
  int neiExtent[6];
  KMIN(wholeExtent) = KMAX(wholeExtent) =
  KMIN(gridExtent)  = KMAX(gridExtent)  =
  KMIN(neiExtent)   = KMAX(neiExtent)   = 0;

  int gridLevel = 1;
  int neiLevel  = 0;
  int orientation[3];
  orientation[2] = vtkStructuredNeighbor::UNDEFINED;

  int gridOverlap[6];
  int neiOverlap[6];
  KMIN(gridOverlap) = KMAX(gridOverlap) =
  KMIN(neiOverlap)  = KMAX(neiOverlap)  = 0;


  int ExpectedSendExtent[6];
  int ExpectedRcvExtent[6];
  KMIN(ExpectedSendExtent) = KMAX(ExpectedSendExtent) =
  KMIN(ExpectedRcvExtent)  = KMAX(ExpectedRcvExtent)  = 0;

  // STEP 1: Setup grid extents and whole extent
  IMIN(gridExtent) = 4; JMIN(gridExtent) = 0;
  IMAX(gridExtent) = 8; JMAX(gridExtent) = 4;

  IMIN(neiExtent)  = JMIN(neiExtent) = 0;
  IMAX(neiExtent)  = JMAX(neiExtent) = 2;

  IMIN(wholeExtent) = JMIN(wholeExtent) = 0;
  IMAX(wholeExtent) = JMAX(wholeExtent) = 8;

  // STEP 2: Setup overlap extents
  IMIN(gridOverlap) = IMAX(gridOverlap) = 4;
  JMIN(gridOverlap) = 0; JMAX(gridOverlap) = 4;

  IMIN(neiOverlap) = IMAX(neiOverlap) = 2;
  JMIN(neiOverlap) = 0; JMAX(neiOverlap) = 2;

  // STEP 3: Setup orientation tuple
  orientation[0] = vtkStructuredNeighbor::LO;
  orientation[1] = vtkStructuredNeighbor::ONE_TO_ONE;

  // STEP 4: Setup expected send/rcv extents
  IMIN(ExpectedSendExtent) = 4; JMIN(ExpectedSendExtent) = 0;
  IMAX(ExpectedSendExtent) = 5; JMAX(ExpectedSendExtent) = 4;

  IMIN(ExpectedRcvExtent)  = 1; JMIN(ExpectedRcvExtent)  = 0;
  IMAX(ExpectedRcvExtent)  = 2; JMAX(ExpectedRcvExtent)  = 2;

  // STEP 5: Setup AMR neighbor
  vtkStructuredAMRNeighbor amrnei(
    gridLevel,0,neiLevel,gridOverlap,neiOverlap,orientation,relationship);

  // STEP 6: Compute send/receive extents
  amrnei.ComputeSendAndReceiveExtent(
      gridExtent,gridExtent,neiExtent,wholeExtent,NG);

  // STEP 7: Check expected extents
  rc += CheckExtents(amrnei.SendExtent,ExpectedSendExtent);
  rc += CheckExtents(amrnei.RcvExtent,ExpectedRcvExtent);
  return( rc );
}

//------------------------------------------------------------------------------
int TestAssignmentOperator()
{
  int rc = 0;

  // Allocate A with default values
  vtkStructuredAMRNeighbor A;
  if( A.GetRelationShipString() != "UNDEFINED" )
  {
    ++rc;
  }

  // Allocate B and set som ivars arbitrarily
  vtkStructuredAMRNeighbor B;
  B.GridLevel = B.NeighborLevel = 100;
  B.RelationShip = vtkStructuredAMRNeighbor::SAME_LEVEL_SIBLING;
  if( B.GetRelationShipString() != "SAME_LEVEL_SIBLING" )
  {
    ++rc;
  }

  // Reset B to initial
  B = A;

  // Ensure initial values for ivars
  if( B.GridLevel != -1 )
  {
    ++rc;
  }
  if( B.NeighborLevel != -1 )
  {
    ++rc;
  }
  if( B.RelationShip != vtkStructuredAMRNeighbor::UNDEFINED )
  {
    ++rc;
  }
  return( rc );
}

//------------------------------------------------------------------------------
int TestGetRelationShipString()
{
  int rc = 0;
  vtkStructuredAMRNeighbor A;
  A.RelationShip = vtkStructuredAMRNeighbor::PARENT;
  if( A.GetRelationShipString() != "PARENT" )
  {
    std::cerr << "Expected PARENT\n";
    ++rc;
  }

  A.RelationShip = vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_PARENT;
  if( A.GetRelationShipString() != "PARTIALLY_OVERLAPPING_PARENT" )
  {
    std::cerr << "Expected PARTIALY_OVERLAPPING_PARENT\n";
    ++rc;
  }

  A.RelationShip = vtkStructuredAMRNeighbor::CHILD;
  if( A.GetRelationShipString() != "CHILD" )
  {
    std::cerr << "Expected CHILD\n";
    ++rc;
  }

  A.RelationShip = vtkStructuredAMRNeighbor::PARTIALLY_OVERLAPPING_CHILD;
  if( A.GetRelationShipString() != "PARTIALLY_OVERLAPPING_CHILD" )
  {
    std::cerr << "Expected PARTIALLY_OVERLAPPING_CHILD\n";
    ++rc;
  }

  A.RelationShip = vtkStructuredAMRNeighbor::SAME_LEVEL_SIBLING;
  if( A.GetRelationShipString() != "SAME_LEVEL_SIBLING" )
  {
    std::cerr << "Expected SAME_LEVEL_SIBLING\n";
    ++rc;
  }

  A.RelationShip = vtkStructuredAMRNeighbor::COARSE_TO_FINE_SIBLING;
  if( A.GetRelationShipString() != "COARSE_TO_FINE_SIBLING" )
  {
    std::cerr << "Expected COARSE_TO_FINE_SIBLING\n";
    ++rc;
  }

  A.RelationShip = vtkStructuredAMRNeighbor::FINE_TO_COARSE_SIBLING;
  if( A.GetRelationShipString() != "FINE_TO_COARSE_SIBLING" )
  {
    std::cerr << "Expected FINE_TO_COARSE_SIBLING\n";
    ++rc;
  }

  return( rc );
}

//------------------------------------------------------------------------------
int TestStructuredAMRNeighbor(int argc, char *argv[])
{
  // Silence compiler warnings
  static_cast<void>(argc);
  static_cast<void>(argv);

  int rc = 0;
  rc += TestParentNeighbor();
  rc += TestPartiallyOverlappingParent();
  rc += TestChildNeighbor();
  rc += TestPartiallyOverlappingChild();
  rc += TestSameLevelSibling();
  rc += TestCoarseToFineNeighbor();
  rc += TestCoarseToFineNeighbor();
  rc += TestFineToCoarseNeighbor();
  rc += TestAssignmentOperator();
  rc += TestGetRelationShipString();
  return( rc );
}
