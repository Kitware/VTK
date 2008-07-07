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
// .NAME vtkDescriptiveStatistics - A class for univariate descriptive statistics
//
// .SECTION Description
// Given a selection of columns of interest in an input data table, this 
// class provides the following functionalities, depending on the
// execution mode it is executed in:
// * Learn: calculate extremal values, arithmetic mean, unbiased variance 
//   estimator, skewness estimator, and both sample and G2 estimation of the 
//   kurtosis excess. More precisely, ExecuteLearn calculates the sums; if
//   \p finalize is set to true (default), the final statistics are calculated
//   with CalculateFromSums. Otherwise, only raw sums are output; this 
//   option is made for efficient parallel calculations.
//   Note that CalculateFromSums is a static function, so that it can be used
//   directly with no need to instantiate a vtkDescriptiveStatistics object.
// * Assess: given an input data set in port 0, and a reference value x along
//   with an acceptable deviation d>0, assess all entries in the data set which
//   are outside of [x-d,x+d].
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkDescriptiveStatistics_h
#define __vtkDescriptiveStatistics_h

#include "vtkUnivariateStatisticsAlgorithm.h"

class vtkTable;
class vtkDoubleArray;

class VTK_INFOVIS_EXPORT vtkDescriptiveStatistics : public vtkUnivariateStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkDescriptiveStatistics, vtkUnivariateStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDescriptiveStatistics* New();

  // Description:
  // Calculate descriptive statistics estimators from the raw sums: 
  // mean (unbiased), variance (unbiased), sample skewness, 
  // sample kurtosis excess, standard deviation, and G2 kurtosis excess estimators.
  // Input: the sample size and a vector of doubles of size 5, with its 4 first entries 
  //        initialized as (in this order) the 1st to 4th raw sums.
  // Output: -1 if meaningless input (sample size < 1),
  //          1 if could not calculate all estimators (e.g., sample size too small),
  //          0 otherwise.
  // NB: this is a static function, so as to provide this functionality even when no
  // vtkStatistics are instantiated.
  static int CalculateFromSums( int n, 
                                double& s1,
                                double& s2,
                                double& s3,
                                double& s4,
                                double& sd,
                                double& G2 );
  static int CalculateFromSums( int n, double* s ) 
    { 
    return CalculateFromSums( n, s[0], s[1], s[2], s[4], s[5], s[6] );
    }

  // Description:
  // Set/get whether the deviations returned should be signed, or should
  // only have their magnitude reported.
  // The default is that signed deviations will be computed.
  vtkSetMacro(SignedDeviations,int);
  vtkGetMacro(SignedDeviations,int);
  vtkBooleanMacro(SignedDeviations,int);

  //BTX
  // Description:
  // A base class for a functor that computes deviations.
  class DeviantFunctor {
  public:
    double Nominal;
    double Deviation;
    virtual double operator() ( vtkIdType row ) = 0;
    virtual ~DeviantFunctor() { }
  };
  //ETX

protected:
  vtkDescriptiveStatistics();
  ~vtkDescriptiveStatistics();

  // Description:
  // Execute the required calculations in the specified execution modes
  virtual void ExecuteLearn( vtkTable* inData,
                             vtkTable* outMeta,
                             bool finalize = true );
  virtual void ExecuteAssess( vtkTable* inData,
                              vtkTable* inMeta,
                              vtkTable* outData,
                              vtkTable* outMeta ); 

  //BTX
  // Description:
  // The routine to compute the deviations for an entire array.
  void ComputeDeviations( vtkDoubleArray* relDev, DeviantFunctor* dfunc, vtkIdType nrow );
  //ETX

  int SignedDeviations;
  
private:
  vtkDescriptiveStatistics( const vtkDescriptiveStatistics& ); // Not implemented
  void operator = ( const vtkDescriptiveStatistics& );   // Not implemented
};

#endif

