
// .NAME vtkPiecewiseFunction - Defines a 1D piecewise function.
// 
// .SECTION Description
// 

// .SECTION see also
//

#ifndef __vtkPiecewiseFunction_h
#define __vtkPiecewiseFunction_h

#include "vtkObject.h"
//#include "vtkInterpolator.h"

class VTK_EXPORT vtkPiecewiseFunction : public vtkObject 
{
public:
  vtkPiecewiseFunction();
  ~vtkPiecewiseFunction();
  static vtkPiecewiseFunction *New() {return new vtkPiecewiseFunction;};
  char *GetClassName() {return "vtkPiecewiseFunction";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the number of points used to specify the function
  int  GetSize();

  //  This we will add when we go beyond linear interpolation
  //  vtkSetObjectMacro( Interpolator, vtkInterpolator );
  //  vtkGetObjectMacro( Interpolator, vtkInterpolator );

  // Description:
  // Add/Remove points to/from the function. If a duplicate point is added
  // then the function value is changed at that location.
  void AddPoint( float x, float val );
  void RemovePoint( float x );

  // Description:
  // Removes all points from the function. 
  void RemoveAllPoints();

  // Description:
  /// Add a line segment to the function. All points defined between the
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
  // Fills in an array of function values evaluated at regular intervals
  void GetTable( float x1, float x2, int size, float *table );

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
  char  *GetType();

  // Description:
  // Returns the first point location which precedes a non-zero segment of the
  // function. Note that the value at this point may be zero.
  float GetFirstNonZeroValue();

protected:

  // Size of the array used to store function points
  int	ArraySize;

  // Determines the function value outside of defined points
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  int	Clamping;

  // If no interpolator defined, defaults to nearest neighbor
  //  vtkInterpolator	*Interpolator;

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


