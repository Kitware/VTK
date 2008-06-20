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
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkCorrelativeStatistics - A class for linear correlation
//
// .SECTION Description
// Given a selection of pairs of columns of interest, this class provides the
// following functionalities, depending on the execution mode it is executed in:
// * Learn: calculate means, unbiased variance and covariance estimators of
//   column pairs, and corresponding linear regressions and linear correlation 
//   coefficient. More precisely, ExecuteLearn calculates the sums; if \p finalize
//   is set to true (default), the final statistics are calculated with the 
//   function CalculateFromSums. Otherwise, only raw sums are output; this 
//   option is made for efficient parallel calculations.
//   Note that CalculateFromSums is a static function, so that it can be used
//   directly with no need to instantiate a vtkCorrelativeStatistics object.
// * Validate: not validate mode for this statistics class.
// * Evince: given two data vectors X and Y with the same number of entries as
//   input in port 0, and reference means, variances, and covariance, along
//   with an acceptable threshold t>1, evince all pairs of values of (X,Y) 
//   whose relative PDF (assuming a bivariate Gaussian model) is below t.
//  
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkCorrelativeStatistics_h
#define __vtkCorrelativeStatistics_h

#include "vtkStatisticsAlgorithm.h"

class vtkCorrelativeStatisticsPrivate;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkCorrelativeStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkCorrelativeStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCorrelativeStatistics* New();

  // Description:
  // Reset list of column pairs of interest. 
  void ResetColumnPairs();

  // Description:
  // Add column name pair (\p namColX, \p namColY).
  // If \p reset is true, then the list of column pairs is priorly reset.
  // Warning: no name checking is performed on \p namColX nor \p namColY; it is 
  // the user's responsibility to use valid column names.
  void AddColumnPair( const char* namColX, const char* namColY, bool reset = false );

  // Description:
  // Remove, if it is present, column name pair (\p namColX, \p namColY).
 void RemoveColumnPair( const char* namColX, const char* namColY );

  // Description:
  // Method for UI to call to add/remove columns to/from the list
  void SetColumnStatus( const char* namCol, int status );

  // Description:
  // Calculate the following unbiased estimators from the raw sums:
  // means, variances, and covariance estimators, as well as the linear regressions
  // of X in Y, and Y in X, along with the correlation coefficient of X and Y.
  // Input: the sample size and a vector of doubles of size 5, initialized as (in this
  //        order) sum X, sum Y, sum X^2, sum Y^2, and sum XY.
  // Output: -1 if meaningless input (sample size < 2),
  //          1 if one of the variances is 0,
  //          0 otherwise.
  // NB: this is a static function, so as to provide this functionality even when no
  // vtkStatistics are instantiated.
  static int CalculateFromSums( int n, 
                                double& sx,
                                double& sy,
                                double& sx2,
                                double& sy2,
                                double& sxy,
                                double* correlations );
  static int CalculateFromSums( int n, double* sums, double* correlations )
    { 
    return CalculateFromSums( n, sums[0], sums[1], sums[2], sums[4], sums[5], correlations ); 
    }

protected:
  vtkCorrelativeStatistics();
  ~vtkCorrelativeStatistics();

  // Description:
  // Execute the required calculations in the specified execution modes
  virtual void ExecuteLearn( vtkTable* dataset,
                             vtkTable* output,
                             bool finalize = true );
  virtual void ExecuteValidate( vtkTable* dataset,
                                vtkTable* params,
                                vtkTable* output); 
  virtual void ExecuteEvince( vtkTable* dataset,
                              vtkTable* params,
                              vtkTable* output ); 

  vtkCorrelativeStatisticsPrivate* Internals;

private:
  vtkCorrelativeStatistics(const vtkCorrelativeStatistics&); // Not implemented
  void operator=(const vtkCorrelativeStatistics&);   // Not implemented
};

#endif

