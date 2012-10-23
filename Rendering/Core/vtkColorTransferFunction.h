/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkColorTransferFunction - Defines a transfer function for mapping a property to an RGB color value.

// .SECTION Description
// vtkColorTransferFunction is a color mapping in RGB or HSV space that
// uses piecewise hermite functions to allow interpolation that can be
// piecewise constant, piecewise linear, or somewhere in-between
// (a modified piecewise hermite function that squishes the function
// according to a sharpness parameter). The function also allows for
// the specification of the midpoint (the place where the function
// reaches the average of the two bounding nodes) as a normalize distance
// between nodes.
// See the description of class vtkPiecewiseFunction for an explanation of
// midpoint and sharpness.

// .SECTION see also
// vtkPiecewiseFunction

#ifndef __vtkColorTransferFunction_h
#define __vtkColorTransferFunction_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkScalarsToColors.h"

class vtkColorTransferFunctionInternals;

#define VTK_CTF_RGB           0
#define VTK_CTF_HSV           1
#define VTK_CTF_LAB           2
#define VTK_CTF_DIVERGING     3

#define VTK_CTF_LINEAR        0
#define VTK_CTF_LOG10         1

class VTKRENDERINGCORE_EXPORT vtkColorTransferFunction : public vtkScalarsToColors
{
public:
  static vtkColorTransferFunction *New();
  vtkTypeMacro(vtkColorTransferFunction,vtkScalarsToColors);
  void DeepCopy( vtkScalarsToColors *f );
  void ShallowCopy( vtkColorTransferFunction *f );

  // Description:
  // Print method for vtkColorTransferFunction
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // How many points are there defining this function?
  int GetSize();

  // Description:
  // Add/Remove a point to/from the function defined in RGB or HSV
  // Return the index of the point (0 based), or -1 on error.
  // See the description of class vtkPiecewiseFunction for an explanation of
  // midpoint and sharpness.
  int AddRGBPoint( double x, double r, double g, double b );
  int AddRGBPoint( double x, double r, double g, double b,
                   double midpoint, double sharpness );
  int AddHSVPoint( double x, double h, double s, double v );
  int AddHSVPoint( double x, double h, double s, double v,
                   double midpoint, double sharpness );
  int RemovePoint( double x );

  // Description:
  // Add two points to the function and remove all the points
  // between them
  void AddRGBSegment( double x1, double r1, double g1, double b1,
                      double x2, double r2, double g2, double b2 );
  void AddHSVSegment( double x1, double h1, double s1, double v1,
                      double x2, double h2, double s2, double v2 );

  // Description:
  // Remove all points
  void RemoveAllPoints();

  // Description:
  // Returns an RGB color for the specified scalar value
  double *GetColor(double x) {
    return vtkScalarsToColors::GetColor(x); }
  void GetColor(double x, double rgb[3]);

  // Description:
  // Get the color components individually.
  double GetRedValue( double x );
  double GetGreenValue( double x );
  double GetBlueValue( double x );

  // Description:
  // For the node specified by index, set/get the
  // location (X), R, G, and B values, midpoint, and
  // sharpness values at the node.
  int GetNodeValue( int index, double val[6] );
  int SetNodeValue( int index, double val[6] );

  // Description:
  // Map one value through the lookup table.
  virtual unsigned char *MapValue(double v);

  // Description:
  // Returns min and max position of all function points.
  vtkGetVector2Macro( Range, double );

  // Description:
  // Remove all points out of the new range, and make sure there is a point
  // at each end of that range.
  // Return 1 on success, 0 otherwise.
  int AdjustRange(double range[2]);

  // Description:
  // Fills in a table of n function values between x1 and x2.
  //
  // Note that \a GetTable ignores \a IndexedLookup
  void GetTable( double x1, double x2, int n, double* table );
  void GetTable( double x1, double x2, int n, float* table );
  const unsigned char *GetTable( double x1, double x2, int n);

  // Description:
  // Construct a color transfer function from a table. Function range is
  // is set to [x1, x2], each function size is set to size, and function
  // points are regularly spaced between x1 and x2. Parameter "table" is
  // assumed to be a block of memory of size [3*size]
  void BuildFunctionFromTable( double x1, double x2, int size, double *table);

  // Description:
  // Sets and gets the clamping value for this transfer function.
  vtkSetClampMacro( Clamping, int, 0, 1 );
  vtkGetMacro( Clamping, int );
  vtkBooleanMacro( Clamping, int );

  // Description:
  // Set/Get the color space used for interpolation: RGB, HSV, CIELAB, or
  // Diverging.  In HSV mode, if HSVWrap is on, it will take the shortest path
  // in Hue (going back through 0 if that is the shortest way around the hue
  // circle) whereas if HSVWrap is off it will not go through 0 (in order the
  // match the current functionality of vtkLookupTable).  Diverging is a special
  // mode where colors will pass through white when interpolating between two
  // saturated colors.
  vtkSetClampMacro( ColorSpace, int, VTK_CTF_RGB, VTK_CTF_DIVERGING );
  void SetColorSpaceToRGB(){this->SetColorSpace(VTK_CTF_RGB);};
  void SetColorSpaceToHSV(){this->SetColorSpace(VTK_CTF_HSV);};
  void SetColorSpaceToLab(){this->SetColorSpace(VTK_CTF_LAB);};
  void SetColorSpaceToDiverging(){this->SetColorSpace(VTK_CTF_DIVERGING);}
  vtkGetMacro( ColorSpace, int );
  vtkSetMacro(HSVWrap, int);
  vtkGetMacro(HSVWrap, int);
  vtkBooleanMacro(HSVWrap, int);

  // Description:
  // Set the type of scale to use, linear or logarithmic.  The default
  // is linear.  If the scale is logarithmic, and the range contains
  // zero, the color mapping will be linear.
  vtkSetMacro(Scale,int);
  void SetScaleToLinear() { this->SetScale(VTK_CTF_LINEAR); };
  void SetScaleToLog10() { this->SetScale(VTK_CTF_LOG10); };
  vtkGetMacro(Scale,int);

  // Description:
  // Set the color to use when a NaN (not a number) is encountered.  This is an
  // RGB 3-tuple color of doubles in the range [0,1].
  vtkSetVector3Macro(NanColor, double);
  vtkGetVector3Macro(NanColor, double);

  // Description:
  // Returns a list of all nodes
  // Fills from a pointer to data stored in a similar list of nodes.
  double *GetDataPointer();
  void FillFromDataPointer(int, double*);

  // Description:
  // map a set of scalars through the lookup table
  virtual void MapScalarsThroughTable2(void *input, unsigned char *output,
                                     int inputDataType, int numberOfValues,
                                     int inputIncrement, int outputIncrement);

  // Description:
  // Toggle whether to allow duplicate scalar values in the color transfer
  // function (off by default).
  vtkSetMacro(AllowDuplicateScalars, int);
  vtkGetMacro(AllowDuplicateScalars, int);
  vtkBooleanMacro(AllowDuplicateScalars, int);

  // Description:
  // Get the number of available colors for mapping to.
  virtual vtkIdType GetNumberOfAvailableColors();

protected:
  vtkColorTransferFunction();
  ~vtkColorTransferFunction();

  vtkColorTransferFunctionInternals *Internal;

  // Determines the function value outside of defined points
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  int Clamping;

  // The color space in which interpolation is performed
  int ColorSpace;

  // Specify if HSW is warp or not
  int HSVWrap;

  // The color interpolation scale (linear or logarithmic).
  int Scale;

  // The color to use for not-a-number.
  double NanColor[3];

  double     *Function;

  // The min and max node locations
  double Range[2];

  // An evaluated color (0 to 255 RGBA A=255)
  unsigned char UnsignedCharRGBAValue[4];

  int AllowDuplicateScalars;

  vtkTimeStamp BuildTime;
  unsigned char *Table;
  int TableSize;

  // Description:
  // Set the range of scalars being mapped. The set has no functionality
  // in this subclass of vtkScalarsToColors.
  virtual void SetRange(double, double) {};
  void SetRange(double rng[2]) {this->SetRange(rng[0],rng[1]);};

  // Internal method to sort the vector and update the
  // Range whenever a node is added, edited or removed
  // It always calls Modified().
  void SortAndUpdateRange();
  // Returns true if the range has been updated and Modified() has been called
  bool UpdateRange();

  // Description:
  // Moves point from oldX to newX. It removed the point from oldX. If any point
  // existed at newX, it will also be removed.
  void MovePoint(double oldX, double newX);

private:
  vtkColorTransferFunction(const vtkColorTransferFunction&);  // Not implemented.
  void operator=(const vtkColorTransferFunction&);  // Not implemented.
};

#endif

