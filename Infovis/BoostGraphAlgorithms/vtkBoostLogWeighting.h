// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkBoostLogWeighting
 * @brief   Given an arbitrary-dimension array of doubles,
 * replaces each value x with one of:
 *
 * * The natural logarithm of 1 + x (the default)
 * * The base-2 logarithm of 1 + x
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkBoostLogWeighting_h
#define vtkBoostLogWeighting_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostLogWeighting : public vtkArrayDataAlgorithm
{
public:
  static vtkBoostLogWeighting* New();
  vtkTypeMacro(vtkBoostLogWeighting, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    BASE_E = 0,
    BASE_2 = 1
  };

  ///@{
  /**
   * Specify the logarithm base to apply
   */
  vtkSetMacro(Base, int);
  vtkGetMacro(Base, int);
  ///@}

  ///@{
  /**
   * Specify whether this filter should emit progress events
   */
  vtkSetMacro(EmitProgress, bool);
  vtkGetMacro(EmitProgress, bool);
  vtkBooleanMacro(EmitProgress, bool);
  ///@}

protected:
  vtkBoostLogWeighting();
  ~vtkBoostLogWeighting() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkBoostLogWeighting(const vtkBoostLogWeighting&) = delete;
  void operator=(const vtkBoostLogWeighting&) = delete;

  int Base;
  bool EmitProgress;
};

VTK_ABI_NAMESPACE_END
#endif
