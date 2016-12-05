/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageInterpolator
 * @brief   interpolate data values from images
 *
 * vtkImageInterpolator provides a simple interface for interpolating image
 * data.  It provides linear, cubic, and nearest-neighbor interpolation.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImageReslice
*/

#ifndef vtkImageInterpolator_h
#define vtkImageInterpolator_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkAbstractImageInterpolator.h"

class VTKIMAGINGCORE_EXPORT vtkImageInterpolator :
  public vtkAbstractImageInterpolator
{
public:
  static vtkImageInterpolator *New();
  vtkTypeMacro(vtkImageInterpolator, vtkAbstractImageInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The interpolation mode for point scalars (default: linear).  Subclasses
   * will provide additional interpolation modes, so this is a virtual method.
   */
  virtual void SetInterpolationMode(int mode);
  void SetInterpolationModeToNearest() {
    this->SetInterpolationMode(VTK_NEAREST_INTERPOLATION); }
  void SetInterpolationModeToLinear() {
    this->SetInterpolationMode(VTK_LINEAR_INTERPOLATION); }
  void SetInterpolationModeToCubic() {
    this->SetInterpolationMode(VTK_CUBIC_INTERPOLATION); }
  int GetInterpolationMode() { return this->InterpolationMode; }
  virtual const char *GetInterpolationModeAsString();
  //@}

  /**
   * Get the support size for use in computing update extents.  If the data
   * will be sampled on a regular grid, then pass a matrix describing the
   * structured coordinate transformation between the output and the input.
   * Otherwise, pass NULL as the matrix to retrieve the full kernel size.
   */
  virtual void ComputeSupportSize(const double matrix[16], int support[3]) VTK_OVERRIDE;

  /**
   * Returns true if the interpolator supports weight precomputation.
   * This will always return true for this interpolator.
   */
  virtual bool IsSeparable() VTK_OVERRIDE;

  //@{
  /**
   * If the data is going to be sampled on a regular grid, then the
   * interpolation weights can be precomputed.  A matrix must be supplied
   * that provides a transformation between the provided extent and the
   * structured coordinates of the input.  This matrix must perform only
   * permutations, scales, and translation, i.e. each of the three columns
   * must have only one non-zero value.  A new extent is provided that can
   * be used for out-of-bounds checks. THIS METHOD IS THREAD SAFE.
   */
  virtual void PrecomputeWeightsForExtent(
    const double matrix[16], const int extent[6], int newExtent[6],
    vtkInterpolationWeights *&weights) VTK_OVERRIDE;
  virtual void PrecomputeWeightsForExtent(
    const float matrix[16], const int extent[6], int newExtent[6],
    vtkInterpolationWeights *&weights) VTK_OVERRIDE;
  //@}

  /**
   * Free the precomputed weights.  THIS METHOD IS THREAD SAFE.
   */
  virtual void FreePrecomputedWeights(vtkInterpolationWeights *&weights) VTK_OVERRIDE;

protected:
  vtkImageInterpolator();
  ~vtkImageInterpolator();

  /**
   * Update the interpolator.
   */
  virtual void InternalUpdate() VTK_OVERRIDE;

  /**
   * Copy all members.
   */
  virtual void InternalDeepCopy(vtkAbstractImageInterpolator *obj) VTK_OVERRIDE;

  //@{
  /**
   * Get the interpolation functions.
   */
  virtual void GetInterpolationFunc(
    void (**doublefunc)(
      vtkInterpolationInfo *, const double [3], double *)) VTK_OVERRIDE;
  virtual void GetInterpolationFunc(
    void (**floatfunc)(
      vtkInterpolationInfo *, const float [3], float *)) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Get the row interpolation functions.
   */
  virtual void GetRowInterpolationFunc(
    void (**doublefunc)(
      vtkInterpolationWeights *, int, int, int, double *, int)) VTK_OVERRIDE;
  virtual void GetRowInterpolationFunc(
    void (**floatfunc)(
      vtkInterpolationWeights *, int, int, int, float *, int)) VTK_OVERRIDE;
  //@}

  int InterpolationMode;

private:
  vtkImageInterpolator(const vtkImageInterpolator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageInterpolator&) VTK_DELETE_FUNCTION;
};

#endif
