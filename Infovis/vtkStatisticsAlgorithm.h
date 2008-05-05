/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStatisticsAlgorithm.h

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
// .NAME vtkStatisticsAlgorithm - Base class for statistics algorithms
//
// .SECTION Description
// All statistics algorithms can conceptually be operated in a variety of 
// execution modes:
// * Learn: given an input data set, calculate some type of statistics (e.g., 
//   descriptive, 5-point, correlations, PCA).
// * Validate: given an input data set and input statistics of some kind, 
//   calculate to what extent these statistics are applicable to this data set.
//   Some type of thresholding may be part of the assessment process.
// * Evince: given an input data set, input statistics, and some form of 
//   threshold, evince a subset of the data set. 
// Therefore, a vtkStatisticsAlgorithm has the following vtkTable ports
// * 2 input ports:
//   * Data (mandatory)
//   * Statistics (optional) 
// * 1 output port (called Output):
//   * When in Learn mode, the output table contains summary statistics of 
//     the input dataset.
//   * When in Validate mode, the output table contains a summary of how well 
//     the model fits the data.
//   * When in Evince mode, the output table is a list of input rows that don't 
//     fit (or fit) the model to within some amount specified by the filter input 
//     parameters.
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkStatisticsAlgorithm_h
#define __vtkStatisticsAlgorithm_h

#include "vtkTableAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkStatisticsAlgorithm : public vtkTableAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkStatisticsAlgorithm, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The execution mode of the statistics algorithm.
  //BTX
  enum ExecutionModeType {
    LearnMode    = 0,
    ValidateMode = 1,
    EvinceMode   = 2,
    };
  //ETX

  // Description:
  // Set the execution mode.
  vtkSetMacro( ExecutionMode, ExecutionModeType );

  // Description:
  // Get the execution mode.
  vtkGetMacro( ExecutionMode, ExecutionModeType );

  // Description:
  // Set the sample size.
  vtkSetMacro( SampleSize, vtkIdType );

  // Description:
  // Get the sample size.
  vtkGetMacro( SampleSize, vtkIdType );

protected:
  vtkStatisticsAlgorithm();
  ~vtkStatisticsAlgorithm();

  virtual int FillInputPortInformation( int port, vtkInformation* info );

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* );

  // Description:
  // Execute the required calculations in the specified execution modes
  virtual void ExecuteLearn( vtkTable*,
                             vtkTable* ) = 0;
  virtual void ExecuteValidate( vtkTable*,
                                vtkTable*,
                                vtkTable* ) = 0; 
  virtual void ExecuteEvince( vtkTable*,
                              vtkTable*,
                              vtkTable* ) = 0; 

  vtkIdType SampleSize;
  ExecutionModeType ExecutionMode;
  
private:
  vtkStatisticsAlgorithm(const vtkStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkStatisticsAlgorithm&);   // Not implemented
};

#endif

