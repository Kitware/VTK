/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkContingencyStatistics.h

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
// .NAME vtkContingencyStatistics - A class for correlation with contigency
// tables.
//
// .SECTION Description
// Given a pair of columns of interest, this class provides the
// following functionalities, depending on the execution mode it is executed in:
// * Learn: calculate contigency tables and corresponding discrete bivariate
//   probability distribution. 
//   More precisely, ExecuteLearn calculates the sums; if \p finalize
//   is set to true (default), the final statistics are calculated with the 
//   function CalculateFromSums. Otherwise, only raw sums are output; this 
//   option is made for efficient parallel calculations.
//   Note that CalculateFromSums is a static function, so that it can be used
//   directly with no need to instantiate a vtkContingencyStatistics object.
// * Validate: no validate mode for this statistics class.
// * Evince: given two columns of interest with the same number of entries as
//   input in port 0, and a corresponding bivariate probability distribution,
//  
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkContingencyStatistics_h
#define __vtkContingencyStatistics_h

#include "vtkStatisticsAlgorithm.h"

#include "vtkStdString.h" // Because at least one method returns a vtkStdString

class vtkTable;

class VTK_INFOVIS_EXPORT vtkContingencyStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkContingencyStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkContingencyStatistics* New();

  // Description:
  // Set variable X.
  vtkSetStringMacro(X);

  // Description:
  // Get variable X.
  vtkGetStringMacro(X);

  // Description:
  // Set variable Y.
  vtkSetStringMacro(Y);

  // Description:
  // Get variable Y.
  vtkGetStringMacro(Y);

protected:
  vtkContingencyStatistics();
  ~vtkContingencyStatistics();

  // Description:
  // Execute the required calculations in the specified execution modes
  virtual void ExecuteLearn( vtkTable* dataset,
                             vtkTable* output,
                             bool finalize = true );
  virtual void ExecuteValidate( vtkTable* dataset,
                                vtkTable* params,
                                vtkTable* output); 
  virtual void ExecuteEvince( vtkTable* dataset,
                              vtkTable* params,
                              vtkTable* output ); 


  char* X;
  char* Y;

private:
  vtkContingencyStatistics(const vtkContingencyStatistics&); // Not implemented
  void operator=(const vtkContingencyStatistics&);   // Not implemented
};

#endif

