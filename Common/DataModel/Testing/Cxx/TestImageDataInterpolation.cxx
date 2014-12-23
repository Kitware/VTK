/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestImageDataInterpolation.cxx -- Test interpolation from image data
//
// .SECTION Description
//  This test applies a function, F(x,y,z), to the image data and tests the
//  interpolation.

#include "vtkDebugLeaks.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkPointData.h"
#include "vtkCell.h"

#include <cassert>
#include <cmath>
#include <limits>

// Description:
// Performs safe division a/b which also checks for underflow & overflow
double SafeDiv( const double a, const double b )
{
  // Catch overflow
  if( (b < 1) && (a > ( b*std::numeric_limits<double>::max() ) ) )
    return std::numeric_limits<double>::max();

  // Catch underflow
  if( (a == static_cast<double>(0.0)) ||
      ( (b > 1) && (a < b*std::numeric_limits<double>::max() ) ) )
      return( static_cast<double>( 0.0 ) );

  return( a/b );
}

// Description:
// Checks if two given floating numbers are equivalent.
// This algorithm is based on Knuth, The of Computer Programming (vol II).
bool eq( double a, double b, double TOL=1e-9 )
{
  double adiff = std::abs( a-b );
  double d1    = SafeDiv( adiff,std::abs(a) );
  double d2    = SafeDiv( adiff,std::abs(b) );
  if( (d1 <= TOL) || (d2 <= TOL) )
    return true;
  return false;
}


// Description:
// Applies the function to the point with the given coordinates.
// The function is defined as F(x,y,z)= x + y + z
inline double F( const double x, const double y, const double z )
{
  return( (x+y+z) );
}

// Description:
// Returns the grid with the specified extent, origin and spacing.
inline vtkImageData* GetGrid( int dims[3], double origin[3], double h[3] )
{
  vtkImageData *image = vtkImageData::New();
  assert( "pre: image data is NULL!" && (image != NULL) );

  image->SetDimensions( dims );
  image->SetOrigin( origin );
  image->SetSpacing( h );

  vtkPointData *PD = image->GetPointData();
  assert( "pre: point-data is NULL" && (PD != NULL) );

  vtkDoubleArray *dataArray = vtkDoubleArray::New();
  dataArray->SetName( "Fx" );
  dataArray->SetNumberOfTuples( image->GetNumberOfPoints() );
  dataArray->SetNumberOfComponents( 1 );

  vtkIdType idx = 0;
  for( ; idx < image->GetNumberOfPoints(); ++idx )
    {
    double pnt[3];
    image->GetPoint(idx, pnt);
    dataArray->SetComponent( idx, 0, F(pnt[0],pnt[1],pnt[2]) );
    } // END for all points

  PD->AddArray( dataArray );
  dataArray->Delete();

  return( image );
}

// Description:
// Given the image data this method returns a list of test points for
// interpolation at each cell center.
inline vtkPoints* GetReceivePoints(
    vtkImageData *img, vtkIdList* donorCellList)
{
  vtkPoints *rcvPoints = vtkPoints::New();
  rcvPoints->SetNumberOfPoints( img->GetNumberOfCells() );
  donorCellList->SetNumberOfIds( img->GetNumberOfCells() );

  vtkIdType cellIdx = 0;
  for( ; cellIdx < img->GetNumberOfCells(); ++cellIdx  )
    {
    // Get cell
    vtkCell *myCell = img->GetCell( cellIdx  );
    assert( "pre: myCell != NULL" && (myCell != NULL) );

    // Compute center
    double c[3];
    double pCenter[3];
    double *weights = new double[ myCell->GetNumberOfPoints() ];
    int subId       = myCell->GetParametricCenter( pCenter );
    myCell->EvaluateLocation( subId,pCenter,c,weights );
    delete [] weights;

    // Insert center to point list
    donorCellList->SetId( cellIdx, cellIdx );
    rcvPoints->SetPoint( cellIdx, c );
    } // END for all cells

  return( rcvPoints );
}

// Description:
// Given the mesh data, the donor cell and the interpolation weights, this
// method returns the interpolated value at the corresponding point location.
inline double InterpolateValue(vtkImageData *img, vtkCell *c, double* w)
{
  assert( "pre: image is NULL!" && (img != NULL) );
  assert( "pre: donor cell is NULL" && (c != NULL) );
  assert( "pre: weights is NULL" && (w != NULL) );

  vtkPointData *PD = img->GetPointData();
  assert( "pre: point data is NULL" && (PD != NULL) );
  vtkDataArray *dataArray = PD->GetArray( "Fx" );
  assert( "pre: data array is NULL" && (dataArray != NULL) );

  std::cout << "W:[";
  std::cout.flush();
  double val         = 0.0;
  vtkIdType numNodes = c->GetNumberOfPoints();
  for( vtkIdType nodeIdx=0; nodeIdx < numNodes; ++nodeIdx )
    {
    std::cout << w[ nodeIdx ] << " ";
    std::cout.flush();
    vtkIdType meshNodeIdx = c->GetPointId( nodeIdx );
    val += w[nodeIdx]*dataArray->GetComponent( meshNodeIdx, 0 );
    } // END for all cells nodes
  std::cout << "]\n";
  std::cout.flush();
  return( val );
}

// Description:
// Main test routine for testing the interpolation
inline int TestInterpolation( int dims[3], double origin[3], double h[3] )
{
  vtkImageData *grid = GetGrid( dims, origin, h );
  assert( "pre: grid is NULL" && (grid != NULL) );

  std::cout << "NUMBER OF CELLS:  " << grid->GetNumberOfCells() << std::endl;
  std::cout.flush();

  vtkIdList *donors = vtkIdList::New();
  vtkPoints *pnts = GetReceivePoints( grid, donors );

  int subId = 0;
  double pcoords[3];
  double weights[8];
  double x[3];

  int interpErrors = 0;

  vtkIdType idx = 0;
  for( ; idx < pnts->GetNumberOfPoints(); ++idx )
    {
    pnts->GetPoint( idx, x );
    vtkIdType cellIdx =
     grid->FindCell(x,NULL,0,0.0,subId,pcoords,weights);

    if( cellIdx < 0 )
      {
      std::cerr << "point (" << x[0] << ", " << x[1] << ", " << x[2];
      std::cerr << ") is out-of-bounds!\n";
      return 1;
      }

    vtkCell *donorCell = grid->GetCell( cellIdx );
    assert( "pre: donor cell is NULL" && (donorCell != NULL));

    std::cout << "N:  [" << pcoords[0] << " ";
    std::cout << pcoords[1] << " " << pcoords[2] << "]\n";
    std::cout.flush();

    double f         = InterpolateValue( grid, donorCell, weights );
    double fExpected = F( x[0],x[1],x[2] );

    if( !eq( f, fExpected ) )
      {
      std::cout << "INTERPOLATION ERROR: ";
      std::cout << "f_expeted=" << fExpected << " ";
      std::cout << "f_interp=" << f << std::endl;
      std::cout.flush();
      ++ interpErrors;
      }

    } // END for all points

  grid->Delete();
  donors->Delete();
  pnts->Delete();

  return interpErrors;
}

// Description:
// Test main function
int TestImageDataInterpolation(int, char *[])
{
  static int dims[4][3]={
      {3,3,1}, // XY Plane
      {3,1,3}, // XZ Plane
      {1,3,3}, // YZ Plane
      {3,3,3}  // XYZ Volume
  };

  static const char* TestName[4]={
      "XY-Plane",
      "XZ-Plane",
      "YZ-Plane",
      "XYZ-Grid"
  };

  double origin[3];
  origin[0] = origin[1] = origin[2] = 0.0;

  double h[3];
  h[0] = h[1] = h[2] = 0.2;

  int testStatus = 0;
  for( int i=0; i < 4; ++i )
    {
    std::cout << "Testing " << TestName[ i ] << "...\n";
    std::cout.flush();
    int rc = 0;
    rc     = TestInterpolation( dims[i], origin, h );
    testStatus += rc;
    std::cout << "[DONE]\n";
    std::cout.flush();

    if( rc != 0 )
      {
      std::cout << rc << " failures detected!\n";
      std::cout << "TEST FAILED!\n\n";
      std::cout.flush();
      }
    else
      {
      std::cout << "TEST PASSED!\n";
      std::cout << std::endl;
      }
    } // END for all tests

  return( testStatus );

}
