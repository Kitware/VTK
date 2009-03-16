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
// * Assess: given two columns of interest with the same number of entries as
//   input in port 0, and a corresponding bivariate probability distribution,
//  
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkContingencyStatistics_h
#define __vtkContingencyStatistics_h

#include "vtkBivariateStatisticsAlgorithm.h"

class vtkStringArray;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkContingencyStatistics : public vtkBivariateStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkContingencyStatistics, vtkBivariateStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkContingencyStatistics* New();

//BTX  
  // Description:
  // Provide the appropriate assessment functor.
  // This one does nothing because the API is not sufficient for tables indexed
  // by a separate summary table.
  virtual void SelectAssessFunctor( vtkTable* outData, 
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
  // Description:
  // Provide the appropriate assessment functor.
  // This one is the one that is actually used.
  virtual void SelectAssessFunctor( vtkTable* outData,
                                    vtkTable* contingencyTab,
                                    vtkIdType pairKey,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
//ETX

protected:
  vtkContingencyStatistics();
  ~vtkContingencyStatistics();

  // Description:
  // This algorithm accepts and returns a multiblock dataset containing several tables for
  // its meta input/output (port 1) instead of a single vtkTable.
  // FillInputPortInformation/FillOutputPortInformation are overridden accordingly.
  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  // Description:
  // Execute the calculations required by the Learn option.
  virtual void ExecuteLearn( vtkTable* inData,
                             vtkDataObject* outMeta );
  // Description:
  // Execute the calculations required by the Derive option.
  virtual void ExecuteDerive( vtkDataObject* );

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void ExecuteAssess( vtkTable* inData,
                              vtkDataObject* inMeta,
                              vtkTable* outData,
                              vtkDataObject* outMeta ); 

private:
  vtkContingencyStatistics(const vtkContingencyStatistics&); // Not implemented
  void operator=(const vtkContingencyStatistics&);   // Not implemented
};

#endif

