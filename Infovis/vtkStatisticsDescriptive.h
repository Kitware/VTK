/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStatisticsDescriptive.h

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
// .NAME vtkStatisticsDescriptive - A class for descriptive statistics
//
// .SECTION Description
// This class provides the following functionalities, depending on the
// execution mode it is executed in:
// * Learn: given an input data set, calculate its extremal values, arithmetic
//   mean, unbiased variance estimator, skewness estimator, and G2 estimation 
//   of the kurtosis "excess".
// * Validate: not implemented.
// * Evince: given an input data set in port 0, and a reference value x along
//   with an acceptable deviation d>0, evince all entries in the data set who
//   are outside of [x-d,x+d].
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkStatisticsDescriptive_h
#define __vtkStatisticsDescriptive_h

#include "vtkStatisticsAlgorithm.h"

class vtkTable;

class VTK_INFOVIS_EXPORT vtkStatisticsDescriptive : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkStatisticsDescriptive, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkStatisticsDescriptive* New();

  // Description:
  // Calculate centered moment estimators (unbiased when available)
  // using the raw moments: mean (unbiased), variance (unbiased), sample skewness, 
  // kurtosis excess (sample and G2 estimators).
  // Input: the sample size and a vector of doubles of size 5, with its 4 first entries 
  //        initialized as (in this order) the 1st to 4th raw moments.
  // Return: -1 if meaningless input (sample size < 1),
  //           1 if could not calculate all improved estimators (sample size too small),
  //           0 otherwise.
  // NB: this is a static function, so as to provide functionality for centered moments
  // calculation, irrespective of whether vtkStatistics objects are used at all.
  static int CalculateCenteredMoments( int n, double* s );

protected:
  vtkStatisticsDescriptive();
  ~vtkStatisticsDescriptive();

  // Description:
  // Execute the required calculations in the specified execution modes
  virtual void ExecuteLearn( vtkTable* dataset,
                             vtkTable* output );
  virtual void ExecuteValidate( vtkTable* dataset,
                                vtkTable* params,
                                vtkTable* output); 
  virtual void ExecuteEvince( vtkTable* dataset,
                              vtkTable* params,
                              vtkTable* output ); 

private:
  vtkStatisticsDescriptive(const vtkStatisticsDescriptive&); // Not implemented
  void operator=(const vtkStatisticsDescriptive&);   // Not implemented
};

#endif

