// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkContingencyStatistics
 * @brief   A class for bivariate correlation contigency
 * tables, conditional probabilities, and information entropy
 *
 *
 * Given a pair of columns of interest, this class provides the
 * following functionalities, depending on the operation in which it is executed:
 * * Learn: calculate contigency tables and corresponding discrete joint
 *   probability distribution.
 * * Derive: calculate conditional probabilities, information entropies, and
 *   pointwise mutual information.
 * * Assess: given two columns of interest with the same number of entries as
 *   input in port INPUT_DATA, and a corresponding bivariate probability distribution,
 * * Test: calculate Chi-square independence statistic and, if VTK to R interface is available,
 *   retrieve corresponding p-value for independence testing.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef vtkContingencyStatistics_h
#define vtkContingencyStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;
class vtkIdTypeArray;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkContingencyStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkContingencyStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkContingencyStatistics* New();

  /**
   * Given a collection of models, calculate aggregate model
   * NB: not implemented
   */
  void Aggregate(vtkDataObjectCollection*, vtkMultiBlockDataSet*) override {}

protected:
  vtkContingencyStatistics();
  ~vtkContingencyStatistics() override;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn(vtkTable*, vtkTable*, vtkMultiBlockDataSet*) override;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive(vtkMultiBlockDataSet*) override;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) override;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) override;

  /**
   * Calculate p-value. This will be overridden using the object factory with an
   * R implementation if R is present.
   */
  virtual void CalculatePValues(vtkTable*);

  /**
   * Provide the appropriate assessment functor.
   * This one does nothing because the API is not sufficient for tables indexed
   * by a separate summary table.
   */
  void SelectAssessFunctor(vtkTable* outData, vtkDataObject* inMeta, vtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;
  /**
   * Provide the appropriate assessment functor.
   * This one is the one that is actually used.
   */
  virtual void SelectAssessFunctor(vtkTable* outData, vtkMultiBlockDataSet* inMeta,
    vtkIdType pairKey, vtkStringArray* rowNames, AssessFunctor*& dfunc);

private:
  vtkContingencyStatistics(const vtkContingencyStatistics&) = delete;
  void operator=(const vtkContingencyStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
