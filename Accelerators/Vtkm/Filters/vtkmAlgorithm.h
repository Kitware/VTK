// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class  vtkmAlgorithm
 * @brief  Base class for VTK-m algorithms
 *
 * This class is used to define methods and properties that are common to all
 * VTK-m algorithms.
 *
 * @warning Python wrapping of subclasses require special handling. Here is an example
 * ensuring wrapping works as expected,
 * implementing a temporal algorithm as a `vtkPassInputTypeAlgorithm`:
 *
 * ```
 * #include <vtkPassInputTypeAlgorithm.h>
 * #include <vtkmAlgorithm.h>
 *
 * #ifndef __VTK_WRAP__
 * #define vtkPassInputTypeAlgorithm vtkmAlgorithm<vtkPassInputTypeAlgorithm>
 * #endif
 *
 * // We make the wrapper think that we inherit from vtkPassInputTypeAlgorithm
 * class MyVTKmFilter : vtkPassInputTypeAlgorithm
 * {
 * public:
 *   vtkTypeMacro(MyVTKmFilter, vtkPassInputTypeAlgorithm);
 *
 * // We do not need to trick the wrapper with the superclass name anymore
 * #ifndef __VTK_WRAP__
 * #undef vtkPassInputTypeAlgorithm
 * #endif
 * ```
 *
 * @sa
 * vtkTemporalAlgorithm
 */

#ifndef vtkmAlgorithm_h
#define vtkmAlgorithm_h

#include "vtkAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

template <class AlgorithmT>
class vtkmAlgorithm : public AlgorithmT
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  vtkTemplateTypeMacro(vtkmAlgorithm, AlgorithmT);
  ///@}

  static_assert(std::is_base_of<vtkAlgorithm, AlgorithmT>::value,
    "Template argument must inherit vtkAlgorithm");

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if Viskores fails to run. When the flag is on,
   * the filter will generate an error if Viskores fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, bool);
  vtkSetMacro(ForceVTKm, bool);
  vtkBooleanMacro(ForceVTKm, bool);
  ///@}

protected:
  vtkmAlgorithm() = default;

  bool ForceVTKm = false;

private:
  vtkmAlgorithm(const vtkmAlgorithm&) = delete;
  void operator=(const vtkmAlgorithm&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkmAlgorithm_h
