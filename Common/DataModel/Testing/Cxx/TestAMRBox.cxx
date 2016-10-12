/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRBox.h"
#include "vtkStructuredData.h"

#include <iostream>
#include <string>

void Construct2DAMRBox(
    vtkAMRBox& box,int lo[3],int hi[3])
{
  box.SetDimensions( lo, hi, VTK_XY_PLANE );
}

//------------------------------------------------------------------------------
void Construct3DAMRBox(
    vtkAMRBox& box,int lo[3],int hi[3])
{
  box.SetDimensions( lo, hi , VTK_XYZ_GRID);
}

//------------------------------------------------------------------------------
int TestAMRBoxEquality()
{
  int rc = 0;

  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A, B, C, A2D;
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox( A, lo, hi );
  Construct3DAMRBox( B, lo, hi );
  Construct2DAMRBox( A2D, lo, hi );

  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 16;
  hi[0] = hi[1] = hi[2] = 32;
  Construct3DAMRBox( C, lo, hi );

  if( A != B )
  {
    rc++;
  }

  if( A == A2D )
  {
    rc++;
  }

  if( A == C  )
  {
    rc++;
  }

  return( rc );
}

//------------------------------------------------------------------------------
int TestAMRBoxAssignmentOperator()
{
  int rc = 0;
  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A, B;
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;

  Construct3DAMRBox( A, lo, hi );
  B = A;
  if( A != B )
  {
    rc++;
  }
  return( rc );
}

//------------------------------------------------------------------------------
int TestAMRBoxCoarsenRefineOperators()
{
  int rc = 0;
  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A, A0, Ar;

  // Here is the initial AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox( A, lo, hi);

  // Here is the refined AMR box
  h[0]  = h[1]  = h[2]  = 0.5;
  lo[0] = lo[1] = lo[2] = 16;
  hi[0] = hi[1] = hi[2] = 33;
  Construct3DAMRBox( Ar, lo, hi );

  // Save the intial AMR box to A0
  A0 = A;

  // Refine the AMR box to Ar
  A.Refine( 2 );
  if( A != Ar )
  {
    std::cerr << "Here is A: ";
    A.Print( std::cerr ) << "\n";

    std::cerr << "Here is Ar: ";
    Ar.Print( std::cerr ) << "\n";

    std::cerr << "ERROR: refining AMR box failed!\n";
    rc++;
  }

  // Coarsen AMR box to A0
  A.Coarsen( 2 );
  if( A != A0 )
  {
    std::cerr << "ERROR: coarsening AMR box failed!\n";
    rc++;
  }
  return( rc );
}

//------------------------------------------------------------------------------
int TestAMRBoxShiftOperator()
{
  int rc = 0;

  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A, A0, Ashifted;

  // Here is the initial AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox( A, lo, hi);

  A0 = A;

  int shift[3];
  shift[0] = shift[1] = shift[2] = 3;

  // Here is the shifted AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 11;
  hi[0] = hi[1] = hi[2] = 19;
  Construct3DAMRBox( Ashifted, lo, hi);

  A.Shift( shift );
  if( A != Ashifted )
  {
    std::cerr << "ERROR: shifting AMR box failed!\n";
    rc++;
  }

  // Reverse shift orientation
  for( int i=0; i < 3; i++)
  {
    shift[i] = shift[i]*(-1);
  }
  A.Shift( shift );
  if( A != A0 )
  {
    std::cerr << "ERROR: shifting AMR box failed!\n";
    rc++;
  }
  return( rc );
}

//------------------------------------------------------------------------------
int TestAMRBoxGrowShrinkOperators()
{
  int rc = 0;

  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A, A0, Agrown;

  // Here is the initial AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox( A, lo, hi);

  A0 = A;

  // Here is the initial AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 6;
  hi[0] = hi[1] = hi[2] = 18;
  Construct3DAMRBox( Agrown,  lo, hi);

  A.Grow( 2 );
  if( A != Agrown )
  {
    std::cerr << "ERROR: growing AMR box failed!\n";
    rc ++;
  }

  A.Shrink( 2 );
  if( A != A0 )
  {
    std::cerr << "ERROR: shrinking AMR box failed!\n";
    rc ++;
  }
  return( rc );
}

//------------------------------------------------------------------------------
int TestAMRBoxIntersection()
{
  int rc = 0;

  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A0, A, B, I;

  // Here is the initial AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox( A, lo, hi);

  // Save the initial
  A0 = A;

  B = A;
  B.Shrink( 2 );
  bool doesIntersect = A.Intersect( B );
  if( !doesIntersect || (A != B) )
  {
    std::cerr << "ERROR: Intersecting a fully encompassing box failed!\n";
    rc++;
  }

  A = A0;
  B = A;
  B.Shift( 2,2,2 );

  // Here is the expected box after intersecting
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 10;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox(I, lo, hi );

  doesIntersect = A.Intersect( B );
  if( !doesIntersect || (A != I) )
  {
    std::cerr << "ERROR: Intersecting a partially overlapping box failed!\n";
    rc++;
  }

  A = A0;
  B = A;
  B.Shift(10,10,10);
  doesIntersect = A.Intersect( B );
  if( doesIntersect )
  {
    std::cerr << "ERROR: Intersecting a non-overlapping box failed!\n";
    rc++;
  }
  return( rc );
}

//------------------------------------------------------------------------------
int TestAMRBoxSerialization()
{
  int rc = 0;
  double h[3];
  int    lo[3];
  int    hi[3];

  vtkAMRBox A;

  // Here is the initial AMR box
  h[0]  = h[1]  = h[2]  = 1.0;
  lo[0] = lo[1] = lo[2] = 8;
  hi[0] = hi[1] = hi[2] = 16;
  Construct3DAMRBox( A, lo, hi);

  vtkIdType bytesize    = 0;
  unsigned char *buffer = NULL;
  A.Serialize( buffer, bytesize );
  if( (buffer == NULL) || (bytesize == 0) )
  {
    std::cerr << "ERROR: Serializing AMR box failed!\n";
    ++rc;
  }

  vtkIdType expectedByteSize = vtkAMRBox::GetBytesize();
  if( bytesize != expectedByteSize )
  {
    std::cerr << "ERROR: Bytesize of buffer did not match expected size!\n";
    ++rc;
  }

  vtkAMRBox B;
  B.Deserialize( buffer, bytesize );

  if( A != B )
  {
    std::cerr << "ERROR: Deserialization of AMR box failed!\n";
    rc++;
  }

  delete [] buffer;

  return( rc );
}

//------------------------------------------------------------------------------
void CheckTestStatus( int rc, const std::string &TestName )
{
  std::cout << "Test " << TestName << "...";
  std::cout.flush();
  if( rc == 0 )
  {
    std::cout << "PASSED!\n";
    std::cout.flush();
  }
  else
  {
    std::cout << "FAILED!\n";
    std::cout.flush();
  }
}

#include <cassert>
//------------------------------------------------------------------------------
int TestAMRBox(int , char *[])
{
  int rc = 0;

  rc += TestAMRBoxEquality();
  CheckTestStatus(rc, "TestAMRBoxEquality");

  rc += TestAMRBoxAssignmentOperator();
  CheckTestStatus(rc, "TestAMRBoxAssignmentOperator");

  rc += TestAMRBoxCoarsenRefineOperators();
  CheckTestStatus(rc, "TestAMRBoxCoarsenRefineOperators");

  rc += TestAMRBoxShiftOperator();
  CheckTestStatus(rc, "TestAMRBoxShiftOperator");

  rc += TestAMRBoxGrowShrinkOperators();
  CheckTestStatus(rc, "TestAMRBoxGrowShrinkOperators");

  rc += TestAMRBoxIntersection();
  CheckTestStatus(rc, "TestAMRBoxIntersection");

  rc += TestAMRBoxSerialization();
  CheckTestStatus(rc, "TestAMRBoxSerialization");

  return( rc );
}




