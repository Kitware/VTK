// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPiecewiseFunction
 * @brief   Defines a 1D piecewise function.
 *
 *
 * Defines a piecewise function mapping. This mapping allows the addition
 * of control points, and allows the user to control the function between
 * the control points. A piecewise hermite curve is used between control
 * points, based on the sharpness and midpoint parameters. A sharpness of
 * 0 yields a piecewise linear function and a sharpness of 1 yields a
 * piecewise constant function. The midpoint is the normalized distance
 * between control points at which the curve reaches the median Y value.
 * The midpoint and sharpness values specified when adding a node are used
 * to control the transition to the next node (the last node's values are
 * ignored) Outside the range of nodes, the values are 0 if Clamping is off,
 * or the nearest node point if Clamping is on. Using the legacy methods for
 * adding points  (which do not have Sharpness and Midpoint parameters)
 * will default to Midpoint = 0.5 (halfway between the control points) and
 * Sharpness = 0.0 (linear).
 */

#ifndef vtkPiecewiseFunction_h
#define vtkPiecewiseFunction_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunctionInternals;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkPiecewiseFunction : public vtkDataObject
{
public:
  static vtkPiecewiseFunction* New();
  vtkTypeMacro(vtkPiecewiseFunction, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void DeepCopy(vtkDataObject* f) override;
  void ShallowCopy(vtkDataObject* f) override;

  enum SearchMethod
  {
    BINARY_SEARCH = 0,
    INTERPOLATION_SEARCH = 1,
    MAX_ENUM = 2
  };

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_PIECEWISE_FUNCTION; }

  /**
   * Get the number of points used to specify the function
   */
  int GetSize();

  ///@{
  /**
   * Add points to the function. If a duplicate point is added
   * then the previous point is removed unless
   * AllowDuplicateScalars is set to true
   * Return the index of the point (0 based), or -1 on error.
   */
  int AddPoint(double x, double y);
  int AddPoint(double x, double y, double midpoint, double sharpness);
  ///@}

  /**
   * Remove a point from the function at a given id
   * Return true if point has been found and removed, false other wise
   */
  bool RemovePointByIndex(size_t id);

  /**
   * Remove the first point found at the given x location
   * Return the index of the remove point if any, -1 otherwise
   */
  int RemovePoint(double x);

  /**
   * Remove the first point found at the given x and y location
   * Return the index of the remove point if any, -1 otherwise
   */
  int RemovePoint(double x, double y);

  /**
   * Removes all points from the function.
   */
  void RemoveAllPoints();

  /**
   * Add a line segment to the function. All points defined between the
   * two points specified are removed from the function.
   * To specify the sharpness and midpoint values, use AddPoint method instead.
   */
  void AddSegment(double x1, double y1, double x2, double y2);

  /**
   * Returns the value of the function at the specified location using
   * the specified interpolation.
   */
  double GetValue(double x);

  ///@{
  /**
   * For the node specified by index, set/get the
   * location (X), value (Y), midpoint, and sharpness
   * values at the node. Returns -1 if the index is
   * out of range, returns 1 otherwise.
   */
  int GetNodeValue(int index, double val[4]);
  int SetNodeValue(int index, double val[4]);
  ///@}

  ///@{
  /**
   * Returns a pointer to the data stored in the table.
   * Fills from a pointer to data stored in a similar table. These are
   * legacy methods which will be maintained for compatibility - however,
   * note that the vtkPiecewiseFunction no longer stores the nodes
   * in a double array internally.
   */
  double* GetDataPointer();
  void FillFromDataPointer(int, double*);
  ///@}

  ///@{
  /**
   * Returns the min and max node locations of the function.
   */
  vtkGetVector2Macro(Range, double);
  ///@}

  /**
   * Remove all points out of the new range, and make sure there is a point
   * at each end of that range.
   * Return 1 on success, 0 otherwise.
   */
  int AdjustRange(double range[2]);

  ///@{
  /**
   * Fills in an array of function values evaluated at regular intervals.
   * Parameter "stride" is used to step through the output "table". If
   * logIncrements is true, the intervals between entries will be constant in
   * logarithmic space. epsilon is used to move midpoint away from extreme ends
   * of range, it should be changed if the values are the same magnitude of
   * the default epsilon.
   */
  void GetTable(double x1, double x2, int size, float* table, int stride = 1, int logIncrements = 0,
    double epsilon = 1e-5);
  void GetTable(double x1, double x2, int size, double* table, int stride = 1,
    int logIncrements = 0, double epsilon = 1e-5);
  ///@}

  /**
   * Constructs a piecewise function from a table.  Function range is
   * is set to [x1, x2], function size is set to size, and function points
   * are regularly spaced between x1 and x2.  Parameter "stride" is
   * is step through the input table.
   */
  void BuildFunctionFromTable(double x1, double x2, int size, double* table, int stride = 1);

  ///@{
  /**
   * When zero range clamping is Off, GetValue() returns 0.0 when a
   * value is requested outside of the points specified.
   * When zero range clamping is On, GetValue() returns the value at
   * the value at the lowest point for a request below all points
   * specified and returns the value at the highest point for a request
   * above all points specified. On is the default.
   */
  vtkSetMacro(Clamping, vtkTypeBool);
  vtkGetMacro(Clamping, vtkTypeBool);
  vtkBooleanMacro(Clamping, vtkTypeBool);
  ///@}

  /**
   * Interpolate between the control points in base-10 logrithmic space.
   * Default is false.
   * @{
   */
  vtkSetMacro(UseLogScale, bool);
  vtkGetMacro(UseLogScale, bool);
  vtkBooleanMacro(UseLogScale, bool);
  /**@}*/

  /**
   * Return the type of function:
   * Function Types:
   * 0 : Constant        (No change in slope between end points)
   * 1 : NonDecreasing   (Always increasing or zero slope)
   * 2 : NonIncreasing   (Always decreasing or zero slope)
   * 3 : Varied          (Contains both decreasing and increasing slopes)
   */
  const char* GetType();

  /**
   * Returns the first point location which precedes a non-zero segment of the
   * function. Note that the value at this point may be zero.
   */
  double GetFirstNonZeroValue();

  /**
   * Clears out the current function. A newly created vtkPiecewiseFunction
   * is already initialized, so there is no need to call this method which
   * in turn simply calls RemoveAllPoints()
   */
  void Initialize() override;

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPiecewiseFunction* GetData(vtkInformation* info);
  static vtkPiecewiseFunction* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  ///@{
  /**
   * Toggle whether to allow duplicate scalar values in the piecewise
   * function (off by default).
   */
  vtkSetMacro(AllowDuplicateScalars, vtkTypeBool);
  vtkGetMacro(AllowDuplicateScalars, vtkTypeBool);
  vtkBooleanMacro(AllowDuplicateScalars, vtkTypeBool);
  ///@}

  /**
   * Estimates the minimum size of a table such that it would correctly sample this function.
   * The returned value should be passed as parameter 'n' when calling GetTable().
   */
  int EstimateMinNumberOfSamples(double const& x1, double const& x2);

  /**
   * Analyses the point distribution and automatically
   * updates the search method to optimize the time processing
   * This method assumes that the vector of nodes has been sorted
   */
  void UpdateSearchMethod(double epsilon = 1e-12, double thresh = 1e-4);

  //@{
  /**
   * Methods to set / get the search method used.
   * By default the search method used is the one automatically updated
   * each time the data is modified.
   * This behavior can be overridden by using SetUseCustomSearchMethod() and SetCustomSearchMethod()
   */
  int GetAutomaticSearchMethod();
  void SetUseCustomSearchMethod(bool use);
  void SetCustomSearchMethod(int type);
  int GetCustomSearchMethod();
  //@}

protected:
  vtkPiecewiseFunction();
  ~vtkPiecewiseFunction() override;

  /**
   * Internal methods to sort the vector and update the
   * Range whenever a node is added, edited or removed.
   * It always calls Modified().
   *
   * By default it updates search method with UpdateSearchMethod()
   */
  void SortAndUpdateRange(bool updateSearchMethod = true);

  /**
   * Returns true if the range has been updated and Modified() has been called
   */
  bool UpdateRange();

  /**
   * Traverses the nodes to find the minimum distance. Assumes nodes are sorted.
   */
  double FindMinimumXDistance();

  // The internal STL structures
  vtkPiecewiseFunctionInternals* Internal;

  // Determines the function value outside of defined points
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  vtkTypeBool Clamping;

  // Array of points ((X,Y) pairs)
  double* Function;

  // Min and max range of function point locations
  double Range[2];

  vtkTypeBool AllowDuplicateScalars;

  bool UseLogScale;

private:
  vtkPiecewiseFunction(const vtkPiecewiseFunction&) = delete;
  void operator=(const vtkPiecewiseFunction&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
