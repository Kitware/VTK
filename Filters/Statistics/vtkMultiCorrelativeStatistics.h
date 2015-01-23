/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMultiCorrelativeStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkMultiCorrelativeStatistics - A class for multivariate linear correlation
//
// .SECTION Description
// Given a selection of sets of columns of interest, this class provides the
// following functionalities, depending on the operation in which it is executed:
// * Learn: calculates means, unbiased variance and covariance estimators of
//   column pairs coefficient.
//   More precisely, Learn calculates the averages and centered
//   variance/covariance sums; if \p finalize is set to true (default),
//   the final statistics are calculated.
//   The output metadata on port OUTPUT_MODEL is a multiblock dataset containing at a minimum
//   one vtkTable holding the raw sums in a sparse matrix style. If \a finalize is
//   true, then one additional vtkTable will be present for each requested set of
//   column correlations. These additional tables contain column averages, the
//   upper triangular portion of the covariance matrix (in the upper right hand
//   portion of the table) and the Cholesky decomposition of the covariance matrix
//   (in the lower portion of the table beneath the covariance triangle).
//   The leftmost column will be a vector of column averages.
//   The last entry in the column averages vector is the number of samples.
//   As an example, consider a request for a 3-column correlation with columns
//   named ColA, ColB, and ColC.
//   The resulting table will look like this:
//   <pre>
//      Column  |Mean     |ColA     |ColB     |ColC
//      --------+---------+---------+---------+---------
//      ColA    |avg(A)   |cov(A,A) |cov(A,B) |cov(A,C)
//      ColB    |avg(B)   |chol(1,1)|cov(B,B) |cov(B,C)
//      ColC    |avg(C)   |chol(2,1)|chol(2,2)|cov(C,C)
//      Cholesky|length(A)|chol(3,1)|chol(3,2)|chol(3,3)
//   </pre>
//   The mean point and the covariance matrix can be replaced by the median point and the
//   MAD matrix (Median Absolute Deviation) thanks to the MedianAbsoluteDeviation boolean.
//   In this mode, the resulting table will look like this:
//   <pre>
//      Column  |Mean     |ColA     |ColB     |ColC
//      --------+---------+---------+---------+---------
//      ColA    |med(A)   |MAD(A,A) |MAD(A,B) |MAD(A,C)
//      ColB    |med(B)   |chol(1,1)|MAD(B,B) |MAD(B,C)
//      ColC    |med(C)   |chol(2,1)|chol(2,2)|MAD(C,C)
//      Cholesky|length(A)|chol(3,1)|chol(3,2)|chol(3,3)
//   </pre>
//   The Median Absolute Deviation is known to be more robust than the covariance. It is
//   used in the robust PCA computation for instance.
// * Assess: given a set of results matrices as specified above in input port INPUT_MODEL and
//   tabular data on input port INPUT_DATA that contains column names matching those
//   of the tables on input port INPUT_MODEL, the assess mode computes the relative
//   deviation of each observation in port INPUT_DATA's table according to the linear
//   correlations implied by each table in port INPUT_MODEL.
//
// .SECTION Thanks
// Thanks to Philippe Pebay, Jackson Mayo, and David Thompson of
// Sandia National Laboratories for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012
// Updated by Tristan Coulange and Joachim Pouderoux, Kitware SAS 2013

#ifndef vtkMultiCorrelativeStatistics_h
#define vtkMultiCorrelativeStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkDoubleArray;
class vtkMultiBlockDataSet;
class vtkOrderStatistics;
class vtkVariant;

class VTKFILTERSSTATISTICS_EXPORT vtkMultiCorrelativeStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkMultiCorrelativeStatistics, vtkStatisticsAlgorithm);
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  static vtkMultiCorrelativeStatistics* New();

  // Description:
  // Given a collection of models, calculate aggregate model
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* );

  // Description:
  // If set to true, the covariance matrix is replaced by
  // the Median Absolute Deviation matrix.
  // Default is false.
  vtkSetMacro( MedianAbsoluteDeviation, bool );
  vtkGetMacro( MedianAbsoluteDeviation, bool );
  vtkBooleanMacro( MedianAbsoluteDeviation, bool );

protected:
  vtkMultiCorrelativeStatistics();
  ~vtkMultiCorrelativeStatistics();

  // Description:
  // Execute the calculations required by the Learn option.
  virtual void Learn( vtkTable*,
                      vtkTable*,
                      vtkMultiBlockDataSet* );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive( vtkMultiBlockDataSet* );

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable*,
                       vtkMultiBlockDataSet*,
                       vtkTable* );

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* ) { return; }

  //BTX
  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* inData,
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
  //ETX

  // Description:
  // Computes the median of inData with vtkOrderStatistics.
  virtual void ComputeMedian( vtkTable* inData, vtkTable* outData );

  // Description:
  // Return a new vtkOrderStatistics instance.
  // Used by derived class to return a derivate class instead.
  virtual vtkOrderStatistics* CreateOrderStatisticsInstance();

  bool MedianAbsoluteDeviation;

private:
  vtkMultiCorrelativeStatistics( const vtkMultiCorrelativeStatistics& ); // Not implemented
  void operator = ( const vtkMultiCorrelativeStatistics& );  // Not implemented
};

#endif


