/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPiecewiseFunction.h"
#include "vtkSource.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPiecewiseFunction* vtkPiecewiseFunction::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPiecewiseFunction");
  if(ret)
    {
    return (vtkPiecewiseFunction*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPiecewiseFunction;
}




// Construct a new vtkPiecewiseFunction with default values
vtkPiecewiseFunction::vtkPiecewiseFunction()
{
  this->ArraySize     	 = 64;
  this->Clamping         = 1;
  this->Function      	 = new float[this->ArraySize*2];
  this->FunctionSize  	 = 0;
  this->FunctionRange[0] = 0;
  this->FunctionRange[1] = 0;

  for (int i=0; i < this->ArraySize*2; i++)
    {
    this->Function[i] = 0.0;
    }
}

// Destruct a vtkPiecewiseFunction
vtkPiecewiseFunction::~vtkPiecewiseFunction()
{
  if( this->Function )
    {
    delete [] this->Function;
    }
}

void vtkPiecewiseFunction::DeepCopy( vtkDataObject *o )
{
  vtkPiecewiseFunction *f = vtkPiecewiseFunction::SafeDownCast(o);

  if (f != NULL)
    {
    this->ArraySize    = f->ArraySize;
    this->Clamping     = f->Clamping;
    this->FunctionSize = f->FunctionSize;
    memcpy( this->FunctionRange, f->FunctionRange, 2*sizeof(float) );
    if ( this->ArraySize > 0 )
      {
      delete [] this->Function;
      this->Function     = new float[this->ArraySize*2];
      memcpy( this->Function, f->Function, this->ArraySize*2*sizeof(float) );
      }
    
    this->Modified();
    }

  // Do the superclass
  this->vtkDataObject::DeepCopy(o);
}

void vtkPiecewiseFunction::ShallowCopy( vtkDataObject *o )
{
  vtkPiecewiseFunction *f = vtkPiecewiseFunction::SafeDownCast(o);

  if (f != NULL)
    {
    this->ArraySize    = f->ArraySize;
    this->Clamping     = f->Clamping;
    this->Function     = new float[this->ArraySize*2]; 
    this->FunctionSize = f->FunctionSize;
    memcpy( this->FunctionRange, f->FunctionRange, 2*sizeof(float) );
    memcpy( this->Function, f->Function, this->ArraySize*2*sizeof(float) );
    }

  // Do the superclass
  this->vtkDataObject::ShallowCopy(o);
}

vtkDataObject *vtkPiecewiseFunction::MakeObject()
{
  vtkPiecewiseFunction *f;

  f = vtkPiecewiseFunction::New();
  f->DeepCopy( this );
  return (vtkDataObject *)f;
}

void vtkPiecewiseFunction::Initialize()
{
  if ( this->Function)
    {
    delete [] this->Function;
    }

  this->ArraySize     	 = 64;
  this->Clamping         = 1;
  this->Function      	 = new float[this->ArraySize*2];
  this->FunctionSize  	 = 0;
  this->FunctionRange[0] = 0;
  this->FunctionRange[1] = 0;

  for (int i=0; i < this->ArraySize*2; i++)
    {
    this->Function[i] = 0.0;
    }
}


// Return the number of points which specify this function
int vtkPiecewiseFunction::GetSize()
{
  this->Update();
  return( this->FunctionSize );
}

// Return the type of function stored in object:
// Function Types:
//    0 : Constant        (No change in slope between end points)
//    1 : NonDecreasing   (Always increasing or zero slope)
//    2 : NonIncreasing   (Always decreasing or zero slope)
//    3 : Varied          (Contains both decreasing and increasing slopes)
//    4 : Unknown         (Error condition)
//
const char *vtkPiecewiseFunction::GetType()
{
  int   i;
  float value;
  float prev_value = 0.0;
  int   function_type;

  this->Update();

  function_type = 0;

  if( this->FunctionSize )
    {
    prev_value = this->Function[1];
    }

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
      {
      break;
      }
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


// Return the mtime of this object, or the source - whicheve is greater
// This way the pipeline will update correctly
unsigned long vtkPiecewiseFunction::GetMTime()
{
  unsigned long mt1, mt2, mtime;

  mt1 = this->vtkObject::GetMTime();

  if ( this->Source )
    {
    mt2 = this->Source->GetMTime();
    }
  else
    {
    mt2 = 0;
    }

  mtime = (mt1 > mt2)?(mt1):(mt2);

  return mtime;
}

// Returns the first point location which starts a non-zero segment of the
// function. Note that the value at this point may be zero.
float vtkPiecewiseFunction::GetFirstNonZeroValue()
{
  int   i;
  int   all_zero = 1;
  float x = 0.0;

  this->Update();

  // Check if no points specified
  if( this->FunctionSize == 0 )
    {
    return( 0 );
    }

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
    {
    x = this->Function[0];
    }
  else  // A point was found with a non-zero value
    {
    if( i > 0 )
      // Return the value of the point that precedes this one
      {
      x = this->Function[2*(i-1)];
      }
    else
      // If this is the first point in the function, return its value
      {
      x = this->Function[0];
      }
    }
 
  return( x );
}

// Adds a point to the function. If a duplicate point is inserted
// then the function value at that location is set to the new value.
void vtkPiecewiseFunction::AddPoint( float x, float val )
{
  this->InsertPoint( x, val );
}

// Adds a point to the function and returns the array index of the point.
int vtkPiecewiseFunction::InsertPoint( float x, float val )
{
  int	point_index;
  int   i;

  // Increase function size if we exceed array bound
  if( (this->FunctionSize*2) >= this->ArraySize )
    {
    this->IncreaseArraySize();
    }

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
      {
      this->FunctionRange[0] = x;
      }
    if( x > this->FunctionRange[1] )
      {
      this->FunctionRange[1] = x;
      }
    }

    this->Modified();

    return( point_index );
}

// Moves all points to the right of index down or up by one index value
// depending on the down flag. Assumes that memory for move is already 
// allocated.
void vtkPiecewiseFunction::MovePoints( int index, int down )
{
  int i;

  float swap1_x, swap1_y;
  float swap2_x, swap2_y;

  i = index;

  // Moving down
  if( down )
    {
    // If it is the last point we don't need to move anything
    if ( (i+1) < this->FunctionSize )
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
    }
  // Moving up
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

      if (this->FunctionSize > 0)
	{
	this->FunctionRange[0] = this->Function[0];
	this->FunctionRange[1] = this->Function[2*(this->FunctionSize-1)];
	}
      else
	{
	this->FunctionRange[0] = this->FunctionRange[1] = 0.0;
	}
      this->Modified();
      }
    }
}

// Removes all points from the function.
void vtkPiecewiseFunction::RemoveAllPoints()
{
  this->FunctionSize  	 = 0;
  this->FunctionRange[0] = 0;
  this->FunctionRange[1] = 0;
  this->Modified();
}

// Add in end points of line and remove any points between them
void vtkPiecewiseFunction::AddSegment( float x1, float val1, 
  float x2, float val2 )
{
  int	index1, index2;
  int	swap;
  int   distance;
  int   i;

  // Insert the two endpoints
  index1 = this->InsertPoint( x1, val1 );
  index2 = this->InsertPoint( x2, val2 );

  if( index1 == index2 )
    {
    return;
    }

  if( index1 > index2 )
    {
    swap = index1;
    index1 = index2;
    index2 = swap;
    }

  distance = index2 - index1 - 1;

  // Loop between index2 and last point and remove points
  for( i=index2; i < this->FunctionSize; i++ )
    {
    this->Function[(2*(i-distance))]   = this->Function[(2*i)];
    this->Function[(2*(i-distance)+1)] = this->Function[(2*i+1)];
    }

  this->FunctionSize = this->FunctionSize - distance;
}

// Return the value of the function at a position 
float vtkPiecewiseFunction::GetValue( float x )
{
  int   i1, i2;
  float x1, y1;	// Point before x
  float x2, y2;	// Point after x

  float slope;
  float value;

  this->Update();

  if( this->FunctionSize == 0 )
    {
    return 0.0;
    }

  if( this->Clamping == 1 )  // Clamped to lowest value below range and highest above range
    {
    // Check to see if point is out of range
    if( x < this->FunctionRange[0] ) 
      {
      x = this->Function[0];
      }
    else if( x > this->FunctionRange[1] )
      {
      x = this->Function[(this->FunctionSize-1)*2];
      }
    }
  else if( this->Clamping == 0 )	// Always zero outside of range
    {
    if( (x < this->FunctionRange[0]) || (x > this->FunctionRange[1]) )
      {
      return 0.0;
      }
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
    {
    return( this->Function[(i2*2 + 1)] );
    }
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

// Return the smallest and largest position stored in function
float *vtkPiecewiseFunction::GetRange()
{
  return( this->FunctionRange );
}

// Returns a table of function values evaluated at regular intervals
void vtkPiecewiseFunction::GetTable( float x1, float x2, int size,
				     float* table, int stride )
{
  float x, xi1, xi2, yi1, yi2, tx;
  float inc, value, slope, *tbl;
  int   i, i1, i2;

  this->Update();

  if( x1 == x2 )
    {
    return;
    }

  if( size > 1 )
    {
    inc = (x2-x1)/(float)(size-1);
    }
  else
    {
    inc = 0;
    }

  tbl = table;
  x = x1;
  i2 = 0;
  xi2 = this->Function[0];
  yi2 = this->Function[1];
  for (i=0; i < size; i++)
    {
    tx = x;
    
    // Clamped to lowest value below range and highest above range
    if( this->Clamping == 1 )  
      {
      if( x < this->FunctionRange[0] ) 
	{
	tx = this->Function[0];
	}
      else if( x > this->FunctionRange[1] )
	{
	tx = this->Function[(this->FunctionSize-1)*2];
	}
      }
    else if( this->Clamping == 0 )	// Always zero outside of range
      {
      if( (x < this->FunctionRange[0]) || (x > this->FunctionRange[1]) )
	{
	*tbl = 0.0;
	tbl += stride;
	x += inc;
	continue;
	}
      }
    else
      {
      vtkErrorMacro( << "Error: vtkPiecewiseFunction has an unknown clamp type: " << this->Clamping << "\n" );
      *tbl =  0.0;
      tbl += stride;
      x += inc;
      continue;
      }

    // search for the end of the interval containing x
    while( (xi2 < tx) && (i2 < this->FunctionSize) )
      {
      i2 += 1;
      xi2 = this->Function[(i2*2)];
      yi2 = this->Function[(i2*2+1)];
      }
    
    // Check if we have found the exact point
    if( xi2 == tx )
      {
      value = this->Function[(i2*2 + 1)];
      }
    else
      {
      i1 = i2 - 1;
      xi1 = this->Function[(i1*2)];
      yi1 = this->Function[(i1*2 +1)];

      // Now that we have the two points, use linear interpolation
      slope = (yi2-yi1)/(xi2-xi1);
    
      value = yi1 + slope*(tx-xi1);
      }
    
    *tbl = value;
    tbl += stride;
    x += inc;
    }
}


void vtkPiecewiseFunction::BuildFunctionFromTable( float x1, float x2,
						   int size,
						   float* table, int stride )
{
  int i;
  float inc = 0.0;
  float *tptr = table;

  if (size > this->ArraySize)
    {
    delete [] this->Function;
    this->ArraySize = size;
    this->FunctionSize = size;
    this->Function = new float[this->ArraySize*2];
    }
  else
    {
    // no need to reallocate memory, just adjust the function size
    this->FunctionSize = size;
    }

  this->FunctionRange[0] = x1;
  this->FunctionRange[1] = x2;
  
  if( size > 1 )
    {
    inc = (x2-x1)/(float)(size-1);
    }

  for (i=0; i < size; i++)
    {
    this->Function[2*i] = x1 + inc*i;
    this->Function[2*i+1] = *tptr;
    tptr += stride;
    }

  this->Modified();
}

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

  // Initialize the rest of the memory to avoid purify problems
  for ( ; i < this->ArraySize; i++ )
    {
    this->Function[(2*i)]   = 0;
    this->Function[(2*i)+1] = 0;
    }

  delete [] old_function;
}

// Print method for tkPiecewiseFunction
void vtkPiecewiseFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  this->vtkDataObject::PrintSelf(os, indent);

  os << indent << "Clamping: " << this->Clamping << "\n";
  os << indent << "Function Points: " << this->GetSize() << "\n";
  for( i = 0; i < this->FunctionSize; i++ )
    {
    os << indent << indent << i << ": " 
       << this->Function[(2*i)] << ", " << this->Function[(2*i+1)] << "\n";
    }
}
