/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveSphereDirectionEncoder.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkRecursiveSphereDirectionEncoder.h"


// Description:
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

// Description:
// Destruct a vtkRecursiveSphereDirectionEncoder - free up any memory used
vtkRecursiveSphereDirectionEncoder::~vtkRecursiveSphereDirectionEncoder()
{
  if ( this->IndexTable )
    {
    delete [] this->IndexTable;
    }
  if ( this->DecodedNormal )
    {
    delete [] this->DecodedNormal;
    }
}


int vtkRecursiveSphereDirectionEncoder::GetEncodedDirection( float n[3] )
{
  float t;
  int   value;
  int   norm_size;
  int   xindex, yindex;
  int   outer_size, inner_size;

  if ( this->IndexTableRecursionDepth != this->RecursionDepth )
    {
    this->InitializeIndexTable();
    }

  outer_size = pow( 2.0, (double) this->RecursionDepth ) + 1;
  inner_size = outer_size - 1;

  norm_size = outer_size * outer_size + inner_size * inner_size;

  // Convert the gradient direction into an encoded index value
  // This is done by computing the (x,y) grid position of this 
  // normal in the 2*NORM_SQR_SIZE - 1 grid, then passing this
  // through the IndexTable to look up the 16 bit index value
  if (  fabs((double)n[0]) + fabs((double)n[1]) + fabs((double)n[2]) )
    {
    t = 1.0 / ( fabs((double)n[2]) + fabs((double)n[0]) + 
		fabs((double)n[1]) );
    
    n[0] *= t;
    n[1] *= t;

    xindex =(int)((float)(n[0]+1 + 1.0 / (float)(2*(inner_size))) * 
	     (float)(inner_size)); 
    yindex = (int)((float)(n[1]+1 + 1.0 / (float)(2*(inner_size))) * 
	     (float)(inner_size));

    value = this->IndexTable[xindex*(outer_size+inner_size) + yindex];
    
    // If the z component is less than 0.0, add norm_size to the
    // index 
    if ( n[2] < 0.0 ) value += norm_size;
    }
  else
    {
    value = 2*norm_size;
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

  outer_size = pow( 2.0, (double) this->RecursionDepth ) + 1;
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

// Description:
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
  int     outer_size, inner_size;

  // Free up any memory previously used
  if ( this->IndexTable )
    {
    delete [] this->IndexTable;
    }
  if ( this->DecodedNormal )
    {
    delete [] this->DecodedNormal;
    }

  outer_size = pow( 2.0, (double) this->RecursionDepth ) + 1;
  inner_size = outer_size - 1;

  // Create space for the tables
  this->IndexTable = new int [(outer_size + inner_size) * 
			      (outer_size + inner_size)];
  this->DecodedNormal = 
    new float [ 3 * ( 1 + 
		      2 * outer_size * outer_size +
		      2 * inner_size * inner_size ) ];

  // Initialize the index
  index = 0;

  // max_index indicates the largest index we will get - the number
  // of vertices in the two-grid square.  This represents half the
  // normals, and max_index is used to offset from one half into the
  // other.  One half of the normals have z components >= 0, and the
  // second half (all with indices above max_index) have z components
  // that are <= 0.
  max_index =  outer_size * outer_size + inner_size * inner_size;

  // The last normal (max_index*2) is the zero normal
  this->DecodedNormal[3*(max_index*2) + 0] = 0.0;
  this->DecodedNormal[3*(max_index*2) + 1] = 0.0;
  this->DecodedNormal[3*(max_index*2) + 2] = 0.0;

  // The outer loop is for outer_size + inner_size rows
  for ( i = 0; i < outer_size + inner_size; i++ )
    {
    // Compute the y component for this row
    tmp_y = (float)(2*i)/(float)(inner_size*2) - 1.0;

    // On the odd rows, we are doing the small grid which has
    // inner_size elements in it
    if ( i%2 )
      {
      for ( j = 0; j < inner_size; j++ )
	{
	// compute the x component for this column
        tmp_x = (float)(2*j)/(float)(inner_size) - 
	  1.0 + (1.0/(float)(inner_size));

	// rotate by 45 degrees
        x = 0.5 * tmp_x - 0.5 * tmp_y;
        y = 0.5 * tmp_x + 0.5 * tmp_y;

	// compute the z based on the x and y values
        if ( x >= 0 && y >= 0 )
          z = 1.0 - x - y;
        else if ( x >= 0 && y < 0 )
          z = 1.0 - x + y;
        else if ( x < 0 && y < 0 )
          z = 1.0 + x + y;
        else 
          z = 1.0 + x - y;

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

	// For this x,y grid location, set the index
	// The grid location ranges between 0 and (outer_size+inner_size)
	// in both x and y
	xindex = (int)((float)(x + 1 + 1.0 / 
			 (float)(2*inner_size))*
			 (float)(inner_size));

	yindex = (int)((float)(y + 1 + 1.0 / 
			 (float)(2*inner_size))*
			 (float)(inner_size));

        this->IndexTable[xindex*(outer_size+inner_size) + yindex] = index;

	// Increment the index
        index++;
	}
      }
    // On the even rows we are doing the big grid which has
    // outer_size elements in it
    else
      {
      for ( j = 0; j < outer_size; j++ )
	{
	// compute the x component for this column
        tmp_x = (float)(2*j)/(float)(inner_size) - 1.0;

	// rotate by 45 degrees
        x = 0.5 * tmp_x - 0.5 * tmp_y;
        y = 0.5 * tmp_x + 0.5 * tmp_y;

	// compute the z based on the x and y values
        if ( x >= 0 && y >= 0 )
          z = 1.0 - x - y;
        else if ( x >= 0 && y < 0 )
          z = 1.0 - x + y;
        else if ( x < 0 && y < 0 )
          z = 1.0 + x + y;
        else 
          z = 1.0 + x - y;

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

	// For this x,y grid location, set the index
	// The grid location ranges between 0 and 2*NORM_SQR_SIZE - 1
	// in both x and y
	xindex = (int)((float)(x + 1 + 1.0 / 
			 (float)(2*inner_size))*
			 (float)(inner_size));
	yindex = (int)((float)(y + 1 + 1.0 / 
			 (float)(2*inner_size))*
			 (float)(inner_size));

        this->IndexTable[xindex*(outer_size+inner_size) + yindex] = index;

	// Increment the index
        index++;
	}
      }
    }

  // The index table has been initialized for the current recursion
  // depth
  this->IndexTableRecursionDepth = this->RecursionDepth;
}


// Description:
// Print the vtkRecursiveSphereDirectionEncoder
void vtkRecursiveSphereDirectionEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDirectionEncoder::PrintSelf(os,indent);

  os << indent << "Number of encoded directions: " << 
    this->GetNumberOfEncodedDirections() << endl;

  os << indent << "Recursion depth: " << this->RecursionDepth << endl;
}
