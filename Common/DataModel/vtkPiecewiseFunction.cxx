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

#include <vector>
#include <set>
#include <algorithm>
#include <iterator>

vtkStandardNewMacro(vtkPiecewiseFunction);

// The Node structure
class vtkPiecewiseFunctionNode
{
public:
  double X;
  double Y;
  double Sharpness;
  double Midpoint;
};

// A comparison method for sorting nodes in increasing order
class vtkPiecewiseFunctionCompareNodes
{
public:
  bool operator () ( const vtkPiecewiseFunctionNode *node1,
                     const vtkPiecewiseFunctionNode *node2 )
    {
      return node1->X < node2->X;
    }
};

// A find method for finding a particular node in the function
class vtkPiecewiseFunctionFindNodeEqual
{
public:
  double X;
  bool operator () ( const vtkPiecewiseFunctionNode *node )
    {
      return node->X == this->X;
    }
};

// A find method for finding nodes inside a specified range
class vtkPiecewiseFunctionFindNodeInRange
{
public:
  double X1;
  double X2;
  bool operator () ( const vtkPiecewiseFunctionNode *node )
    {
      return ( node->X >= this->X1 &&
               node->X <= this->X2 );
    }
};

// A find method for finding nodes outside a specified range
class vtkPiecewiseFunctionFindNodeOutOfRange
{
public:
  double X1;
  double X2;
  bool operator () ( const vtkPiecewiseFunctionNode *node )
    {
      return ( node->X < this->X1 ||
               node->X > this->X2 );
    }
};

// The internal structure for containing the STL objects
class vtkPiecewiseFunctionInternals
{
public:
  std::vector<vtkPiecewiseFunctionNode*> Nodes;
  vtkPiecewiseFunctionCompareNodes        CompareNodes;
  vtkPiecewiseFunctionFindNodeEqual       FindNodeEqual;
  vtkPiecewiseFunctionFindNodeInRange     FindNodeInRange;
  vtkPiecewiseFunctionFindNodeOutOfRange  FindNodeOutOfRange;
};

// Construct a new vtkPiecewiseFunction with default values
vtkPiecewiseFunction::vtkPiecewiseFunction()
{
  this->Clamping = 1;
  this->Range[0] = 0;
  this->Range[1] = 0;

  this->Function = NULL;

  this->AllowDuplicateScalars = 0;

  this->Internal = new vtkPiecewiseFunctionInternals;
}

// Destruct a vtkPiecewiseFunction
vtkPiecewiseFunction::~vtkPiecewiseFunction()
{
  if( this->Function )
    {
    delete [] this->Function;
    }

  for(unsigned int i=0;i<this->Internal->Nodes.size();i++)
    {
    delete this->Internal->Nodes[i];
    }
  this->Internal->Nodes.clear();
  delete this->Internal;
}

void vtkPiecewiseFunction::DeepCopy( vtkDataObject *o )
{
  vtkPiecewiseFunction *f = vtkPiecewiseFunction::SafeDownCast(o);

  if (f != NULL)
    {
    this->Clamping     = f->Clamping;
    int i;
    this->RemoveAllPoints();
    for ( i = 0; i < f->GetSize(); i++ )
      {
      double val[4];
      f->GetNodeValue(i, val);
      this->AddPoint(val[0], val[1], val[2], val[3]);
      }
    this->Modified();
    }

  // Do the superclass
  this->Superclass::DeepCopy(o);
}

void vtkPiecewiseFunction::ShallowCopy( vtkDataObject *o )
{
  vtkPiecewiseFunction *f = vtkPiecewiseFunction::SafeDownCast(o);

  if (f != NULL)
    {
    this->Clamping     = f->Clamping;
    int i;
    this->RemoveAllPoints();
    for ( i = 0; i < f->GetSize(); i++ )
      {
      double val[4];
      f->GetNodeValue(i, val);
      this->AddPoint(val[0], val[1], val[2], val[3]);
      }
    this->Modified();
    }

  // Do the superclass
  this->vtkDataObject::ShallowCopy(o);
}

// This is a legacy method that is no longer needed
void vtkPiecewiseFunction::Initialize()
{
  this->RemoveAllPoints();
}


// Return the number of points which specify this function
int vtkPiecewiseFunction::GetSize()
{
  return static_cast<int>(this->Internal->Nodes.size());
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
  unsigned int   i;
  double value;
  double prev_value = 0.0;
  int   function_type;

  function_type = 0;

  if( this->Internal->Nodes.size() )
    {
    prev_value = this->Internal->Nodes[0]->Y;
    }

  for( i=1; i < this->Internal->Nodes.size(); i++ )
    {
    value = this->Internal->Nodes[i]->Y;

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

// Since we no longer store the data in an array, we must
// copy out of the vector into an array. No modified check -
// could be added if performance is a problem
double *vtkPiecewiseFunction::GetDataPointer()
{
  int size = static_cast<int>(this->Internal->Nodes.size());

  if ( this->Function )
    {
    delete [] this->Function;
    this->Function = NULL;
    }

  if ( size > 0 )
    {
    this->Function = new double[size*2];
    for ( int i = 0; i < size; i++ )
      {
      this->Function[2*i  ] = this->Internal->Nodes[i]->X;
      this->Function[2*i+1] = this->Internal->Nodes[i]->Y;
      }
    }

  return this->Function;
}

// Returns the first point location which starts a non-zero segment of the
// function. Note that the value at this point may be zero.
double vtkPiecewiseFunction::GetFirstNonZeroValue()
{
  unsigned int   i;
  int   all_zero = 1;
  double x = 0.0;

  // Check if no points specified
  if( this->Internal->Nodes.size() == 0 )
    {
    return 0;
    }

  for( i=0; i < this->Internal->Nodes.size(); i++ )
    {
    if( this->Internal->Nodes[i]->Y != 0.0 )
      {
      x = this->Internal->Nodes[i]->X;
      all_zero = 0;
      break;
      }
    }

  // If every specified point has a zero value then return
  // a large value
  if( all_zero )
    {
    x = VTK_DOUBLE_MAX;
    }
  else  // A point was found with a non-zero value
    {
    if( i > 0 )
      // Return the value of the point that precedes this one
      {
      x = this->Internal->Nodes[i-1]->X;
      }
    else
      // If this is the first point in the function, return its
      // value is clamping is off, otherwise VTK_DOUBLE_MIN if
      // clamping is on.
      {
      if ( this->Clamping )
        {
        x = VTK_DOUBLE_MIN;
        }
      else
        {
        x = this->Internal->Nodes[0]->X;
        }
      }
    }

  return x;
}

// For a specified index value, get the node parameters
int vtkPiecewiseFunction::GetNodeValue( int index, double val[4] )
{
  int size = static_cast<int>(this->Internal->Nodes.size());

  if ( index < 0 || index >= size )
    {
    vtkErrorMacro("Index out of range!");
    return -1;
    }

  val[0] = this->Internal->Nodes[index]->X;
  val[1] = this->Internal->Nodes[index]->Y;
  val[2] = this->Internal->Nodes[index]->Midpoint;
  val[3] = this->Internal->Nodes[index]->Sharpness;

  return 1;
}

// For a specified index value, get the node parameters
int vtkPiecewiseFunction::SetNodeValue( int index, double val[4] )
{
  int size = static_cast<int>(this->Internal->Nodes.size());

  if ( index < 0 || index >= size )
    {
    vtkErrorMacro("Index out of range!");
    return -1;
    }

  double oldX = this->Internal->Nodes[index]->X;
  this->Internal->Nodes[index]->X = val[0];
  this->Internal->Nodes[index]->Y = val[1];
  this->Internal->Nodes[index]->Midpoint = val[2];
  this->Internal->Nodes[index]->Sharpness = val[3];

  if (oldX != val[0])
    {
    // The point has been moved, the order of points or the range might have
    // been modified.
    this->SortAndUpdateRange();
    // No need to call Modified() here because SortAndUpdateRange() has done it
    // already.
    }
  else
    {
    this->Modified();
    }

  return 1;
}

// Adds a point to the function. If a duplicate point is inserted
// then the function value at that location is set to the new value.
// This is the legacy version that assumes midpoint = 0.5 and
// sharpness = 0.0
int vtkPiecewiseFunction::AddPoint( double x, double y )
{
  return this->AddPoint( x, y, 0.5, 0.0 );
}

// Adds a point to the function and returns the array index of the point.
int vtkPiecewiseFunction::AddPoint( double x, double y,
                                    double midpoint, double sharpness )
{
  // Error check
  if ( midpoint < 0.0 || midpoint > 1.0 )
    {
    vtkErrorMacro("Midpoint outside range [0.0, 1.0]");
    return -1;
    }

  if ( sharpness < 0.0 || sharpness > 1.0 )
    {
    vtkErrorMacro("Sharpness outside range [0.0, 1.0]");
    return -1;
    }

  // remove any node already at this X location
  if (!this->AllowDuplicateScalars)
    {
    this->RemovePoint( x );
    }

  // Create the new node
  vtkPiecewiseFunctionNode *node = new vtkPiecewiseFunctionNode;
  node->X         = x;
  node->Y         = y;
  node->Sharpness = sharpness;
  node->Midpoint  = midpoint;

  // Add it, then sort to get everything in order
  this->Internal->Nodes.push_back(node);
  this->SortAndUpdateRange();

  // Now find this node so we can return the index
  unsigned int i;
  for ( i = 0; i < this->Internal->Nodes.size(); i++ )
    {
    if ( this->Internal->Nodes[i]->X == x )
      {
      break;
      }
    }

  int retVal;

  // If we didn't find it, something went horribly wrong so
  // return -1
  if ( i < this->Internal->Nodes.size() )
    {
    retVal = i;
    }
  else
    {
    retVal = -1;
    }

  return retVal;
}

// Sort the vector in increasing order, then fill in
// the Range
void vtkPiecewiseFunction::SortAndUpdateRange()
{
  std::sort( this->Internal->Nodes.begin(),
                this->Internal->Nodes.end(),
                this->Internal->CompareNodes );
  bool modifiedInvoked = this->UpdateRange();
  // If range is updated, Modified() has been called, don't call it again.
  if (!modifiedInvoked)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkPiecewiseFunction::UpdateRange()
{
  double oldRange[2];
  oldRange[0] = this->Range[0];
  oldRange[1] = this->Range[1];

  int size = static_cast<int>(this->Internal->Nodes.size());
  if ( size )
    {
    this->Range[0] = this->Internal->Nodes[0]->X;
    this->Range[1] = this->Internal->Nodes[size-1]->X;
    }
  else
    {
    this->Range[0] = 0;
    this->Range[1] = 0;
    }
  // If the rage is the same, then no need to call Modified()
  if (oldRange[0] == this->Range[0] && oldRange[1] == this->Range[1])
    {
    return false;
    }

  this->Modified();
  return true;
}

// Removes a point from the function. If no point is found then function
// remains the same.
int vtkPiecewiseFunction::RemovePoint( double x )
{
  // First find the node since we need to know its
  // index as our return value
  unsigned int i;
  for ( i = 0; i < this->Internal->Nodes.size(); i++ )
    {
    if ( this->Internal->Nodes[i]->X == x )
      {
      break;
      }
    }

  int retVal;

  // If the node doesn't exist, we return -1
  if ( i < this->Internal->Nodes.size() )
    {
    retVal = i;
    }
  else
    {
    return -1;
    }

  // Now use STL to find it, so that we can remove it
  this->Internal->FindNodeEqual.X = x;

  std::vector<vtkPiecewiseFunctionNode*>::iterator iter =
    std::find_if(this->Internal->Nodes.begin(),
                    this->Internal->Nodes.end(),
                    this->Internal->FindNodeEqual );

  // Actually delete it
  if ( iter != this->Internal->Nodes.end() )
    {
    delete *iter;
    this->Internal->Nodes.erase(iter);
    // if the first or last point has been removed, then we update the range
    // No need to sort here as the order of points hasn't changed.
    bool modifiedInvoked = false;
    if (i == 0 || i == this->Internal->Nodes.size())
      {
      modifiedInvoked = this->UpdateRange();
      }
    if (!modifiedInvoked)
      {
      this->Modified();
      }
    }
  else
     {
     // This should never happen - we already returned if the node
     // didn't exist...
     return -1;
     }


  return retVal;
}

// Removes all points from the function.
void vtkPiecewiseFunction::RemoveAllPoints()
{
  for(unsigned int i=0;i<this->Internal->Nodes.size();i++)
    {
    delete this->Internal->Nodes[i];
    }
  this->Internal->Nodes.clear();

  this->SortAndUpdateRange();
}

// Add in end points of line and remove any points between them
// Legacy method with no way to specify midpoint and sharpness
void vtkPiecewiseFunction::AddSegment( double x1, double y1,
                                       double x2, double y2 )
{
  int done;

  // First, find all points in this range and remove them
  done = 0;
  while ( !done )
    {
    done = 1;

    this->Internal->FindNodeInRange.X1 = x1;
    this->Internal->FindNodeInRange.X2 = x2;

    std::vector<vtkPiecewiseFunctionNode*>::iterator iter =
      std::find_if(this->Internal->Nodes.begin(),
                      this->Internal->Nodes.end(),
                      this->Internal->FindNodeInRange );

    if ( iter != this->Internal->Nodes.end() )
      {
      delete *iter;
      this->Internal->Nodes.erase(iter);
      this->Modified();
      done = 0;
      }
    }

  // Now add the points
  this->AddPoint( x1, y1, 0.5, 0.0 );
  this->AddPoint( x2, y2, 0.5, 0.0 );
}

// Return the value of the function at a position
double vtkPiecewiseFunction::GetValue( double x )
{
  double table[1];
  this->GetTable( x, x, 1, table );
  return table[0];
}

// Remove all points outside the range, and make sure a point
// exists at each end of the range. Used as a convenience method
// for transfer function editors
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
  int done;

  done = 0;
  while ( !done )
    {
    done = 1;

    this->Internal->FindNodeOutOfRange.X1 = range[0];
    this->Internal->FindNodeOutOfRange.X2 = range[1];

    std::vector<vtkPiecewiseFunctionNode*>::iterator iter =
      std::find_if(this->Internal->Nodes.begin(),
                      this->Internal->Nodes.end(),
                      this->Internal->FindNodeOutOfRange );

    if ( iter != this->Internal->Nodes.end() )
      {
      delete *iter;
      this->Internal->Nodes.erase(iter);
      this->Modified();
      done = 0;
      }
    }

  this->SortAndUpdateRange();
  return 1;
}

// Returns a table of function values evaluated at regular intervals
void vtkPiecewiseFunction::GetTable( double xStart, double xEnd,
                                     int size, double* table,
                                     int stride )
{
  int i;
  int idx = 0;
  int numNodes = static_cast<int>(this->Internal->Nodes.size());

  // Need to keep track of the last value so that
  // we can fill in table locations past this with
  // this value if Clamping is On.
  double lastValue = 0.0;
  if ( numNodes != 0 )
    {
    lastValue = this->Internal->Nodes[numNodes-1]->Y;
    }

  double *tptr     = NULL;
  double x         = 0.0;
  double x1        = 0.0;
  double x2        = 0.0;
  double y1        = 0.0;
  double y2        = 0.0;
  double midpoint  = 0.0;
  double sharpness = 0.0;

  // For each table entry
  for ( i = 0; i < size; i++ )
    {
    // Find our location in the table
    tptr = table + stride*i;

    // Find our X location. If we are taking only 1 sample, make
    // it halfway between start and end (usually start and end will
    // be the same in this case)
    if ( size > 1 )
      {
      x = xStart + (double(i)/double(size-1))*(xEnd-xStart);
      }
    else
      {
      x = 0.5*(xStart+xEnd);
      }

    // Do we need to move to the next node?
    while ( idx < numNodes &&
            x > this->Internal->Nodes[idx]->X )
      {
      idx++;
      // If we are at a valid point index, fill in
      // the value at this node, and the one before (the
      // two that surround our current sample location)
      // idx cannot be 0 since we just incremented it.
      if ( idx < numNodes )
        {
        x1 = this->Internal->Nodes[idx-1]->X;
        x2 = this->Internal->Nodes[idx  ]->X;

        y1 = this->Internal->Nodes[idx-1]->Y;
        y2 = this->Internal->Nodes[idx  ]->Y;

        // We only need the previous midpoint and sharpness
        // since these control this region
        midpoint  = this->Internal->Nodes[idx-1]->Midpoint;
        sharpness = this->Internal->Nodes[idx-1]->Sharpness;

        // Move midpoint away from extreme ends of range to avoid
        // degenerate math
        if ( midpoint < 0.00001 )
          {
          midpoint = 0.00001;
          }

        if ( midpoint > 0.99999 )
          {
          midpoint = 0.99999;
          }
        }
      }

    // Are we at the end? If so, just use the last value
    if ( idx >= numNodes )
      {
      *tptr = (this->Clamping)?(lastValue):(0.0);
      }
    // Are we before the first node? If so, duplicate this nodes values
    else if ( idx == 0 )
      {
      *tptr = (this->Clamping)?(this->Internal->Nodes[0]->Y):(0.0);
      }
    // Otherwise, we are between two nodes - interpolate
    else
      {
      // Our first attempt at a normalized location [0,1] -
      // we will be modifying this based on midpoint and
      // sharpness to get the curve shape we want and to have
      // it pass through (y1+y2)/2 at the midpoint.
      double s = (x - x1) / (x2 - x1);

      // Readjust based on the midpoint - linear adjustment
      if ( s < midpoint )
        {
        s = 0.5 * s / midpoint;
        }
      else
        {
        s = 0.5 + 0.5*(s-midpoint)/(1.0-midpoint);
        }

      // override for sharpness > 0.99
      // In this case we just want piecewise constant
      if ( sharpness > 0.99 )
        {
        // Use the first value since we are below the midpoint
        if ( s < 0.5 )
          {
          *tptr = y1;
          continue;
          }
        // Use the second value at or above the midpoint
        else
          {
          *tptr = y2;
          continue;
          }
        }

      // Override for sharpness < 0.01
      // In this case we want piecewise linear
      if ( sharpness < 0.01 )
        {
        // Simple linear interpolation
        *tptr = (1-s)*y1 + s*y2;
        continue;
        }

      // We have a sharpness between [0.01, 0.99] - we will
      // used a modified hermite curve interpolation where we
      // derive the slope based on the sharpness, and we compress
      // the curve non-linearly based on the sharpness

      // First, we will adjust our position based on sharpness in
      // order to make the curve sharper (closer to piecewise constant)
      if ( s < .5 )
        {
        s = 0.5 * pow(s*2,1.0 + 10*sharpness);
        }
      else if ( s > .5 )
        {
        s = 1.0 - 0.5 * pow((1.0-s)*2,1+10*sharpness);
        }

      // Compute some coefficients we will need for the hermite curve
      double ss = s*s;
      double sss = ss*s;

      double h1 =  2*sss - 3*ss + 1;
      double h2 = -2*sss + 3*ss;
      double h3 =    sss - 2*ss + s;
      double h4 =    sss -   ss;

      double slope;
      double t;

      // Use one slope for both end points
      slope = y2 - y1;
      t = (1.0 - sharpness)*slope;

      // Compute the value
      *tptr = h1*y1 + h2*y2 + h3*t + h4*t;

      // Final error check to make sure we don't go outside
      // the Y range
      double min = (y1<y2)?(y1):(y2);
      double max = (y1>y2)?(y1):(y2);

      *tptr = (*tptr < min)?(min):(*tptr);
      *tptr = (*tptr > max)?(max):(*tptr);

      }
    }
}

// Copy from double table to float
void vtkPiecewiseFunction::GetTable( double xStart, double xEnd,
                                     int size, float* table,
                                     int stride )
{
  double *tmpTable = new double [size];

  this->GetTable( xStart, xEnd, size, tmpTable, 1 );

  double *tmpPtr = tmpTable;
  float *tPtr = table;

  for ( int i = 0; i < size; i++ )
    {
    *tPtr = static_cast<float>(*tmpPtr);
    tPtr   += stride;
    tmpPtr ++;
    }

  delete[] tmpTable;
}

// Given a table of values, build the piecewise function. Legacy method
// that does not allow for midpoint and sharpness control
void vtkPiecewiseFunction::BuildFunctionFromTable( double xStart, double xEnd,
                                                   int size, double* table,
                                                   int stride )
{
  double inc = 0.0;
  double *tptr = table;

  this->RemoveAllPoints();


  if( size > 1 )
    {
    inc = (xEnd-xStart)/static_cast<double>(size-1);
    }

  int i;
  for (i=0; i < size; i++)
    {
    vtkPiecewiseFunctionNode *node = new vtkPiecewiseFunctionNode;
    node->X    = xStart + inc*i;
    node->Y   = *tptr;
    node->Sharpness = 0.0;
    node->Midpoint  = 0.5;

    this->Internal->Nodes.push_back(node);
    tptr += stride;
    }

  this->SortAndUpdateRange();
}

// Given a pointer to an array of values, build the piecewise function.
// Legacy method that does not allow for midpoint and sharpness control
void vtkPiecewiseFunction::FillFromDataPointer(int nb, double *ptr)
{
  if (nb <= 0 || !ptr)
    {
    return;
    }

  this->RemoveAllPoints();

  double *inPtr = ptr;

  int i;
  for (i=0; i < nb; i++)
    {
    vtkPiecewiseFunctionNode *node = new vtkPiecewiseFunctionNode;
    node->X  = inPtr[0];
    node->Y  = inPtr[1];
    node->Sharpness = 0.0;
    node->Midpoint  = 0.5;

    this->Internal->Nodes.push_back(node);
    inPtr += 2;
    }

  this->SortAndUpdateRange();
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

  unsigned int i;

  os << indent << "Clamping: " << this->Clamping << endl;
  os << indent << "Range: [" << this->Range[0] << ","
     << this->Range[1] << "]" << endl;
  os << indent << "Function Points: " << this->Internal->Nodes.size() << endl;
  for( i = 0; i < this->Internal->Nodes.size(); i++ )
    {
    os << indent << "  " << i << " X: "
       << this->Internal->Nodes[i]->X << " Y: "
       << this->Internal->Nodes[i]->Y << " Sharpness: "
       << this->Internal->Nodes[i]->Sharpness << " Midpoint: "
       << this->Internal->Nodes[i]->Midpoint << endl;
    }
  os << indent << "AllowDuplicateScalars: " << this->AllowDuplicateScalars
     << endl;
}

