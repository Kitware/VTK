/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveSphereDirectionEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRecursiveSphereDirectionEncoder.h"
#include "vtkObjectFactory.h"

#include <cmath>

vtkStandardNewMacro(vtkRecursiveSphereDirectionEncoder);

// Construct the object. Initialize the index table which will be
// used to map the normal into a patch on the recursively subdivided
// sphere.
vtkRecursiveSphereDirectionEncoder::vtkRecursiveSphereDirectionEncoder()
{
  this->RecursionDepth = 6;
  this->IndexTable = NULL;
  this->DecodedNormal = NULL;
  this->InitializeIndexTable();
}

// Destruct a vtkRecursiveSphereDirectionEncoder - free up any memory used
vtkRecursiveSphereDirectionEncoder::~vtkRecursiveSphereDirectionEncoder()
{
  delete [] this->IndexTable;
  delete [] this->DecodedNormal;
}


int vtkRecursiveSphereDirectionEncoder::GetEncodedDirection( float n[3] )
{
  float t;
  int   value;
  int   xindex, yindex;
  float x, y;

  if ( this->IndexTableRecursionDepth != this->RecursionDepth )
  {
    this->InitializeIndexTable();
  }

  // Convert the gradient direction into an encoded index value
  // This is done by computing the (x,y) grid position of this
  // normal in the 2*NORM_SQR_SIZE - 1 grid, then passing this
  // through the IndexTable to look up the 16 bit index value

  // Don't use fabs because it is slow - just convert to absolute
  // using a simple conditional.
  t =
    ((n[0]>=0.0)?(n[0]):(-n[0])) +
    ((n[1]>=0.0)?(n[1]):(-n[1])) +
    ((n[2]>=0.0)?(n[2]):(-n[2]));

  if ( t )
  {

    t = 1.0 / t;

    x = n[0] * t;
    y = n[1] * t;

    xindex = (int)((x+1.0)*(float)(this->InnerSize) + 0.5);
    yindex = (int)((y+1.0)*(float)(this->InnerSize) + 0.5);

    if ( xindex > 2*this->InnerSize )
    {
      xindex = 2*this->InnerSize;
    }
    if ( yindex > 2*this->InnerSize )
    {
      yindex = 2*this->InnerSize;
    }

    value = this->IndexTable[xindex*(this->OuterSize+this->InnerSize) + yindex];

    // If the z component is less than 0.0, add this->GridSize to the
    // index
    if ( n[2] < 0.0 )
    {
      value += this->GridSize;
    }
  }
  else
  {
    value = 2*this->GridSize;
  }

  return value;
}

float *vtkRecursiveSphereDirectionEncoder::GetDecodedGradient( int value )
{
  if ( this->IndexTableRecursionDepth != this->RecursionDepth )
  {
    this->InitializeIndexTable();
  }

  return (this->DecodedNormal + value*3);
}

int vtkRecursiveSphereDirectionEncoder::GetNumberOfEncodedDirections( void )
{
  int     outer_size, inner_size;
  int     norm_size;

  outer_size = (int)(pow( 2.0, (double) this->RecursionDepth ) + 1);
  inner_size = outer_size - 1;

  norm_size = outer_size * outer_size + inner_size * inner_size;

  return (norm_size*2 + 1);
}

float *vtkRecursiveSphereDirectionEncoder::GetDecodedGradientTable( void )
{
  if ( this->IndexTableRecursionDepth != this->RecursionDepth )
  {
    this->InitializeIndexTable();
  }

  return this->DecodedNormal;
}

// Initialize the index table.  This is a 2*NORM_SQR_SIZE - 1 by
// 2*NORM_SQR_SIZE - 1 entry table that maps (x,y) grid position to
// encoded normal index.  The grid position is obtained by starting
// with an octahedron (comprised of 8 triangles forming a double
// pyramid). Each triangle is then replaced by 4 triangles by joining
// edge midpoints.  This is done recursively until NORM_SQR_SIZE
// vertices exist on each original edge. If you "squish" this octahedron,
// it will look like a diamond.  Then rotate it 45 degrees, it will
// look like a square.  Then look at the pattern of vertices - there
// is a NORM_SQR_SIZE by NORM_SQR_SIZE grid, with a (NORM_SQR_SIZE-1) by
// NORM_SQR_SIZE - 1 grid inside of it.  The vertices all fall on
// (x,y) locatiions in a grid that is 2*NORM_SQR_SIZE - 1 by
// 2*NORM_SQR_SIZE - 1, although not every (x,y) location has a vertex.
void vtkRecursiveSphereDirectionEncoder::InitializeIndexTable( void )
{
  int     i, j, index, max_index;
  int     xindex, yindex;
  float   x, y, z, tmp_x, tmp_y;
  float   norm;
  int     limit;

  // Free up any memory previously used
  delete [] this->IndexTable;
  delete [] this->DecodedNormal;

  this->OuterSize = (int)(pow( 2.0, (double) this->RecursionDepth ) + 1);
  this->InnerSize = this->OuterSize - 1;
  this->GridSize =
    this->OuterSize * this->OuterSize +
    this->InnerSize * this->InnerSize;


  // Create space for the tables
  this->IndexTable = new int [(this->OuterSize + this->InnerSize) *
                              (this->OuterSize + this->InnerSize)];

  // Initialize the table to -1 -- we'll use this later to determine which
  // entries are still not filled in
  for ( i = 0; i < ( (this->OuterSize + this->InnerSize) *
                     (this->OuterSize + this->InnerSize) ); i ++ )
  {
      this->IndexTable[i] = -1;
  }

  this->DecodedNormal =
    new float [ 3 * ( 1 +
                      2 * this->OuterSize * this->OuterSize +
                      2 * this->InnerSize * this->InnerSize ) ];

  // Initialize the index
  index = 0;

  // max_index indicates the largest index we will get - the number
  // of vertices in the two-grid square.  This represents half the
  // normals, and max_index is used to offset from one half into the
  // other.  One half of the normals have z components >= 0, and the
  // second half (all with indices above max_index) have z components
  // that are <= 0.
  max_index =  this->GridSize;

  // The last normal (max_index*2) is the zero normal
  this->DecodedNormal[3*(max_index*2) + 0] = 0.0;
  this->DecodedNormal[3*(max_index*2) + 1] = 0.0;
  this->DecodedNormal[3*(max_index*2) + 2] = 0.0;

  // The outer loop is for this->OuterSize + this->InnerSize rows
  for ( i = 0; i < this->OuterSize + this->InnerSize; i++ )
  {
    // Compute the y component for this row
    tmp_y = (float)(2*i)/(float)(this->InnerSize*2) - 1.0;

    // On the odd rows, we are doing the small grid which has
    // this->InnerSize elements in it
    limit = ( i%2 )?(this->InnerSize):(this->OuterSize);

    for ( j = 0; j < limit; j++ )
    {
      // compute the x component for this column
      if ( i%2 )
      {
        tmp_x = (float)(2*j)/(float)(this->InnerSize) -
          1.0 + (1.0/(float)(this->InnerSize));
      }
      else
      {
        tmp_x = (float)(2*j)/(float)(this->InnerSize) - 1.0;
      }

      // rotate by 45 degrees
      // This rotation intentionally does not preserve length -
      // we could have tmp_x = 1.0 and tmp_y = 1.0, we want this
      // to lie within [-1.0,1.0] after rotation.
      x = 0.5 * tmp_x - 0.5 * tmp_y;
      y = 0.5 * tmp_x + 0.5 * tmp_y;

      // compute the z based on the x and y values
      if ( x >= 0 && y >= 0 )
      {
        z = 1.0 - x - y;
      }
      else if ( x >= 0 && y < 0 )
      {
        z = 1.0 - x + y;
      }
      else if ( x < 0 && y < 0 )
      {
        z = 1.0 + x + y;
      }
      else
      {
        z = 1.0 + x - y;
      }

      // Normalize this direction and set the DecodedNormal table for
      // this index to this normal.  Also set the corresponding
      // entry for this normal with a negative z component
      norm = sqrt( (double)( x*x + y*y + z*z ) );
      this->DecodedNormal[3*index + 0] = x / norm;
      this->DecodedNormal[3*index + 1] = y / norm;
      this->DecodedNormal[3*index + 2] = z / norm;
      this->DecodedNormal[3*(index+max_index) + 0] =   x / norm;
      this->DecodedNormal[3*(index+max_index) + 1] =   y / norm;
      this->DecodedNormal[3*(index+max_index) + 2] = -(z / norm);

      // Figure out the location in the IndexTable. Be careful with
      // boundary conditions.
      xindex = (int)((x+1.0)*(float)(this->InnerSize) + 0.5);
      yindex = (int)((y+1.0)*(float)(this->InnerSize) + 0.5);
      if ( xindex > 2*this->InnerSize )
      {
        xindex = 2*this->InnerSize;
      }
      if ( yindex > 2*this->InnerSize )
      {
        yindex = 2*this->InnerSize;
      }
      this->IndexTable[xindex*(this->OuterSize+this->InnerSize) + yindex] = index;

      // Do the grid location to the left - unless we are at the left
      // border of the grid. We are computing indices only for the
      // actual vertices of the subdivided octahedron, but we'll
      // convert these into the IndexTable coordinates and fill in
      // the index for the intermediate points on the grid as well.
      // This way we can't get bitten by a scan-conversion problem
      // where we skip over some table index due to precision, and
      // therefore it doesn't have a valid value in it.
      if ( tmp_x > 0 )
      {
        x = 0.5 * (tmp_x - (1.0/(float)this->InnerSize)) - 0.5 * tmp_y;
        y = 0.5 * (tmp_x - (1.0/(float)this->InnerSize)) + 0.5 * tmp_y;
        xindex = (int)((x+1.0)*(float)(this->InnerSize) + 0.5);
        yindex = (int)((y+1.0)*(float)(this->InnerSize) + 0.5);
        if ( xindex > 2*this->InnerSize )
        {
          xindex = 2*this->InnerSize;
        }
        if ( yindex > 2*this->InnerSize )
        {
          yindex = 2*this->InnerSize;
        }
        this->IndexTable[xindex*(this->OuterSize+this->InnerSize) + yindex] = index;
      }

      // On the odd rows we also need to do the last grid location on
      // the right.
      if ( (i%2) && (j == limit - 1) )
      {
        x = 0.5 * (tmp_x + (1.0/(float)this->InnerSize)) - 0.5 * tmp_y;
        y = 0.5 * (tmp_x + (1.0/(float)this->InnerSize)) + 0.5 * tmp_y;
        xindex = (int)((x+1.0)*(float)(this->InnerSize) + 0.5);
        yindex = (int)((y+1.0)*(float)(this->InnerSize) + 0.5);
        if ( xindex > 2*this->InnerSize )
        {
          xindex = 2*this->InnerSize;
        }
        if ( yindex > 2*this->InnerSize )
        {
          yindex = 2*this->InnerSize;
        }
        this->IndexTable[xindex*(this->OuterSize+this->InnerSize) + yindex] = index;
      }

      // Increment the index
      index++;
    }
  }


  // The index table has been initialized for the current recursion
  // depth
  this->IndexTableRecursionDepth = this->RecursionDepth;


  // Spread the first index value in each row to the left, and the last to the right.
  // This is because we have only filled in a diamond of index values within the square
  // grid, and we need to be careful at the edges due to precision problems. This way
  // we won't be able to access a table location that does not have a valid index in it.
  for ( j = 0; j < this->OuterSize + this->InnerSize; j++ )
  {
    // Start from the middle going right, copy the value from the left if
    // this entry is not initialized
    for ( i = (this->OuterSize+this->InnerSize)/2;
          i < this->OuterSize + this->InnerSize; i++ )
    {
      if ( this->IndexTable[j*(this->OuterSize+this->InnerSize)+i] == -1 )
      {
        this->IndexTable[j*(this->OuterSize+this->InnerSize)+i] =
          this->IndexTable[j*(this->OuterSize+this->InnerSize)+i-1];
      }
    }

    // Start from the middle going left, copy the value from the right if
    // this entry is not initialized
    for ( i = (this->OuterSize+this->InnerSize)/2; i >= 0; i-- )
    {
      if ( this->IndexTable[j*(this->OuterSize+this->InnerSize)+i] == -1 )
      {
        this->IndexTable[j*(this->OuterSize+this->InnerSize)+i] =
          this->IndexTable[j*(this->OuterSize+this->InnerSize)+i+1];
      }
    }
  }
}


// Print the vtkRecursiveSphereDirectionEncoder
void vtkRecursiveSphereDirectionEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of encoded directions: " <<
    this->GetNumberOfEncodedDirections() << endl;

  os << indent << "Recursion depth: " << this->RecursionDepth << endl;
}
