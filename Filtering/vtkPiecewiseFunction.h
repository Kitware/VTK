/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunction.h
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

// .NAME vtkPiecewiseFunction - Defines a 1D piecewise function.
// 
// .SECTION Description
// Defines a piecewise linear function mapping. Used for transfer functions
// in volume rendering.

#ifndef __vtkPiecewiseFunction_h
#define __vtkPiecewiseFunction_h

#include "vtkDataObject.h"

class VTK_EXPORT vtkPiecewiseFunction : public vtkDataObject
{
public:
  static vtkPiecewiseFunction *New();
  vtkTypeMacro(vtkPiecewiseFunction,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkDataObject *MakeObject();
  void Initialize();
  void DeepCopy( vtkDataObject *f );
  void ShallowCopy( vtkDataObject *f );

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_PIECEWISE_FUNCTION;};
  
  // Description:
  // Get the number of points used to specify the function
  int  GetSize();

  // Description:
  // Add/Remove points to/from the function. If a duplicate point is added
  // then the function value is changed at that location.
  void AddPoint( float x, float val );
  void RemovePoint( float x );

  // Description:
  // Removes all points from the function. 
  void RemoveAllPoints();

  // Description:
  // Add a line segment to the function. All points defined between the
  // two points specified are removed from the function.
  void AddSegment( float x1, float val1, float x2, float val2 );

  // Description:
  // Returns the value of the function at the specified location using
  // the specified interpolation. Returns zero if the specified location
  // is outside the min and max points of the function.
  float GetValue( float x );

  // Description:
  // Returns a pointer to the data stored in the table.
  float *GetDataPointer() {return this->Function;};

  // Description:
  // Returns the min and max point locations of the function.
  float *GetRange();

  // Description:
  // Fills in an array of function values evaluated at regular intervals.
  // Parameter "stride" is used to step through the output "table". It is
  // used by vtkColorTransferFunction to fill in an rgb table using three
  // separate piecewise functions and three separate calls to GetTable().
  void GetTable( float x1, float x2, int size, float *table, int stride=1 );

  // Description:
  // Constructs a piecewise function from a table.  Function range is
  // is set to [x1, x2], function size is set to size, and function points
  // are regularly spaced between x1 and x2.  Parameter "stride" is
  // is step through the input table.  It is used by vtkColorTransferFunction
  // to construct 3 piecewise functions from an rgb table.
  void BuildFunctionFromTable( float x1, float x2, int size,
			       float *table, int stride=1 );
  
  // Description:
  // When zero range clamping is Off, GetValue() returns 0.0 when a
  // value is requested outside of the points specified.
  // When zero range clamping is On, GetValue() returns the value at
  // the value at the lowest point for a request below all points 
  // specified and returns the value at the highest point for a request 
  // above all points specified. On is the default.
  vtkSetMacro( Clamping, int );
  vtkGetMacro( Clamping, int );
  vtkBooleanMacro( Clamping, int );

  // Description:
  // Return the type of function:
  // Function Types:
  //    0 : Constant        (No change in slope between end points)
  //    1 : NonDecreasing   (Always increasing or zero slope)
  //    2 : NonIncreasing   (Always decreasing or zero slope)
  //    3 : Varied          (Contains both decreasing and increasing slopes)
  const char  *GetType();

  // Description:
  // Get the mtime of this object - override to consider the
  // mtime of the source as well.
  unsigned long GetMTime();

  // Description:
  // Returns the first point location which precedes a non-zero segment of the
  // function. Note that the value at this point may be zero.
  float GetFirstNonZeroValue();

protected:
  vtkPiecewiseFunction();
  ~vtkPiecewiseFunction();
  vtkPiecewiseFunction(const vtkPiecewiseFunction&);
  void operator=(const vtkPiecewiseFunction&);

  // Size of the array used to store function points
  int	ArraySize;

  // Determines the function value outside of defined points
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  int	Clamping;

  // Array of points ((X,Y) pairs)
  float	*Function;

  // Number of points used to specify function
  int   FunctionSize;

  // Min and max range of function point locations
  float FunctionRange[2];

  // Increases size of the function array. The array grows by a factor of 2
  // when the array limit has been reached.
  void IncreaseArraySize();

  // Private function to add a point to the function. Returns the array
  // index of the inserted point.
  int InsertPoint( float x, float val );

  // Move points one index down or up in the array. This is useful for 
  // inserting and deleting points into the array.
  void MovePoints( int index, int down );
};

#endif

