/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunction.cxx
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
#include "vtkPiecewiseFunction.h"

// Description:
// Construct a new vtkPiecewiseFunction with default values
vtkPiecewiseFunction::vtkPiecewiseFunction()
{
  this->ArraySize     	 = 64;
  this->Clamping         = 1;
  this->Function      	 = new float[this->ArraySize*2];
  this->FunctionSize  	 = 0;
  this->FunctionRange[0] = 0;
  this->FunctionRange[1] = 0;
}

// Description:
// Destruct a vtkPiecewiseFunction
vtkPiecewiseFunction::~vtkPiecewiseFunction()
{
  if( this->Function )
    delete this->Function;
}

// Description:
// Return the number of points which specify this function
int vtkPiecewiseFunction::GetSize()
{
  return( this->FunctionSize );
}

// Description:
// Return the type of function stored in object:
// Function Types:
//    0 : Constant        (No change in slope between end points)
//    1 : NonDecreasing   (Always increasing or zero slope)
//    2 : NonIncreasing   (Always decreasing or zero slope)
//    3 : Varied          (Contains both decreasing and increasing slopes)
//    4 : Unknown         (Error condition)
//
char *vtkPiecewiseFunction::GetType()
{
  int   i;
  float value;
  float prev_value;
  int   function_type;

  function_type = 0;

  if( this->FunctionSize )
    prev_value = this->Function[1];

  for( i=1; i < this->FunctionSize; i++ )
    {
    value = this->Function[(2*i+1)];

    // Do not change the function type if equal
    if( value != prev_value )
      {
      if( value > prev_value )
        {
        switch( function_type )
          {
          case 0:
          case 1:
            function_type = 1;	// NonDecreasing
            break;
          case 2:
            function_type = 3;	// Varied
            break;
          }
        }
      else // value < prev_value
        {
        switch( function_type )
          {
          case 0:
          case 2:
            function_type = 2;	// NonIncreasing
            break;
          case 1:
            function_type = 3;	// Varied
            break;
          }
        } 
      }

    prev_value = value;

    // Exit loop if we find a Varied function
    if( function_type == 3 )
      break;
    }

  switch( function_type )
    {
    case 0:
      return( "Constant" );
    case 1:
      return( "NonDecreasing" );
    case 2:
      return( "NonIncreasing" );
    case 3:
      return( "Varied" );
    }

    return( "Unknown" );
}


// Description:
// Returns the first point location which starts a non-zero segment of the
// function. Note that the value at this point may be zero.
float vtkPiecewiseFunction::GetFirstNonZeroValue()
{
  int   i;
  int   all_zero = 1;
  float x = 0.0;

  // Check if no points specified
  if( this->FunctionSize == 0 )
    return( 0 );

  for( i=0; i < this->FunctionSize; i++ )
    {
    if( this->Function[(2*i+1)] != 0.0 )
      {
      x = this->Function[(2*i)];
      all_zero = 0;
      break;
      }
    }

  // If every specified point has a zero value then return the first points
  // position
  if( all_zero )
    x = this->Function[0];
  else  // A point was found with a non-zero value
    {
    if( i > 0 )
      // Return the value of the point that precedes this one
      x = this->Function[2*(i-1)];
    else
      // If this is the first point in the function, return its value
      x = this->Function[0];
    }
 
  return( x );
}

// Description:
// Adds a point to the function. If a duplicate point is inserted
// then the function value at that location is set to the new value.
void vtkPiecewiseFunction::AddPoint( float x, float val )
{
  this->InsertPoint( x, val );
}

// Description:
// Adds a point to the function and returns the array index of the point.
int vtkPiecewiseFunction::InsertPoint( float x, float val )
{
  int	point_index;
  int   i;

  // Increase function size if we exceed array bound
  if( (this->FunctionSize*2) >= this->ArraySize )
    this->IncreaseArraySize();

  // Insert the first point 
  if( this->FunctionSize == 0 )
    {
    // Set the point in the function
    this->Function[0]  = x;
    this->Function[1]  = val;
    this->FunctionSize = 1;

    // Update function range
    this->FunctionRange[0] = x;
    this->FunctionRange[1] = x;

    point_index = 0;
    }
  else // Insert a point inside list
    {
    i  = 0;

    // Find insertion index 
    while( (i < this->FunctionSize) && (this->Function[(i*2)] <= x) )
      {
      // Check for duplicate entries
      // Overwrite value if found
      if( x == this->Function[(i*2)] )
        {
        this->Function[(i*2 + 1)] = val;

        this->Modified();

        return( i );
        }

      // Move to next point
      i++;
      }

    this->FunctionSize++;

    // Move points down one element
    this->MovePoints( i, 1 );

    // Insert new point at index
    this->Function[(2*i)]   = x;
    this->Function[(2*i+1)] = val;
    point_index = i;

    // Update function range
    if( x < this->FunctionRange[0] )
      this->FunctionRange[0] = x;
    if( x > this->FunctionRange[1] )
      this->FunctionRange[1] = x;
    }

    this->Modified();

    return( point_index );
}

// Description:
// Moves all points to the right of index down or up by one index value
// depending on the down flag. Assumes that memory for move is already 
// allocated.
void vtkPiecewiseFunction::MovePoints( int index, int down )
{
  int i;

  float swap1_x, swap1_y;
  float swap2_x, swap2_y;

  i = index;

  if( down )
    {
    // Move points down (i+1) = i
    swap1_x = this->Function[(2*i)];
    swap1_y = this->Function[(2*i+1)];

    i = i + 1;

    // Move following points down
    while( i < this->FunctionSize )
      {
      swap2_x = this->Function[(2*i)];
      swap2_y = this->Function[(2*i+1)];
      
      this->Function[(2*i)] = swap1_x;
      this->Function[(2*i+1)] = swap1_y;

      swap1_x = swap2_x;
      swap1_y = swap2_y;

      i++;
      }

    }
  else 
    {
    // Move points up (i = i+1)
    // This destroys values at index i
    while( i < (this->FunctionSize-1) )
      {
      this->Function[(2*i)]   = this->Function[(2*(i+1))];
      this->Function[(2*i+1)] = this->Function[(2*(i+1)+1)];
      i++;
      }
    }
}

// Description:
// Removes a point from the function. If no point is found then function
// remains the same.
void vtkPiecewiseFunction::RemovePoint( float x )
{
  int   i;
  float x1;

  if( this->FunctionSize )
    {
    i  = 0;
    x1 = this->Function[0];

    // Locate the point in the array
    while( (x1 != x) && (i < this->FunctionSize) )
      {
      // Move to next point
      i++;
      x1 = this->Function[(i*2)];
      }

    // Remove the point
    if( i < this->FunctionSize )
      {
      this->MovePoints( i, 0 );

      this->FunctionSize--;

      this->Modified();
      }
    }
}

// Description:
// Removes all points from the function.
void vtkPiecewiseFunction::RemoveAllPoints()
{
  this->FunctionSize  	 = 0;
  this->FunctionRange[0] = 0;
  this->FunctionRange[1] = 0;
  this->Modified();
}

// Description:
// Add in end points of line and remove any points between them
void vtkPiecewiseFunction::AddSegment( float x1, float val1, 
  float x2, float val2 )
{
  int	index1, index2;
  int	swap;
  int	num_points;
  int   distance;
  int   i;

  // Insert the two endpoints
  index1 = this->InsertPoint( x1, val1 );
  index2 = this->InsertPoint( x2, val2 );

  if( index1 == index2 )
    return;

  if( index1 > index2 )
    {
    swap = index1;
    index1 = index2;
    index2 = swap;
    }

  num_points = this->FunctionSize - index2;
  distance = index2 - index1 - 1;

  // Loop between index2 and last point and remove points
  for( i=index2; i < this->FunctionSize; i++ )
    {
    this->Function[(2*(i-distance))]   = this->Function[(2*i)];
    this->Function[(2*(i-distance)+1)] = this->Function[(2*i+1)];
    }

  this->FunctionSize = this->FunctionSize - distance;
}

// Description:
// Return the value of the function at a position 
float vtkPiecewiseFunction::GetValue( float x )
{
  int   i1, i2;
  float x1, y1;	// Point before x
  float x2, y2;	// Point after x

  float slope;
  float value;

  if( this->FunctionSize == 0 )
    return 0.0;

  if( this->Clamping == 1 )  // Clamped to lowest value below range and highest above range
    {
    // Check to see if point is out of range
    if( x < this->FunctionRange[0] ) 
      x = this->Function[0];
    else if( x > this->FunctionRange[1] )
      x = this->Function[(this->FunctionSize-1)*2];
    }
  else if( this->Clamping == 0 )	// Always zero outside of range
    {
    if( (x < this->FunctionRange[0]) || (x > this->FunctionRange[1]) )
      return 0.0;
    }
  else
    {
    vtkErrorMacro( << "Error: vtkPiecewiseFunction has an unknown clamp type: " << this->Clamping << "\n" );
    return 0.0;
    }

  i2 = 0;
  x2 = this->Function[0];
  y2 = this->Function[1];

  while( (x2 < x) && (i2 < this->FunctionSize) )
    {
    i2 += 1;
    x2 = this->Function[(i2*2)];
    y2 = this->Function[(i2*2+1)];
    }

  // Check if we have found the exact point
  if( x2 == x )
    return( this->Function[(i2*2 + 1)] );
  else
    {
    i1 = i2 - 1;
    x1 = this->Function[(i1*2)];
    y1 = this->Function[(i1*2 +1)];
    }

  // Now that we have the two points, use linear interpolation
  slope = (y2-y1)/(x2-x1);

  value = y1 + slope*(x-x1);

  return( value );
}

// Description:
// Return the smallest and largest position stored in function
float *vtkPiecewiseFunction::GetRange()
{
  return( this->FunctionRange );
}

// Description:
// Returns a table of function values evaluated at regular intervals
void vtkPiecewiseFunction::GetTable( float x1, float x2, int size, float* table )
{
  float x;
  float inc;
  int   i;

  if( x1 == x2 )
    return;

  x = x1;
  if( size > 1 )
    inc = (x2-x1)/(float)(size-1);
  else
    inc = 0;

  for( i=0; i<size; i++ )
    {
    table[i] = this->GetValue( x );
    x += inc;
    }
}

// Description:
// Increase the size of the array used to store the function
void vtkPiecewiseFunction::IncreaseArraySize()
{
  float *old_function;
  int   old_size;

  int   i;

  old_function = this->Function;
  old_size     = this->ArraySize;

  // Create larger array to store points
  this->ArraySize = old_size * 2;
  this->Function  = new float[(this->ArraySize*2)];

  // Copy points from old array to new array
  for( i=0; i<old_size; i++ )
    {
      this->Function[(2*i)]   = old_function[(2*i)];
      this->Function[(2*i)+1] = old_function[(2*i)+1];
    }

  delete old_function;
}

// Description:
// Print method for tkPiecewiseFunction
void vtkPiecewiseFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkObject::PrintSelf(os, indent);

  os << indent << "Clamping: " << this->Clamping << "\n";
  os << indent << "Function Points: " << this->GetSize() << "\n";
  for( i=0; i<this->FunctionSize; i++ )
    os << indent << indent << i << ": " << this->Function[(2*i)] << ", " << this->Function[(2*i+1)] << "\n";
}

