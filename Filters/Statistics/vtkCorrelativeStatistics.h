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
// .NAME vtkCorrelativeStatistics - A class for bivariate linear correlation
//
// .SECTION Description
// Given a selection of pairs of columns of interest, this class provides the
// following functionalities, depending on the chosen execution options:
// * Learn: calculate sample mean and M2 aggregates for each pair of variables
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

#ifndef __vtkCorrelativeStatistics_h
#define __vtkCorrelativeStatistics_h

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
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCorrelativeStatistics* New();

  // Description:
  // Given a collection of models, calculate aggregate model
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* );

protected:
  vtkCorrelativeStatistics();
  ~vtkCorrelativeStatistics();

  // Description:
  // Execute the calculations required by the Learn option.
  virtual void Learn( vtkTable*,
                      vtkTable*,
                      vtkMultiBlockDataSet* );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive( vtkMultiBlockDataSet* );

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* );

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable* inData,
                       vtkMultiBlockDataSet* inMeta,
                       vtkTable* outData )
  { this->Superclass::Assess( inData, inMeta, outData, 2 ); }

//BTX
  // Description:
  // Calculate p-value. This will be overridden using the object factory with an
  // R implementation if R is present.
  virtual vtkDoubleArray* CalculatePValues(vtkDoubleArray*);

  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* outData,
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
//ETX

private:
  vtkCorrelativeStatistics(const vtkCorrelativeStatistics&); // Not implemented
  void operator=(const vtkCorrelativeStatistics&);   // Not implemented
};

#endif
