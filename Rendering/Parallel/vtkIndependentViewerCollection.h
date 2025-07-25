// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIndependentViewerCollection
 * @brief   Store data for independent viewers
 *
 * This class holds information about independent viewers, only useful in
 * CAVE mode.  Information stored for each viewer includes an eye transform
 * matrix and an eye separation.
 */

#ifndef vtkIndependentViewerCollection_h
#define vtkIndependentViewerCollection_h

#include "vtkObject.h"
#include "vtkRenderingParallelModule.h" //needed for exports

#include <memory> // needed for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGPARALLEL_EXPORT vtkIndependentViewerCollection : public vtkObject
{
public:
  static vtkIndependentViewerCollection* New();
  vtkTypeMacro(vtkIndependentViewerCollection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // API needed by proxy/wrapping infrastructure (due to 'repeat_command="1"')

  /**
   * @brief Return the number of eye transform matrices (the number of
   * independent viewers).
   */
  int GetNumberOfEyeTransforms();

  /**
   * @brief Set the number of eye transform matrices (the number of
   * independent viewers).
   */
  void SetNumberOfEyeTransforms(int n);

  /**
   * @brief Set the eye transform matrix of the ith independent viewer.
   *
   * Note that if necessary, the number of viewers will be increased to
   * ensure the ith one can be set.
   */
  void SetEyeTransform(int i, const double* vals);

  /**
   * @brief Get the ith eye transform matrix as a vector of doubles.
   *
   * Note that if necessary, the number of viewers will be increased to
   * ensure the ith one can be returned.
   */
  void GetEyeTransform(int i, std::vector<double>& valuesOut);

  /**
   * @brief Return the number of eye separations (the number of
   * independent viewers).
   */
  int GetNumberOfEyeSeparations();

  /**
   * @brief Set the number of eye separations (the number of
   * independent viewers).
   */
  void SetNumberOfEyeSeparations(int n);

  /**
   * @brief Set the eye separation of the ith independent viewer.
   *
   * Note that if necessary, the number of viewers will be increased to
   * ensure the ith one can be set.
   */
  void SetEyeSeparation(int i, double separation);

  /**
   * @brief Get the ith eye separation as a vector of doubles.
   *
   * Note that if necessary, the number of viewers will be increased to
   * ensure the ith one can be returned.
   */
  double GetEyeSeparation(int i);

  // Convenience API

  /**
   * @brief Set the eye transform for the ith indpendent viewer, given
   * a vector of matrix values.
   *
   * Note that if necessary, the number of viewers will be increased to
   * ensure the ith one can be set.
   */
  void SetEyeTransform(int i, const std::vector<double>& vals);

  /**
   * @brief Get the number of independent viewers. This is the same number
   * returned by the methods that get the number of eye transforms and the
   * number of eye separations.
   */
  int GetNumberOfIndependentViewers();

  /**
   * @brief Set the number of independent viewers
   */
  void SetNumberOfIndependentViewers(int n);

protected:
  vtkIndependentViewerCollection();
  ~vtkIndependentViewerCollection() override;

private:
  vtkIndependentViewerCollection(const vtkIndependentViewerCollection&) = delete;
  void operator=(const vtkIndependentViewerCollection&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
