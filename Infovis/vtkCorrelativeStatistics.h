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
//   coefficient. More precisely, Learn calculates the sums; if \p finalize
//   is set to true (default), the final statistics are calculated with the 
//   function CalculateFromSums. Otherwise, only raw sums are output; this 
//   option is made for efficient parallel calculations.
//   Note that CalculateFromSums is a static function, so that it can be used
//   directly with no need to instantiate a vtkCorrelativeStatistics object.
// * Assess: given two data vectors X and Y with the same number of entries as
//   input in port INPUT_DATA, and reference means, variances, and covariance, along
//   with an acceptable threshold t>1, assess all pairs of values of (X,Y) 
//   whose relative PDF (assuming a bivariate Gaussian model) is below t.
//  
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkCorrelativeStatistics_h
#define __vtkCorrelativeStatistics_h

#include "vtkBivariateStatisticsAlgorithm.h"

class vtkStringArray;
class vtkTable;
class vtkVariant;

class VTK_INFOVIS_EXPORT vtkCorrelativeStatistics : public vtkBivariateStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkCorrelativeStatistics, vtkBivariateStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCorrelativeStatistics* New();

  // Description:
  // Given a collection of models, calculate aggregate model
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkDataObject* );

protected:
  vtkCorrelativeStatistics();
  ~vtkCorrelativeStatistics();

  // Description:
  // Execute the calculations required by the Learn option.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkDataObject* outMeta );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive( vtkDataObject* );

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkDataObject*,
                     vtkDataObject* ) { return; };

//BTX  
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

