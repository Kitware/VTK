/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkContingencyStatisticsGnuR.h

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
// .NAME vtkContingencyStatisticsGnuR - A class for bivariate correlation contigency
// tables, conditional probabilities, and information entropy. The p-value are
// calculated using R.
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

#ifndef vtkContingencyStatisticsGnuR_h
#define vtkContingencyStatisticsGnuR_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkContingencyStatistics.h"

class vtkIdTypeArray;
class vtkTable;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkContingencyStatisticsGnuR : public vtkContingencyStatistics
{
public:
  vtkTypeMacro(vtkContingencyStatisticsGnuR, vtkContingencyStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkContingencyStatisticsGnuR* New();

protected:
  vtkContingencyStatisticsGnuR();
  ~vtkContingencyStatisticsGnuR();

//BTX
  virtual void CalculatePValues(vtkTable*);
//ETX

private:
  vtkContingencyStatisticsGnuR(const vtkContingencyStatisticsGnuR&); // Not implemented
  void operator=(const vtkContingencyStatisticsGnuR&); // Not implemented
};

#endif

