/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkAutoCorrelativeStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAutoCorrelativeStatistics
 * @brief   A class for univariate auto-correlative statistics
 *
 *
 * Given a selection of columns of interest in an input data table, this
 * class provides the following functionalities, depending on the chosen
 * execution options:
 * * Learn: calculate sample mean and M2 aggregates for each variable w.r.t. itself
 *   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
 *   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
 *   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
 *   for each specified time lag.
 * * Derive: calculate unbiased autocovariance matrix estimators and its determinant,
 *   linear regressions, and Pearson correlation coefficient, for each specified
 *   time lag.
 * * Assess: given an input data set, two means and a 2x2 covariance matrix,
 *   mark each datum with corresponding relative deviation (2-dimensional Mahlanobis
 *   distance).
 *
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012
*/

#ifndef vtkAutoCorrelativeStatistics_h
#define vtkAutoCorrelativeStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkAutoCorrelativeStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkAutoCorrelativeStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkAutoCorrelativeStatistics* New();

  //@{
  /**
   * Set/get the cardinality of the data set at given time, i.e., of
   * any given time slice. It cannot be negative.
   * The input data set is assumed to have a cardinality which
   * is a multiple of this value.
   * The default is 0, meaning that the user must specify a value
   * that is consistent with the input data set.
   */
  vtkSetClampMacro(SliceCardinality,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(SliceCardinality,vtkIdType);
  //@}

  /**
   * Given a collection of models, calculate aggregate model
   */
  void Aggregate( vtkDataObjectCollection*,
                  vtkMultiBlockDataSet* ) VTK_OVERRIDE;

protected:
  vtkAutoCorrelativeStatistics();
  ~vtkAutoCorrelativeStatistics() VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Learn option, given some input Data
   * NB: input parameters are unused.
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
                     vtkTable* ) VTK_OVERRIDE { return; };

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess( vtkTable* inData,
               vtkMultiBlockDataSet* inMeta,
               vtkTable* outData ) VTK_OVERRIDE
  { this->Superclass::Assess( inData, inMeta, outData, 1 ); }

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
                            AssessFunctor*& dfunc ) VTK_OVERRIDE;

  vtkIdType SliceCardinality;

private:
  vtkAutoCorrelativeStatistics( const vtkAutoCorrelativeStatistics& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkAutoCorrelativeStatistics& ) VTK_DELETE_FUNCTION;
};

#endif
