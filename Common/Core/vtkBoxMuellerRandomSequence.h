// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBoxMuellerRandomSequence
 * @brief   Gaussian sequence of pseudo random numbers implemented with the Box-Mueller transform
 *
 * vtkGaussianRandomSequence is a sequence of pseudo random numbers
 * distributed according to the Gaussian/normal distribution (mean=0 and
 * standard deviation=1).
 *
 * It based is calculation from a uniformly distributed pseudo random sequence.
 * The initial sequence is a vtkMinimalStandardRandomSequence.
 */

#ifndef vtkBoxMuellerRandomSequence_h
#define vtkBoxMuellerRandomSequence_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGaussianRandomSequence.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkBoxMuellerRandomSequence : public vtkGaussianRandomSequence
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkBoxMuellerRandomSequence* New();
  vtkTypeMacro(vtkBoxMuellerRandomSequence, vtkGaussianRandomSequence);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Satisfy general API of vtkRandomSequence superclass. Initialize the
   * sequence with a seed.
   */
  void Initialize(vtkTypeUInt32 vtkNotUsed(seed)) override {}

  /**
   * Current value.
   */
  double GetValue() override;

  /**
   * Move to the next number in the random sequence.
   */
  void Next() override;

  /**
   * Return the uniformly distributed sequence of random numbers.
   */
  vtkRandomSequence* GetUniformSequence();

  /**
   * Set the uniformly distributed sequence of random numbers.
   * Default is a .
   */
  void SetUniformSequence(vtkRandomSequence* uniformSequence);

protected:
  vtkBoxMuellerRandomSequence();
  ~vtkBoxMuellerRandomSequence() override;

  vtkRandomSequence* UniformSequence;
  double Value;

private:
  vtkBoxMuellerRandomSequence(const vtkBoxMuellerRandomSequence&) = delete;
  void operator=(const vtkBoxMuellerRandomSequence&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // #ifndef vtkBoxMuellerRandomSequence_h
