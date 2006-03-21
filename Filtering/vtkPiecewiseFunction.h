/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPiecewiseFunction - Defines a 1D piecewise function.
// 
// .SECTION Description
// Defines a piecewise linear function mapping. Used for transfer functions
// in volume rendering.

#ifndef __vtkPiecewiseFunction_h
#define __vtkPiecewiseFunction_h

#include "vtkDataObject.h"

class VTK_FILTERING_EXPORT vtkPiecewiseFunction : public vtkDataObject
{
public:
  static vtkPiecewiseFunction *New();
  vtkTypeRevisionMacro(vtkPiecewiseFunction,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Return the index of the point (0 based), or -1 on error.
  int AddPoint( double x, double val );
  int RemovePoint( double x );

  // Description:
  // Removes all points from the function. 
  void RemoveAllPoints();

  // Description:
  // Add a line segment to the function. All points defined between the
  // two points specified are removed from the function.
  void AddSegment( double x1, double val1, double x2, double val2 );

  // Description:
  // Returns the value of the function at the specified location using
  // the specified interpolation. Returns zero if the specified location
  // is outside the min and max points of the function.
  double GetValue( double x );

  // Description:
  // Returns a pointer to the data stored in the table.
  // Fills from a pointer to data stored in a similar table.
  double *GetDataPointer() {return this->Function;};
  void FillFromDataPointer(int, double*);

  // Description:
  // Returns the min and max point locations of the function.
  double *GetRange();

  // Description:
  // Remove all points out of the new range, and make sure there is a point
  // at each end of that range.
  // Return 1 on success, 0 otherwise.
  int AdjustRange(double range[2]);

  // Description:
  // Fills in an array of function values evaluated at regular intervals.
  // Parameter "stride" is used to step through the output "table". It is
  // used by vtkColorTransferFunction to fill in an rgb table using three
  // separate piecewise functions and three separate calls to GetTable().
  void GetTable( double x1, double x2, int size, float *table, int stride=1 );
  void GetTable( double x1, double x2, int size, double *table, int stride=1 );

  // Description:
  // Constructs a piecewise function from a table.  Function range is
  // is set to [x1, x2], function size is set to size, and function points
  // are regularly spaced between x1 and x2.  Parameter "stride" is
  // is step through the input table.  It is used by vtkColorTransferFunction
  // to construct 3 piecewise functions from an rgb table.
  void BuildFunctionFromTable( double x1, double x2, int size,
                               double *table, int stride=1 );
  
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
  // Returns the first point location which precedes a non-zero segment of the
  // function. Note that the value at this point may be zero.
  double GetFirstNonZeroValue();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkPiecewiseFunction* GetData(vtkInformation* info);
  static vtkPiecewiseFunction* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkPiecewiseFunction();
  ~vtkPiecewiseFunction();

  // Size of the array used to store function points
  int   ArraySize;

  // Determines the function value outside of defined points
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  int   Clamping;

  // Array of points ((X,Y) pairs)
  double *Function;

  // Number of points used to specify function
  int   FunctionSize;

  // Min and max range of function point locations
  double FunctionRange[2];

  // Increases size of the function array. The array grows by a factor of 2
  // when the array limit has been reached.
  void IncreaseArraySize();

  // Private function to add a point to the function. Returns the array
  // index of the inserted point.
  int InsertPoint( double x, double val );

  // Move points one index down or up in the array. This is useful for 
  // inserting and deleting points into the array.
  void MovePoints( int index, int down );
private:
  vtkPiecewiseFunction(const vtkPiecewiseFunction&);  // Not implemented.
  void operator=(const vtkPiecewiseFunction&);  // Not implemented.
};

#endif

