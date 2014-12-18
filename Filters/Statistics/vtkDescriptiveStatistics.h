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
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkDescriptiveStatistics - A class for univariate descriptive statistics
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
// * Test: calculate Jarque-Bera statistic and, if VTK to R interface is available,
//   retrieve corresponding p-value for normality testing.
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef vtkDescriptiveStatistics_h
#define vtkDescriptiveStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkDescriptiveStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkDescriptiveStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkDescriptiveStatistics* New();

  // Description:
  // Set/get whether the unbiased estimator for the variance should be used, or if
  // the population variance will be calculated.
  // The default is that the unbiased estimator will be used.
  vtkSetMacro(UnbiasedVariance,int);
  vtkGetMacro(UnbiasedVariance,int);
  vtkBooleanMacro(UnbiasedVariance,int);

  // Description:
  // Set/get whether the G1 estimator for the skewness should be used, or if
  // the g1 skewness will be calculated.
  // The default is that the g1 skewness estimator will be used.
  vtkSetMacro(G1Skewness,int);
  vtkGetMacro(G1Skewness,int);
  vtkBooleanMacro(G1Skewness,int);

  // Description:
  // Set/get whether the G2 estimator for the kurtosis should be used, or if
  // the g2 kurtosis will be calculated.
  // The default is that the g2 kurtosis estimator will be used.
  vtkSetMacro(G2Kurtosis,int);
  vtkGetMacro(G2Kurtosis,int);
  vtkBooleanMacro(G2Kurtosis,int);

  // Description:
  // Set/get whether the deviations returned should be signed, or should
  // only have their magnitude reported.
  // The default is that signed deviations will be computed.
  vtkSetMacro(SignedDeviations,int);
  vtkGetMacro(SignedDeviations,int);
  vtkBooleanMacro(SignedDeviations,int);

  // Description:
  // Given a collection of models, calculate aggregate model
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* );

protected:
  vtkDescriptiveStatistics();
  ~vtkDescriptiveStatistics();

  // Description:
  // Execute the calculations required by the Learn option, given some input Data
  // NB: input parameters are unused.
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
  { this->Superclass::Assess( inData, inMeta, outData, 1 ); }

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

  int UnbiasedVariance;
  int G1Skewness;
  int G2Kurtosis;
  int SignedDeviations;

private:
  vtkDescriptiveStatistics( const vtkDescriptiveStatistics& ); // Not implemented
  void operator = ( const vtkDescriptiveStatistics& );   // Not implemented
};

#endif
