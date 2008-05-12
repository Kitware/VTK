/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDescriptiveStatistics.h

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
// .NAME vtkDescriptiveStatistics - A class for descriptive statistics
//
// .SECTION Description
// This class provides the following functionalities, depending on the
// execution mode it is executed in:
// * Learn: given an input data set, calculate its extremal values, arithmetic
//   mean, unbiased variance estimator, skewness estimator, and both sample
//   and G2 estimation of the kurtosis excess. More precisely, ExecuteLearn
//   always calculates the sums; if the \a finalize parameter is
//   set to true (default), the final statistics are calculated with the 
//   function CalculateFromSums. Otherwise, only raw sums are output; this 
//   option is made for efficient parallel calculations.
//   Note that CalculateFromSums is a static function, so that it can be used
//   directly with no need to instantiate a vtkDescriptiveStatistics object.
// * Validate: not implemented.
// * Evince: given an input data set in port 0, and a reference value x along
//   with an acceptable deviation d>0, evince all entries in the data set who
//   are outside of [x-d,x+d].
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkDescriptiveStatistics_h
#define __vtkDescriptiveStatistics_h

#include "vtkStatisticsAlgorithm.h"

class vtkDescriptiveStatisticsPrivate;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkDescriptiveStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkDescriptiveStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDescriptiveStatistics* New();

  // Description:
  // Reset list of columns of interest
  void ResetColumns();

  // Description:
  // Add column index \p idxCol to the list of columns of interest
  // Warning: no range checking is performed on \p idxCol; it is the user's
  // responsibility to use valid column indices.
  void AddColumn( vtkIdType idxCol );

  // Description:
  // Remove (if it exists) column index \p idxCol to the list of columns of interest
  void RemoveColumn( vtkIdType idxCol );

  // Description:
  // Add column indices from \p idxColBegin (included) to \p idxColEnd (excluded).
  // Warning: no range checking is performed on \p idxColBegin nor \p idxColEnd; it is 
  // the user's responsibility to use valid column indices.
  void AddColumnRange( vtkIdType idxColBegin, vtkIdType idxColEnd );

  // Description:
  // Remove column indices from \p idxColBegin (included) to \p idxColEnd (excluded),
  // for those which are present.
  void RemoveColumnRange( vtkIdType idxColBegin, vtkIdType idxColEnd );

  // Description:
  // Calculate descriptive statistics estimators from the raw moments: 
  // mean (unbiased), variance (unbiased), sample skewness, 
  // kurtosis excess (sample and G2 estimators).
  // Input: the sample size and a vector of doubles of size 5, with its 4 first entries 
  //        initialized as (in this order) the 1st to 4th raw moments.
  // Output: -1 if meaningless input (sample size < 1),
  //          1 if could not calculate all improved estimators (sample size too small),
  //          0 otherwise.
  // NB: this is a static function, so as to provide this functionality even when no
  // vtkStatistics are instantiated.
  static int CalculateFromSums( int n, 
                                double& s1,
                                double& s2,
                                double& s3,
                                double& s4,
                                double& G2 );
  static int CalculateFromSums( int n, double* s ) 
    { 
    return CalculateFromSums( n, s[0], s[1], s[2], s[4], s[5] );
    }

  vtkDescriptiveStatisticsPrivate* Internals;

protected:
  vtkDescriptiveStatistics();
  ~vtkDescriptiveStatistics();

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

private:
  vtkDescriptiveStatistics(const vtkDescriptiveStatistics&); // Not implemented
  void operator=(const vtkDescriptiveStatistics&);   // Not implemented
};

#endif

