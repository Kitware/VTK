/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPiecewiseFunction.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPiecewiseFunction, "1.41.8.1");
vtkStandardNewMacro(vtkPiecewiseFunction);

// Construct a new vtkPiecewiseFunction with default values
vtkPiecewiseFunction::vtkPiecewiseFunction()
{
  this->ArraySize        = 64;
  this->Clamping         = 1;
  this->Function         = new double[this->ArraySize*2];
  this->FunctionSize     = 0;
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
    memcpy( this->FunctionRange, f->FunctionRange, 2*sizeof(double) );
    if ( this->ArraySize > 0 )
      {
      delete [] this->Function;
      this->Function     = new double[this->ArraySize*2];
      memcpy( this->Function, f->Function, this->ArraySize*2*sizeof(double) );
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
    this->Function     = new double[this->ArraySize*2]; 
    this->FunctionSize = f->FunctionSize;
    memcpy( this->FunctionRange, f->FunctionRange, 2*sizeof(double) );
    memcpy( this->Function, f->Function, this->ArraySize*2*sizeof(double) );
    }

  // Do the superclass
  this->vtkDataObject::ShallowCopy(o);
}

void vtkPiecewiseFunction::Initialize()
{
  if ( this->Function)
    {
    delete [] this->Function;
    }

  this->ArraySize        = 64;
  this->Clamping         = 1;
  this->Function         = new double[this->ArraySize*2];
  this->FunctionSize     = 0;
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
  return this->FunctionSize;
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
  double value;
  double prev_value = 0.0;
  int   function_type;

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
            function_type = 1;  // NonDecreasing
            break;
          case 2:
            function_type = 3;  // Varied
            break;
          }
        }
      else // value < prev_value
        {
        switch( function_type )
          {
          case 0:
          case 2:
            function_type = 2;  // NonIncreasing
            break;
          case 1:
            function_type = 3;  // Varied
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
      return "Constant";
    case 1:
      return "NonDecreasing";
    case 2:
      return "NonIncreasing";
    case 3:
      return "Varied";
    }

    return "Unknown";
}


// Returns the first point location which starts a non-zero segment of the
// function. Note that the value at this point may be zero.
double vtkPiecewiseFunction::GetFirstNonZeroValue()
{
  int   i;
  int   all_zero = 1;
  double x = 0.0;

  // Check if no points specified
  if( this->FunctionSize == 0 )
    {
    return 0;
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
 
  return x;
}

// Adds a point to the function. If a duplicate point is inserted
// then the function value at that location is set to the new value.
int vtkPiecewiseFunction::AddPoint( double x, double val )
{
  return this->InsertPoint( x, val );
}

// Adds a point to the function and returns the array index of the point.
int vtkPiecewiseFunction::InsertPoint( double x, double val )
{
  int   point_index;
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
        if (this->Function[(i*2 + 1)] != val)
          {
          this->Function[(i*2 + 1)] = val;
          this->Modified();
          }
        return i;
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

    return point_index;
}

// Moves all points to the right of index down or up by one index value
// depending on the down flag. Assumes that memory for move is already 
// allocated.
void vtkPiecewiseFunction::MovePoints( int index, int down )
{
  int i;

  double swap1_x, swap1_y;
  double swap2_x, swap2_y;

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
int vtkPiecewiseFunction::RemovePoint( double x )
{
  int   i;
  double x1;

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
      return i;
      }
    }
  return -1;
}

// Removes all points from the function.
void vtkPiecewiseFunction::RemoveAllPoints()
{
  if (!this->FunctionSize)
    {
    return;
    }
  this->FunctionSize     = 0;
  this->FunctionRange[0] = 0;
  this->FunctionRange[1] = 0;
  this->Modified();
}

// Add in end points of line and remove any points between them
void vtkPiecewiseFunction::AddSegment( double x1, double val1, 
                                       double x2, double val2 )
{
  int   index1, index2;
  int   swap;
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
double vtkPiecewiseFunction::GetValue( double x )
{
  int   i1, i2;
  double x1, y1; // Point before x
  double x2, y2; // Point after x

  double slope;
  double value;

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
  else if( this->Clamping == 0 )        // Always zero outside of range
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
    return this->Function[(i2*2 + 1)];
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

  return value;
}

// Return the smallest and largest position stored in function
double *vtkPiecewiseFunction::GetRange()
{
  return this->FunctionRange;
}

int vtkPiecewiseFunction::AdjustRange(double range[2])
{
  if (!range)
    {
    return 0;
    }

  double *function_range = this->GetRange();
  
  // Make sure we have points at each end of the range

  if (function_range[0] < range[0])
    {
    this->AddPoint(range[0], this->GetValue(range[0]));
    }
  else
    {
    this->AddPoint(range[0], this->GetValue(function_range[0]));
    }

  if (function_range[1] > range[1])
    {
    this->AddPoint(range[1], this->GetValue(range[1]));
    }
  else
    {
    this->AddPoint(range[1], this->GetValue(function_range[1]));
    }

  // Remove all points out-of-range

  int func_size = this->GetSize();
  double *func_ptr = this->GetDataPointer();
  
  int i;
  for (i = func_size - 1; i >= 0; i--)
    {
    double x = func_ptr[i * 2];
    if (x < range[0] || x > range[1])
      {
      this->RemovePoint(x);
      }
    }

  return 1;
}

// Returns a table of function values evaluated at regular intervals
void vtkPiecewiseFunction::GetTable( double x1, double x2, int size,
                                     double* table, int stride )
{
  double x, xi1, xi2, yi1, yi2, tx;
  double inc, value, slope, *tbl;
  int   i, i1, i2;

  if( x1 == x2 )
    {
    return;
    }

  if( size > 1 )
    {
    inc = (x2-x1)/(double)(size-1);
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
    else if( this->Clamping == 0 )      // Always zero outside of range
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

void vtkPiecewiseFunction::GetTable( double x1, double x2, int size,
                                     float* table, int stride )
{
  double x, xi1, xi2, yi1, yi2, tx;
  double inc, value, slope;
  float *tbl;
  int   i, i1, i2;

  if( x1 == x2 )
    {
    return;
    }

  if( size > 1 )
    {
    inc = (x2-x1)/(double)(size-1);
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
    else if( this->Clamping == 0 )      // Always zero outside of range
      {
      if( (x < this->FunctionRange[0]) || (x > this->FunctionRange[1]) )
        {
        *tbl = 0.0f;
        tbl += stride;
        x += inc;
        continue;
        }
      }
    else
      {
      vtkErrorMacro( << "Error: vtkPiecewiseFunction has an unknown clamp type: " << this->Clamping << "\n" );
      *tbl =  0.0f;
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
    
    *tbl = static_cast<float>(value);
    tbl += stride;
    x += inc;
    }
}

void vtkPiecewiseFunction::BuildFunctionFromTable( double x1, double x2,
                                                   int size,
                                                   double* table, int stride )
{
  int i;
  double inc = 0.0;
  double *tptr = table;

  if (size > this->ArraySize)
    {
    delete [] this->Function;
    this->ArraySize = size;
    this->FunctionSize = size;
    this->Function = new double[this->ArraySize*2];
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
    inc = (x2-x1)/(double)(size-1);
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
  double *old_function;
  int   old_size;

  int   i;

  old_function = this->Function;
  old_size     = this->ArraySize;

  // Create larger array to store points
  this->ArraySize = old_size * 2;
  this->Function  = new double[(this->ArraySize*2)];

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

void vtkPiecewiseFunction::FillFromDataPointer(int nb, double *ptr)
{
  if (nb <= 0 || !ptr)
    {
    return;
    }

  this->RemoveAllPoints();

  while (nb)
    {
    this->AddPoint(ptr[0], ptr[1]);
    ptr += 2;
    nb--;
    }
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkPiecewiseFunction::GetData(vtkInformation* info)
{
  return
    info? vtkPiecewiseFunction::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkPiecewiseFunction::GetData(vtkInformationVector* v,
                                                    int i)
{
  return vtkPiecewiseFunction::GetData(v->GetInformationObject(i));
}

// Print method for tkPiecewiseFunction
void vtkPiecewiseFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  int i;

  os << indent << "Clamping: " << this->Clamping << "\n";
  os << indent << "Function Points: " << this->GetSize() << "\n";
  for( i = 0; i < this->FunctionSize; i++ )
    {
    os << indent << indent << i << ": " 
       << this->Function[(2*i)] << ", " << this->Function[(2*i+1)] << "\n";
    }
}
