/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkContingencyStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkContingencyStatistics* New();

  /**
   * Given a collection of models, calculate aggregate model
   * NB: not implemented
   */
  void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* ) VTK_OVERRIDE { return; };

protected:
  vtkContingencyStatistics();
  ~vtkContingencyStatistics() VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn( vtkTable*,
              vtkTable*,
              vtkMultiBlockDataSet* ) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive( vtkMultiBlockDataSet* ) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test( vtkTable*,
             vtkMultiBlockDataSet*,
             vtkTable* ) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess( vtkTable*,
               vtkMultiBlockDataSet*,
               vtkTable* ) VTK_OVERRIDE;

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
  void SelectAssessFunctor( vtkTable* outData,
                            vtkDataObject* inMeta,
                            vtkStringArray* rowNames,
                            AssessFunctor*& dfunc ) VTK_OVERRIDE;
  /**
   * Provide the appropriate assessment functor.
   * This one is the one that is actually used.
   */
  virtual void SelectAssessFunctor( vtkTable* outData,
                                    vtkMultiBlockDataSet* inMeta,
                                    vtkIdType pairKey,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );

private:
  vtkContingencyStatistics(const vtkContingencyStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContingencyStatistics&) VTK_DELETE_FUNCTION;
};

#endif

