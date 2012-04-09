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
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkContingencyStatistics - A class for bivariate correlation contigency
// tables, conditional probabilities, and information entropy
//
// .SECTION Description
// Given a pair of columns of interest, this class provides the
// following functionalities, depending on the operation in which it is executed:
// * Learn: calculate contigency tables and corresponding discrete joint
//   probability distribution.
// * Derive: calculate conditional probabilities, information entropies, and
//   pointwise mutual information.
// * Assess: given two columns of interest with the same number of entries as
//   input in port INPUT_DATA, and a corresponding bivariate probability distribution,
// * Test: calculate Chi-square independence statistic and, if VTK to R interface is available,
//   retrieve corresponding p-value for independence testing.
//  
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef __vtkContingencyStatistics_h
#define __vtkContingencyStatistics_h

#include "vtkStatisticsAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;

class VTK_INFOVIS_EXPORT vtkContingencyStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkContingencyStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkContingencyStatistics* New();

  // Description:
  // Given a collection of models, calculate aggregate model
  // NB: not implemented
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* ) { return; };

protected:
  vtkContingencyStatistics();
  ~vtkContingencyStatistics();

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
  virtual void Assess( vtkTable*,
                       vtkMultiBlockDataSet*,
                       vtkTable* );

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
                                    vtkMultiBlockDataSet* inMeta,
                                    vtkIdType pairKey,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
//ETX

private:
  vtkContingencyStatistics(const vtkContingencyStatistics&); // Not implemented
  void operator=(const vtkContingencyStatistics&);   // Not implemented
};

#endif

