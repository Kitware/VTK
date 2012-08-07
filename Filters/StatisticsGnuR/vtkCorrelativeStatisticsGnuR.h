/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCorrelativeStatisticsGnuR.h

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
// .NAME vtkCorrelativeStatisticsGnuR - A class for bivariate linear correlation using R
// to calculate the p-values.
//
// .SECTION Description
// Given a selection of pairs of columns of interest, this class provides the
// following functionalities, depending on the chosen execution options:
// * Learn: calculate extremal values, sample mean, and M2 aggregates
//   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
//   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
//   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
// * Derive: calculate unbiased covariance matrix estimators and its determinant,
//   linear regressions, and Pearson correlation coefficient.
// * Assess: given an input data set, two means and a 2x2 covariance matrix,
//   mark each datum with corresponding relative deviation (2-dimensional Mahlanobis
//   distance).
// * Test: Perform Jarque-Bera-Srivastava test of 2-d normality
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef __vtkCorrelativeStatisticsGnuR_h
#define __vtkCorrelativeStatisticsGnuR_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkCorrelativeStatistics.h"

class vtkDoubleArray;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkCorrelativeStatisticsGnuR : public vtkCorrelativeStatistics
{
public:
  vtkTypeMacro(vtkCorrelativeStatisticsGnuR, vtkCorrelativeStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCorrelativeStatisticsGnuR* New();

protected:
  vtkCorrelativeStatisticsGnuR();
  ~vtkCorrelativeStatisticsGnuR();

//BTX
  virtual vtkDoubleArray* CalculatePValues(vtkDoubleArray*);
//ETX

private:
  vtkCorrelativeStatisticsGnuR(const vtkCorrelativeStatisticsGnuR&); // Not implemented
  void operator=(const vtkCorrelativeStatisticsGnuR&); // Not implemented
};

#endif
