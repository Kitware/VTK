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
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkOrderStatistics - A class for univariate order statistics
//
// .SECTION Description
// Given a selection of columns of interest in an input data table, this
// class provides the following functionalities, depending on the
// execution mode it is executed in:
// * Learn: calculate arbitrary quantiles. Provide specific names when 5-point
//   statistics (minimum, 1st quartile, median, third quartile, maximum) requested.
// * Assess: given an input data set and a set of q-quantiles, label each datum
//   either with the quantile interval to which it belongs, or 0 if it is smaller
//   than smaller quantile, or q if it is larger than largest quantile.
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this class.

#ifndef __vtkOrderStatistics_h
#define __vtkOrderStatistics_h

#include "vtkUnivariateStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;

class VTK_INFOVIS_EXPORT vtkOrderStatistics : public vtkUnivariateStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkOrderStatistics, vtkUnivariateStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkOrderStatistics* New();

//BTX
  // Description:
  // The type of quantile definition.
  enum QuantileDefinitionType {
    InverseCDF              = 0,
    InverseCDFAveragedSteps = 1 // Ignored for non-numeric types
    };
//ETX

  // Description:
  // Set/get whether histograms and assessment data should be treated as numeric data.
  // Otherwise, then everything is treated as strings, which always works, and is thus the
  // the default.
  // Note that if the data is indeed numeric but this is not set, some strange results will
  // occur because of the use of the lexicographic order instead of the order on reals.
  vtkSetMacro( NumericType, int );
  vtkGetMacro( NumericType, int );
  vtkBooleanMacro( NumericType, int );

  // Description:
  // Set/Get the number of quantiles (with uniform spacing).
  vtkSetMacro( NumberOfIntervals, vtkIdType );
  vtkGetMacro( NumberOfIntervals, vtkIdType );

  // Description:
  // Set the quantile definition.
  vtkSetMacro( QuantileDefinition, QuantileDefinitionType );
  void SetQuantileDefinition ( int );

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
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive( vtkMultiBlockDataSet* );

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* );

//BTX
  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* outData,
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
//ETX

  int NumericType;
  int NumberOfIntervals;
  QuantileDefinitionType QuantileDefinition;

private:
  vtkOrderStatistics(const vtkOrderStatistics&); // Not implemented
  void operator=(const vtkOrderStatistics&);   // Not implemented
};

#endif
