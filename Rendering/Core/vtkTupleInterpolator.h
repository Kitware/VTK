// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTupleInterpolator
 * @brief   interpolate a tuple of arbitrary size
 *
 * This class is used to interpolate a tuple which may have an arbitrary
 * number of components (but at least one component). The interpolation may
 * be linear in form, or via a subclasses of vtkSpline.
 *
 * To use this class, begin by specifying the number of components of the
 * tuple and the interpolation function to use. Then specify at least one
 * pair of (t,tuple) with the AddTuple() method.  Next interpolate the
 * tuples with the InterpolateTuple(t,tuple) method, where "t" must be in the
 * range of (t_min,t_max) parameter values specified by the AddTuple() method
 * (if not then t is clamped), and tuple[] is filled in by the method (make
 * sure that tuple [] is long enough to hold the interpolated data).
 *
 * You can control the type of interpolation to use. By default, the
 * interpolation is based on a Kochanek spline. However, other types of
 * splines can be specified. You can also set the interpolation method
 * to linear, in which case the specified spline has no effect on the
 * interpolation.
 *
 * @warning
 * Previously inserted tuples are preserved when the number of components or
 * the type of interpolation changes. When the number of components is reduced,
 * the trailing components of every tuple are dropped; when it is increased,
 * the new components are zero-filled.
 *
 * @note
 * Bisection methods are used to speed up the search for the interpolation interval.
 */

#ifndef vtkTupleInterpolator_h
#define vtkTupleInterpolator_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include <vector> // for arg, return

VTK_ABI_NAMESPACE_BEGIN
class vtkSpline;
class vtkPiecewiseFunction;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkTupleInterpolator : public vtkObject
{
public:
  vtkTypeMacro(vtkTupleInterpolator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate the class.
   */
  static vtkTupleInterpolator* New();

  ///@{
  /**
   * Specify the number of tuple components to interpolate. Previously
   * inserted data is preserved. The policy adopted here is that
   * 1. added components are zero-filled.
   * 2. removed components are dropped.
   */
  void SetNumberOfComponents(int numComp);
  vtkGetMacro(NumberOfComponents, int);
  ///@}

  /**
   * Return the number of tuples in the list of tuples to be
   * interpolated.
   */
  int GetNumberOfTuples();

  ///@{
  /**
   * Obtain some information about the interpolation range. The numbers
   * returned (corresponding to parameter t, usually thought of as time)
   * are undefined if the list of transforms is empty. This is a convenience
   * method for interpolation.
   */
  double GetMinimumT();
  double GetMaximumT();
  ///@}

  /**
   * Reset the class so that it contains no (t,tuple) information.
   */
  void Initialize();

  /**
   * Add all the tuples to the list of tuples in one time,
   * and then sort them only once. Much faster than using
   * AddTuple for each tuple.
   * t is the time values array, nb is the size of time values array,
   * data is the array containing tuples to add (by default AOS ordering)
   */
  void FillFromData(int nb, double* t, double** data, bool isSOADataArray = false);

  /**
   * Add another tuple to the list of tuples to be interpolated.  Note that
   * using the same time t value more than once replaces the previous tuple
   * value at t. At least two tuples must be added to define an
   * interpolation function.
   */
  void AddTuple(double t, double tuple[]) VTK_SIZEHINT(tuple, GetNumberOfComponents());

  /**
   * Delete the tuple at a particular parameter t. If there is no
   * tuple defined at t, then the method does nothing.
   */
  void RemoveTuple(double t);

  /**
   * Interpolate the list of tuples and determine a new tuple (i.e.,
   * fill in the tuple provided). If t is outside the range of
   * (min,max) values, then t is clamped. Note that each component
   * of tuple[] is interpolated independently.
   */
  void InterpolateTuple(double t, double tuple[]);
  std::vector<double> InterpolateTuple(double t);

  /**
   * Enums to control the type of interpolation to use.
   */
  enum
  {
    INTERPOLATION_TYPE_LINEAR = 0,
    INTERPOLATION_TYPE_SPLINE
  };

  ///@{
  /**
   * Specify which type of function to use for interpolation. By default
   * spline interpolation (SetInterpolationTypeToSpline()) is used
   * (i.e., a Kochanek spline) and the InterpolatingSpline instance variable
   * is used to birth the actual interpolation splines via a combination of
   * NewInstance() and DeepCopy(). You may also choose to use linear
   * interpolation by invoking SetInterpolationTypeToLinear(). Previously
   * inserted data is preserved across a change of interpolation type.
   */
  void SetInterpolationType(int type);
  vtkGetMacro(InterpolationType, int);
  void SetInterpolationTypeToLinear() { this->SetInterpolationType(INTERPOLATION_TYPE_LINEAR); }
  void SetInterpolationTypeToSpline() { this->SetInterpolationType(INTERPOLATION_TYPE_SPLINE); }
  ///@}

  ///@{
  /**
   * If the InterpolationType is set to spline, then this method applies. By
   * default Kochanek interpolation is used, but you can specify any instance
   * of vtkSpline to use. Note that the actual interpolating splines are
   * created by invoking NewInstance() followed by DeepCopy() on the
   * interpolating spline specified here, for each tuple component to
   * interpolate. Those splines are rebuilt from the new prototype, preserving
   * previously inserted data.
   */
  void SetInterpolatingSpline(vtkSpline*);
  vtkGetObjectMacro(InterpolatingSpline, vtkSpline);
  ///@}

  ///@{
  /**
   * Get/Set all (t, tuple) samples as a flat interleaved array:
   * t0, c0_0, ..., c0_{n-1}, t1, c1_0, ..., c1_{n-1}, ..., tm, cm_0, ..., cm_{n-1}
   * to represent `m` samples and `n` number of components
   *
   * The length of values passed in to the SetTimedTuples must be
   * divisible by (GetNumberOfComponents() + 1). The samples returned by
   * GetTimedTuples are reconstructed on demand from the interpolation
   * functions, so this class holds no second copy of the data.
   *
   * Convenient method to initialize from wrapped languages.
   */
  void SetTimedTuples(const std::vector<double>& values);
  std::vector<double> GetTimedTuples() const;
  ///@}

protected:
  vtkTupleInterpolator();
  ~vtkTupleInterpolator() override;

  // The number of components being interpolated
  int NumberOfComponents;

  // Specify the type of interpolation to use
  int InterpolationType;

  // This is the default 1D spline to use
  vtkSpline* InterpolatingSpline;

  // Internal variables for interpolation functions
  void InitializeInterpolation();
  vtkPiecewiseFunction** Linear;
  vtkSpline** Spline;

private:
  vtkTupleInterpolator(const vtkTupleInterpolator&) = delete;
  void operator=(const vtkTupleInterpolator&) = delete;

  /**
   * Return the piecewise function holding the samples of the i'th component
   * for the current interpolation type, or `nullptr` when it does not exist.
   */
  vtkPiecewiseFunction* GetComponentFunction(int i) const;

  /**
   * Push interleaved samples laid out with `numComp` components back into the
   * interpolation functions of the current number of components.
   */
  void RestoreTimedTuples(const std::vector<double>& values, int numComp);

  /**
   * Discard the interpolation functions and recreate them for `numComp`
   * components, preserving the samples currently held.
   */
  void ReinitializeInterpolation(int numComp);
};

VTK_ABI_NAMESPACE_END
#endif
