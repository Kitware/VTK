/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPCAStatisticsGnuR.h

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
// .NAME vtkPCAStatisticsGnuR - A class for multivariate principal component analysis
// using R to calculate p-values.
//
// .SECTION Description
// This class derives from the multi-correlative statistics algorithm and
// uses the covariance matrix and Cholesky decomposition computed by it.
// However, when it finalizes the statistics in learn operation, the PCA class
// computes the SVD of the covariance matrix in order to obtain its eigenvectors.
//
// In the assess operation, the input data are
// - projected into the basis defined by the eigenvectors,
// - the energy associated with each datum is computed,
// - or some combination thereof.
// Additionally, the user may specify some threshold energy or
// eigenvector entry below which the basis is truncated. This allows
// projection into a lower-dimensional state while minimizing (in a
// least squares sense) the projection error.
//
// In the test operation, a Jarque-Bera-Srivastava test of n-d normality is performed.
//
// .SECTION Thanks
// Thanks to David Thompson, Philippe Pebay and Jackson Mayo from
// Sandia National Laboratories for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef vtkPCAStatisticsGnuR_h
#define vtkPCAStatisticsGnuR_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkPCAStatistics.h"

class vtkDoubleArray;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkPCAStatisticsGnuR : public vtkPCAStatistics
{
public:
  vtkTypeMacro(vtkPCAStatisticsGnuR,vtkPCAStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  static vtkPCAStatisticsGnuR* New();

protected:
  vtkPCAStatisticsGnuR();
  ~vtkPCAStatisticsGnuR();
//BTX
  virtual vtkDoubleArray* CalculatePValues(vtkIdTypeArray*, vtkDoubleArray*);
//ETX

private:
  vtkPCAStatisticsGnuR(const vtkPCAStatisticsGnuR&); // Not implemented
  void operator=(const vtkPCAStatisticsGnuR&); // Not implemented
};

#endif // vtkPCAStatisticsGnuR_h

