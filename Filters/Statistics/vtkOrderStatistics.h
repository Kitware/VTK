/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOrderStatistics.h

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
// .NAME vtkOrderStatistics - A class for univariate order statistics
//
// .SECTION Description
// Given a selection of columns of interest in an input data table, this
// class provides the following functionalities, depending on the
// execution mode it is executed in:
// * Learn: calculate histogram.
// * Derive: calculate PDFs and arbitrary quantiles. Provide specific names when 5-point
//   statistics (minimum, 1st quartile, median, third quartile, maximum) requested.
// * Assess: given an input data set and a set of q-quantiles, label each datum
//   either with the quantile interval to which it belongs, or 0 if it is smaller
//   than smaller quantile, or q if it is larger than largest quantile.
// * Test: calculate Kolmogorov-Smirnov goodness-of-fit statistic between CDF based on
//   model quantiles, and empirical CDF
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef vtkOrderStatistics_h
#define vtkOrderStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;

class VTKFILTERSSTATISTICS_EXPORT vtkOrderStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkOrderStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkOrderStatistics* New();

//BTX
  // Description:
  // The type of quantile definition.
  enum QuantileDefinitionType {
    InverseCDF              = 0, // Identical to method 1 of R
    InverseCDFAveragedSteps = 1, // Identical to method 2 of R, ignored for non-numeric types
    NearestObservation      = 2  // Identical to method 3 of R
    };
//ETX

  // Description:
  // Set/Get the number of quantiles (with uniform spacing).
  vtkSetMacro( NumberOfIntervals, vtkIdType );
  vtkGetMacro( NumberOfIntervals, vtkIdType );

  // Description:
  // Set the quantile definition.
  vtkSetMacro( QuantileDefinition, QuantileDefinitionType );
  void SetQuantileDefinition ( int );

  // Description:
  // Set/Get whether quantization will be allowed to enforce maximum histogram size.
  vtkSetMacro( Quantize, bool );
  vtkGetMacro( Quantize, bool );

  // Description:
  // Set/Get the maximum histogram size.
  // This maximum size is enforced only when Quantize is TRUE.
  vtkSetMacro( MaximumHistogramSize, vtkIdType );
  vtkGetMacro( MaximumHistogramSize, vtkIdType );

  // Description:
  // Get the quantile definition.
  vtkIdType GetQuantileDefinition() { return static_cast<vtkIdType>( this->QuantileDefinition ); }

  // Description:
  // A convenience method (in particular for access from other applications) to
  // set parameter values.
  // Return true if setting of requested parameter name was excuted, false otherwise.
  virtual bool SetParameter( const char* parameter,
                             int index,
                             vtkVariant value );

  // Description:
  // Given a collection of models, calculate aggregate model
  // NB: not implemented
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* ) { return; };

protected:
  vtkOrderStatistics();
  ~vtkOrderStatistics();

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
  { this->Superclass::Assess( inData, inMeta, outData, 1 ); }

//BTX
  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* outData,
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
//ETX

  vtkIdType NumberOfIntervals;
  QuantileDefinitionType QuantileDefinition;
  bool Quantize;
  vtkIdType MaximumHistogramSize;

private:
  vtkOrderStatistics(const vtkOrderStatistics&); // Not implemented
  void operator=(const vtkOrderStatistics&);   // Not implemented
};

#endif
