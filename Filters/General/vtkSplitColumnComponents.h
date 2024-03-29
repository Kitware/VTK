// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov
/**
 * @class   vtkSplitColumnComponents
 * @brief   split multicomponent table columns
 *
 *
 * Splits any columns in a table that have more than one component into
 * individual columns. Single component columns are passed through without
 * any data duplication.
 * NamingMode can be used to control how columns with multiple components are
 * labelled in the output, e.g., if column names "Points" had three components
 * this column would be split into "Points (0)", "Points (1)", and Points (2)"
 * when NamingMode is NUMBERS_WITH_PARENS, into Points_0, Points_1, and Points_2
 * when NamingMode is NUMBERS_WITH_UNDERSCORES, into "Points (X)", "Points (Y)",
 * and "Points (Z)" when NamingMode is NAMES_WITH_PARENS, and into Points_X,
 * Points_Y, and Points_Z when NamingMode is NAMES_WITH_UNDERSCORES.
 */

#ifndef vtkSplitColumnComponents_h
#define vtkSplitColumnComponents_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkTableAlgorithm.h"

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationIntegerKey;
class vtkInformationStringKey;

class VTKFILTERSGENERAL_EXPORT vtkSplitColumnComponents : public vtkTableAlgorithm
{
public:
  static vtkSplitColumnComponents* New();
  vtkTypeMacro(vtkSplitColumnComponents, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * If on this filter will calculate an additional magnitude column for all
   * columns it splits with two or more components.
   * Default is on.
   */
  vtkSetMacro(CalculateMagnitudes, bool);
  vtkGetMacro(CalculateMagnitudes, bool);
  vtkBooleanMacro(CalculateMagnitudes, bool);
  ///@}

  enum
  {
    NUMBERS_WITH_PARENS = 0,      // e.g Points (0)
    NAMES_WITH_PARENS = 1,        // e.g. Points (X)
    NUMBERS_WITH_UNDERSCORES = 2, // e.g. Points_0
    NAMES_WITH_UNDERSCORES = 3    // e.g. Points_X
  };

  ///@{
  /**
   * Get/Set the array naming mode.
   * Description is NUMBERS_WITH_PARENS.
   */
  vtkSetClampMacro(NamingMode, int, NUMBERS_WITH_PARENS, NAMES_WITH_UNDERSCORES);
  void SetNamingModeToNumberWithParens() { this->SetNamingMode(NUMBERS_WITH_PARENS); }
  void SetNamingModeToNumberWithUnderscores() { this->SetNamingMode(NUMBERS_WITH_UNDERSCORES); }
  void SetNamingModeToNamesWithParens() { this->SetNamingMode(NAMES_WITH_PARENS); }
  void SetNamingModeToNamesWithUnderscores() { this->SetNamingMode(NAMES_WITH_UNDERSCORES); }
  vtkGetMacro(NamingMode, int);
  ///@}

  ///@{
  /**
   * These are keys that get added to each output array to make it easier for
   * downstream filters to know which output array were extracted from which
   * input array.
   *
   * If either of these keys are missing, then the array was not extracted at
   * all.
   *
   * `ORIGINAL_COMPONENT_NUMBER` of -1 indicates magnitude.
   */
  static vtkInformationStringKey* ORIGINAL_ARRAY_NAME();
  static vtkInformationIntegerKey* ORIGINAL_COMPONENT_NUMBER();
  ///@}

protected:
  vtkSplitColumnComponents();
  ~vtkSplitColumnComponents() override;

  /**
   * Returns the label to use for the specific component in the array based on
   * this->NamingMode. Use component_no==-1 for magnitude.
   */
  std::string GetComponentLabel(vtkAbstractArray* array, int component_no);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSplitColumnComponents(const vtkSplitColumnComponents&) = delete;
  void operator=(const vtkSplitColumnComponents&) = delete;

  bool CalculateMagnitudes;
  int NamingMode;
};

VTK_ABI_NAMESPACE_END
#endif
