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
/**
 * @class   vtkColorTransferFunction
 * @brief   Defines a transfer function for mapping a
 * property to an RGB color value.
 *
 *
 * vtkColorTransferFunction is a color mapping in RGB or HSV space that
 * uses piecewise hermite functions to allow interpolation that can be
 * piecewise constant, piecewise linear, or somewhere in-between
 * (a modified piecewise hermite function that squishes the function
 * according to a sharpness parameter). The function also allows for
 * the specification of the midpoint (the place where the function
 * reaches the average of the two bounding nodes) as a normalize distance
 * between nodes.
 * See the description of class vtkPiecewiseFunction for an explanation of
 * midpoint and sharpness.
 *
 * @sa
 * vtkPiecewiseFunction
*/

#ifndef vtkColorTransferFunction_h
#define vtkColorTransferFunction_h

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

  /**
   * Print method for vtkColorTransferFunction
   */
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * How many nodes define this function?
   */
  int GetSize();

  //@{
  /**
   * Add/Remove a point to/from the function defined in RGB or HSV
   * Return the index of the point (0 based), or -1 on error.
   * See the description of class vtkPiecewiseFunction for an explanation of
   * midpoint and sharpness.
   */
  int AddRGBPoint( double x, double r, double g, double b );
  int AddRGBPoint( double x, double r, double g, double b,
                   double midpoint, double sharpness );
  int AddHSVPoint( double x, double h, double s, double v );
  int AddHSVPoint( double x, double h, double s, double v,
                   double midpoint, double sharpness );
  int RemovePoint( double x );
  //@}

  //@{
  /**
   * Add two points to the function and remove all the points
   * between them
   */
  void AddRGBSegment( double x1, double r1, double g1, double b1,
                      double x2, double r2, double g2, double b2 );
  void AddHSVSegment( double x1, double h1, double s1, double v1,
                      double x2, double h2, double s2, double v2 );
  //@}

  /**
   * Remove all points
   */
  void RemoveAllPoints();

  /**
   * Returns an RGB color for the specified scalar value
   */
  double *GetColor(double x) {
    return vtkScalarsToColors::GetColor(x); }
  void GetColor(double x, double rgb[3]);

  //@{
  /**
   * Get the color components individually.
   */
  double GetRedValue( double x );
  double GetGreenValue( double x );
  double GetBlueValue( double x );
  //@}

  //@{
  /**
   * For the node specified by index, set/get the
   * location (X), R, G, and B values, midpoint, and
   * sharpness values at the node.
   */
  int GetNodeValue( int index, double val[6] );
  int SetNodeValue( int index, double val[6] );
  //@}

  /**
   * Map one value through the lookup table.
   */
  virtual unsigned char *MapValue(double v);

  //@{
  /**
   * Returns min and max position of all function points.
   */
  vtkGetVector2Macro( Range, double );
  //@}

  /**
   * Remove all points out of the new range, and make sure there is a point
   * at each end of that range.
   * Returns 1 on success, 0 otherwise.
   */
  int AdjustRange(double range[2]);

  //@{
  /**
   * Fills in a table of \a n colors mapped from \a values mapped with
   * even spacing between x1 and x2, inclusive.

   * Note that \a GetTable ignores \a IndexedLookup
   */
  void GetTable( double x1, double x2, int n, double* table );
  void GetTable( double x1, double x2, int n, float* table );
  const unsigned char *GetTable( double x1, double x2, int n );
  //@}

  /**
   * Construct a color transfer function from a table. Unlike
   * FillFromDataPointer(), the \p table parameter's layout is assumed
   * to be [R1, G1, B1, R2, G2, B2, ..., Rn, Gn, Bn], and it is
   * assumed to be a block of memory of size [3*size]. After calling
   * this method, the function range will be [x1, x2], the function
   * will have \p size nodes, and function values will be regularly spaced
   * between x1 and x2.
   */
  void BuildFunctionFromTable( double x1, double x2, int size, double *table );

  //@{
  /**
   * Sets/gets whether clamping is used. If on, scalar values below
   * the lower range value set for the transfer function will be
   * mapped to the first node color, and scalar values above the upper
   * range value set for the transfer function will be mapped to the
   * last node color. If off, values outside the range are mapped to
   * black.
   */
  vtkSetClampMacro( Clamping, int, 0, 1 );
  vtkGetMacro( Clamping, int );
  vtkBooleanMacro( Clamping, int );
  //@}

  //@{
  /**
   * Set/Get the color space used for interpolation: RGB, HSV, CIELAB, or
   * Diverging.  In HSV mode, if HSVWrap is on, it will take the shortest path
   * in Hue (going back through 0 if that is the shortest way around the hue
   * circle) whereas if HSVWrap is off it will not go through 0 (in order the
   * match the current functionality of vtkLookupTable).  Diverging is a special
   * mode where colors will pass through white when interpolating between two
   * saturated colors.
   */
  vtkSetClampMacro( ColorSpace, int, VTK_CTF_RGB, VTK_CTF_DIVERGING );
  void SetColorSpaceToRGB(){this->SetColorSpace(VTK_CTF_RGB);};
  void SetColorSpaceToHSV(){this->SetColorSpace(VTK_CTF_HSV);};
  void SetColorSpaceToLab(){this->SetColorSpace(VTK_CTF_LAB);};
  void SetColorSpaceToDiverging(){this->SetColorSpace(VTK_CTF_DIVERGING);}
  vtkGetMacro( ColorSpace, int );
  vtkSetMacro(HSVWrap, int);
  vtkGetMacro(HSVWrap, int);
  vtkBooleanMacro(HSVWrap, int);
  //@}

  //@{
  /**
   * Set the type of scale to use, linear or logarithmic.  The default
   * is linear.  If the scale is logarithmic, and the range contains
   * zero, the color mapping will be linear.
   */
  vtkSetMacro(Scale,int);
  void SetScaleToLinear() { this->SetScale(VTK_CTF_LINEAR); };
  void SetScaleToLog10() { this->SetScale(VTK_CTF_LOG10); };
  vtkGetMacro(Scale,int);
  //@}

  //@{
  /**
   * Set the RGB color to use when a NaN (not a number) is
   * encountered.  This is an RGB 3-tuple color of doubles in the
   * range [0,1].
   */
  vtkSetVector3Macro(NanColor, double);
  vtkGetVector3Macro(NanColor, double);
  //@}

  //@{
  /**
   * Set the color to use when a value below the range is
   * encountered. This is an RGB 3-tuple of doubles in the range [0, 1].
   */
  vtkSetVector3Macro(BelowRangeColor, double);
  vtkGetVector3Macro(BelowRangeColor, double);
  //@}

  //@{
  /**
   * Set whether the below range color should be used.
   */
  vtkSetMacro(UseBelowRangeColor, int);
  vtkGetMacro(UseBelowRangeColor, int);
  vtkBooleanMacro(UseBelowRangeColor, int);
  //@}

  //@{
  /**
   * Set the color to use when a value above the range is
   * encountered. This is an RGB 3-tuple of doubles in the range [0, 1].
   */
  vtkSetVector3Macro(AboveRangeColor, double);
  vtkGetVector3Macro(AboveRangeColor, double);
  //@}

  //@{
  /**
   * Set whether the below range color should be used.
   */
  vtkSetMacro(UseAboveRangeColor, int);
  vtkGetMacro(UseAboveRangeColor, int);
  vtkBooleanMacro(UseAboveRangeColor, int);
  //@}

  /**
   * Returns a pointer to an array of all node values in an
   * interleaved array with the layout [X1, R1, G1, B1, X2, R2, G2,
   * B2, ..., Xn, Rn, Gn, Bn] where n is the number of nodes defining
   * the transfer function. The returned pointer points to an array
   * that is managed by this class, so callers should not free it.
   */
  double* GetDataPointer();

  /**
   * Defines the nodes from an array \a ptr with the layout [X1, R1,
   * G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
   * number of nodes.
   */
  void FillFromDataPointer(int n, double* ptr);

  /**
   * Map a set of scalars through the lookup table.
   */
  virtual void MapScalarsThroughTable2(void *input, unsigned char *output,
                                       int inputDataType, int numberOfValues,
                                       int inputIncrement, int outputIncrement);

  //@{
  /**
   * Toggle whether to allow duplicate scalar values in the color transfer
   * function (off by default).
   */
  vtkSetMacro(AllowDuplicateScalars, int);
  vtkGetMacro(AllowDuplicateScalars, int);
  vtkBooleanMacro(AllowDuplicateScalars, int);
  //@}

  /**
   * Get the number of available colors for mapping to.
   */
  virtual vtkIdType GetNumberOfAvailableColors();

  /**
   * Return a color given an integer index.

   * This is used to assign colors to annotations (given an offset into the list of annotations).
   * If there are no control points or \a idx < 0, then NanColor is returned.
   */
  virtual void GetIndexedColor(vtkIdType idx, double rgba[4]);

  /**
   * Estimates the minimum size of a table such that it would correctly sample this function.
   * The returned value should be passed as parameter 'n' when calling GetTable().
   */
  int EstimateMinNumberOfSamples(double const & x1, double const & x2);

protected:
  vtkColorTransferFunction();
  ~vtkColorTransferFunction();

  vtkColorTransferFunctionInternals *Internal;

  /**
   * Determines the function value outside of defined points
   * Zero = always return 0.0 outside of defined points
   * One  = clamp to the lowest value below defined points and
   * highest value above defined points
   */
  int Clamping;

  /**
   * The color space in which interpolation is performed.
   */
  int ColorSpace;

  /**
   * Specify if HSV is wrap or not
   */
  int HSVWrap;

  /**
   * The color interpolation scale (linear or logarithmic).
   */
  int Scale;

  /**
   * The color to use for not-a-number.
   */
  double NanColor[3];

  /**
   * The below-range color.
   */
  double BelowRangeColor[3];

  /**
   * Flag indicating whether below-range color should be used.
   */
  int UseBelowRangeColor;

  /**
   * The above-range color.
   */
  double AboveRangeColor[3];

  /**
   * Flag indicating whether below-range color should be used.
   */
  int UseAboveRangeColor;

  /**
   * Temporary array to store data from the nodes.
   */
  double* Function;

  /**
   * The min and max node locations
   */
  double Range[2];

  /**
   * Temporary storage for an evaluated color (0 to 255 RGBA A=255)
   */
  unsigned char UnsignedCharRGBAValue[4];

  /**
   * If on, the same scalar value may have more than one node assigned to it.
   */
  int AllowDuplicateScalars;

  vtkTimeStamp BuildTime;
  unsigned char *Table;

  /**
   * Temporary storage for the size of the table. Set in the method GetTable()
   * and queried in GetNumberOfAvailableColors().
   */
  int TableSize;

  /**
   * Set the range of scalars being mapped. This method has no functionality
   * in this subclass of vtkScalarsToColors.
   */
  virtual void SetRange(double, double) {}
  void SetRange(double rng[2]) {this->SetRange(rng[0],rng[1]);};

  /**
   * Internal method to sort the vector and update the
   * Range whenever a node is added, edited or removed
   * It always calls Modified().
   */
  void SortAndUpdateRange();

  /**
   * Returns true if the range has been changed. If the ranged has
   * been modified, calls Modified().
   */
  bool UpdateRange();

  /**
   * Moves point from oldX to newX. It removed the point from oldX. If
   * any point existed at newX, it will also be removed.
   */
  void MovePoint(double oldX, double newX);

  /**
   * Traverses the nodes to find the minimum distance. Assumes nodes are sorted.
   */
  double FindMinimumXDistance();

private:
  vtkColorTransferFunction(const vtkColorTransferFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkColorTransferFunction&) VTK_DELETE_FUNCTION;
};

#endif
