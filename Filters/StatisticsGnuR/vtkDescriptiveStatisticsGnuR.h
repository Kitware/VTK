/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDescriptiveStatisticsGnuR.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDescriptiveStatisticsGnu - A class for univariate descriptive statistics using
// R to calculate p-values
//
// .SECTION Description
// Given a selection of columns of interest in an input data table, this
// class provides the following functionalities, depending on the chosen
// execution options:
// * Learn: calculate extremal values, sample mean, and M2, M3, and M4 aggregates
//   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
//   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
//   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
// * Derive: calculate unbiased variance estimator, standard deviation estimator,
//   two skewness estimators, and two kurtosis excess estimators.
// * Assess: given an input data set, a reference value and a non-negative deviation,
//   mark each datum with corresponding relative deviation (1-dimensional Mahlanobis
//   distance). If the deviation is zero, then mark each datum which are equal to the
//   reference value with 0, and all others with 1. By default, the reference value
//   and the deviation are, respectively, the mean and the standard deviation of the
//   input model.
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef vtkDescriptiveStatisticsGnuR_h
#define vtkDescriptiveStatisticsGnuR_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkDescriptiveStatistics.h"

class vtkDoubleArray;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkDescriptiveStatisticsGnuR : public vtkDescriptiveStatistics
{
public:
  vtkTypeMacro(vtkDescriptiveStatisticsGnuR, vtkDescriptiveStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDescriptiveStatisticsGnuR* New();

protected:
  vtkDescriptiveStatisticsGnuR();
  ~vtkDescriptiveStatisticsGnuR();

//BTX
  virtual vtkDoubleArray* CalculatePValues(vtkDoubleArray*);
//ETX

private:
  vtkDescriptiveStatisticsGnuR(const vtkDescriptiveStatisticsGnuR&); // Not implemented
  void operator=(const vtkDescriptiveStatisticsGnuR&); // Not implemented
};

#endif
