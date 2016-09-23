/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataToStructuredGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkImageToStructuredGrid.h"

#include <cmath>
#include <limits>
#include <string>
#include <cstring>

// Description:
// Performs safe division a/b which also checks for underflow & overflow
double SafeDivision( const double a, const double b )
{
  // Catch overflow
  if( (b < 1) && (a > ( b*std::numeric_limits<double>::max() ) ) )
  {
    return std::numeric_limits<double>::max();
  }

  // Catch underflow
  if( (a == static_cast<double>(0.0)) ||
      ( (b > 1) && (a < b*std::numeric_limits<double>::max() ) ) )
  {
    return( static_cast<double>( 0.0 ) );
  }

  return( a/b );
}

// Description:
// Checks if two given floating numbers are equivalent.
// This algorithm is based on Knuth, The of Computer Programming (vol II).
bool FloatNumberEquals( double a, double b, double TOL )
{
  double adiff = std::abs( a-b );
  double d1    = SafeDivision( adiff,std::abs(a) );
  double d2    = SafeDivision( adiff,std::abs(b) );
  if( (d1 <= TOL) || (d2 <= TOL) )
  {
    return true;
  }
  return false;
}

// Description:
// Checks if the given two points a & b are equal
bool ArePointsEqual( double a[3], double b[3], double TOL=1e-9 )
{
  for( int i=0; i < 3; ++i )
  {
     if( !FloatNumberEquals( a[i], b[i], TOL ) )
     {
       return false;
     }
  }
  return true;
}

// Description:
// Constructs a uniform grid instance with the given spacing & dimensions
// at the user-supplied origin.
vtkUniformGrid* GetGrid( double *origin, double *spacing, int *ndim )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin( origin );
  grd->SetSpacing( spacing );
  grd->SetDimensions( ndim );

  vtkDoubleArray* pntData = vtkDoubleArray::New();
  pntData->SetName( "XYZ-NODE" );
  pntData->SetNumberOfComponents( 1 );
  pntData->SetNumberOfTuples( grd->GetNumberOfPoints() );

  double node[3];
  for( int pntIdx=0; pntIdx < grd->GetNumberOfPoints(); ++pntIdx )
  {
    grd->GetPoint( pntIdx, node );
    pntData->SetValue(pntIdx, (node[0]+node[1]+node[2]) );
  } // END for all points
  grd->GetPointData()->AddArray( pntData );
  pntData->Delete();


  vtkDoubleArray* xyz = vtkDoubleArray::New( );
  xyz->SetName( "XYZ-CELL" );
  xyz->SetNumberOfComponents( 1 );
  xyz->SetNumberOfTuples( grd->GetNumberOfCells() );

  for( int cellIdx=0; cellIdx < grd->GetNumberOfCells(); ++cellIdx )
  {
    vtkCell* myCell = grd->GetCell( cellIdx);

    vtkPoints *cellPoints = myCell->GetPoints();

    double xyzCenter[3];
    xyzCenter[0] = 0.0;
    xyzCenter[1] = 0.0;
    xyzCenter[2] = 0.0;

    for( int cp=0; cp < cellPoints->GetNumberOfPoints(); ++cp )
    {
      double pnt[3];
      cellPoints->GetPoint( cp, pnt );
      xyzCenter[0] += pnt[0];
      xyzCenter[1] += pnt[1];
      xyzCenter[2] += pnt[2];
    } // END for all cell points

    xyzCenter[0] = xyzCenter[0] / (cellPoints->GetNumberOfPoints());
    xyzCenter[1] = xyzCenter[1] / (cellPoints->GetNumberOfPoints());
    xyzCenter[2] = xyzCenter[2] / (cellPoints->GetNumberOfPoints());

    double f = xyzCenter[0]*xyzCenter[0] +
        xyzCenter[1]*xyzCenter[1] + xyzCenter[2]*xyzCenter[2];
    xyz->SetTuple1(cellIdx,f);
  } // END for all cells

  grd->GetCellData()->AddArray(xyz);
  xyz->Delete();
  return grd;
}

// Description
// Checks if the given image data-set is equivalent to the structured grid
// data-set.
bool DataSetsEqual( vtkImageData* img, vtkStructuredGrid* sg )
{
  // 0. Check dimensions
  int imgdim[3]; int sgdim[3];
  img->GetDimensions( imgdim );
  sg->GetDimensions( sgdim );
  for( int i=0; i < 3; ++i )
  {
    if( imgdim[i] != sgdim[i] )
    {
      return false;
    }
  }

  // 1. Check Number of elements
  if( img->GetNumberOfCells() != sg->GetNumberOfCells() )
  {
    return false;
  }

  // 2. Check Number of points
  if( img->GetNumberOfPoints() != sg->GetNumberOfPoints() )
  {
    return false;
  }

  // 3. Check Point equality
  double pnt1[3]; double pnt2[3];
  for( int pntIdx=0; pntIdx < img->GetNumberOfPoints(); ++pntIdx )
  {
    img->GetPoint( pntIdx, pnt1 );
    sg->GetPoint( pntIdx,pnt2 );
    if( !ArePointsEqual( pnt1, pnt2 ) )
    {
      return false;
    }
  }

  // 4. Check Point data equality
  if( img->GetPointData()->GetNumberOfArrays() !=
      sg->GetPointData()->GetNumberOfArrays() )
  {
      return false;
  }
  int dataArrayIdx = 0;
  for(;dataArrayIdx<img->GetPointData()->GetNumberOfArrays();++dataArrayIdx )
  {
    vtkDataArray* array1 = img->GetPointData()->GetArray( dataArrayIdx );
    vtkDataArray* array2 = sg->GetPointData()->GetArray( dataArrayIdx );
    if( array1->GetNumberOfComponents() != array2->GetNumberOfComponents() )
    {
      return false;
    }
    if( array1->GetNumberOfTuples() != array2->GetNumberOfTuples() )
    {
      return false;
    }
    if( std::strcmp( array1->GetName(), array2->GetName() ) != 0 )
    {
      return false;
    }
  } // END for all point arrays

  // 5. Check Cell data equality
  if( img->GetCellData()->GetNumberOfArrays() !=
      sg->GetCellData()->GetNumberOfArrays() )
  {
      return false;
  }

  dataArrayIdx = 0;
  for(;dataArrayIdx<img->GetCellData()->GetNumberOfArrays();++dataArrayIdx)
  {
    vtkDataArray* array1 = img->GetCellData()->GetArray( dataArrayIdx );
    vtkDataArray* array2 = sg->GetCellData()->GetArray( dataArrayIdx );
    if( array1->GetNumberOfComponents() != array2->GetNumberOfComponents() )
    {
      return false;
    }
    if( array1->GetNumberOfTuples() != array2->GetNumberOfTuples() )
    {
      return false;
    }
    if( std::strcmp( array1->GetName(), array2->GetName() ) != 0 )
    {
      return false;
    }
  } // END for all cell arrays

  return true;
}

int TestImageDataToStructuredGrid(int,char *[])
{
  int rval = 0;

  double origin[3]     = {0.0,0.0,0.0};
  double spacing[3]    = {0.5,0.2,0.0};
  int    ndim[3]       = {10, 10, 1};
  vtkUniformGrid* img1 = GetGrid( origin, spacing, ndim );

  vtkImageToStructuredGrid* myFilter = vtkImageToStructuredGrid::New( );
  myFilter->SetInputData( img1 );
  img1->Delete();
  myFilter->Update();
  vtkStructuredGrid* sg1 = myFilter->GetOutput();

  if( !DataSetsEqual( img1, sg1 ) )
  {
    rval = 1;
  }

  myFilter->Delete();
  return( rval );
}
