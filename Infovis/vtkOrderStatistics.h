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
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkOrderStatistics - A class for univariate order statistics
//
// .SECTION Description
// Given a selection of columns of interest in an input data table, this 
// class provides the following functionalities, depending on the
// execution mode it is executed in:
// * Learn: calculate 5-point statistics (minimum, 1st quartile, median, third
//   quartile, maximum) and all other deciles (1,2,3,4,6,7,8,9).
// * Assess: given an input data set in port INPUT_DATA, and two percentiles p1 < p2,
//   assess all entries in the data set which are outside of [p1,p2].
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkOrderStatistics_h
#define __vtkOrderStatistics_h

#include "vtkUnivariateStatisticsAlgorithm.h"

class vtkStringArray;
class vtkTable;
class vtkVariant;

class VTK_INFOVIS_EXPORT vtkOrderStatistics : public vtkUnivariateStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkOrderStatistics, vtkUnivariateStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkOrderStatistics* New();

  // Description:
  // The type of quantile definition.
  //BTX
  enum QuantileDefinitionType {
    InverseCDF              = 0,
    InverseCDFAveragedSteps = 1
    };
  //ETX

  // Description:
  // Set the number of quantiles (with uniform spacing).
  vtkSetMacro( NumberOfIntervals, vtkIdType );

  // Description:
  // Get the number of quantiles (with uniform spacing).
  vtkGetMacro( NumberOfIntervals, vtkIdType );

  // Description:
  // Set the quantile definition.
  vtkSetMacro( QuantileDefinition, QuantileDefinitionType );
  void SetQuantileDefinition ( int );

  // Description:
  // Get the quantile definition.
  vtkIdType GetQuantileDefinition() { return static_cast<vtkIdType>( this->QuantileDefinition ); }

//BTX
  // Description:
  // A convenience method (in particular for access from other applications) to 
  // set parameter values.
  // Return true if setting of requested parameter name was excuted, false otherwise.
  virtual bool SetParameter( const char* parameter,
                             int index,
                             vtkVariant value );
//ETX

  // Description:
  // Given a collection of models, calculate aggregate model
  // NB: not implemented
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkDataObject* ) { return; };

protected:
  vtkOrderStatistics();
  ~vtkOrderStatistics();

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

  int NumberOfIntervals;
  QuantileDefinitionType QuantileDefinition;

private:
  vtkOrderStatistics(const vtkOrderStatistics&); // Not implemented
  void operator=(const vtkOrderStatistics&);   // Not implemented
};

#endif

