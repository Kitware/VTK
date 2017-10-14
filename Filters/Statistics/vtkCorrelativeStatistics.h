/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCorrelativeStatistics.h

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
 * @class   vtkCorrelativeStatistics
 * @brief   A class for bivariate linear correlation
 *
 *
 * Given a selection of pairs of columns of interest, this class provides the
 * following functionalities, depending on the chosen execution options:
 * * Learn: calculate sample mean and M2 aggregates for each pair of variables
 *   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
 *   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
 *   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
 * * Derive: calculate unbiased covariance matrix estimators and its determinant,
 *   linear regressions, and Pearson correlation coefficient.
 * * Assess: given an input data set, two means and a 2x2 covariance matrix,
 *   mark each datum with corresponding relative deviation (2-dimensional Mahlanobis
 *   distance).
 * * Test: Perform Jarque-Bera-Srivastava test of 2-d normality
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
*/

#ifndef vtkCorrelativeStatistics_h
#define vtkCorrelativeStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkCorrelativeStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkCorrelativeStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCorrelativeStatistics* New();

  /**
   * Given a collection of models, calculate aggregate model
   */
  void Aggregate( vtkDataObjectCollection*,
                  vtkMultiBlockDataSet* ) override;

protected:
  vtkCorrelativeStatistics();
  ~vtkCorrelativeStatistics() override;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn( vtkTable*,
              vtkTable*,
              vtkMultiBlockDataSet* ) override;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive( vtkMultiBlockDataSet* ) override;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test( vtkTable*,
             vtkMultiBlockDataSet*,
             vtkTable* ) override;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess( vtkTable* inData,
               vtkMultiBlockDataSet* inMeta,
               vtkTable* outData ) override
  { this->Superclass::Assess( inData, inMeta, outData, 2 ); }

  /**
   * Calculate p-value. This will be overridden using the object factory with an
   * R implementation if R is present.
   */
  virtual vtkDoubleArray* CalculatePValues(vtkDoubleArray*);

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor( vtkTable* outData,
                            vtkDataObject* inMeta,
                            vtkStringArray* rowNames,
                            AssessFunctor*& dfunc ) override;

private:
  vtkCorrelativeStatistics(const vtkCorrelativeStatistics&) = delete;
  void operator=(const vtkCorrelativeStatistics&) = delete;
};

#endif
