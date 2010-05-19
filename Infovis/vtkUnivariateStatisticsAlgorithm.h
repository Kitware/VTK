/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUnivariateStatisticsAlgorithm.h

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
// .NAME vtkUnivariateStatistics - Base class for univariate statistics 
// algorithms
//
// .SECTION Description
// This class specializes statistics algorithms to the univariate case, where
// a number of columns of interest can be selected in the input data set.
// This is done by the means of the following functions:
//
// ResetColumns() - reset the list of columns of interest.
// Add/RemoveColum( namCol ) - try to add/remove column with name namCol to/from
// the list.
// SetColumnStatus ( namCol, status ) - mostly for UI wrapping purposes, try to 
// add/remove (depending on status) namCol from the list of columns of interest.
// The verb "try" is used in the sense that neither attempting to 
// repeat an existing entry nor to remove a non-existent entry will work.
// 
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkUnivariateStatisticsAlgorithm_h
#define __vtkUnivariateStatisticsAlgorithm_h

#include "vtkStatisticsAlgorithm.h"

class vtkTable;

class VTK_INFOVIS_EXPORT vtkUnivariateStatisticsAlgorithm : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkUnivariateStatisticsAlgorithm, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to create a request with a single column name \p namCol in a single
  // call; this is the preferred method to select columns, ensuring selection consistency
  // (a single column per request).
  // Warning: no name checking is performed on \p namCol; it is the user's
  // responsibility to use valid column names.
  void AddColumn( const char* namCol );

  // Description:
  // Use the current column status values to produce a new request for statistics
  // to be produced when RequestData() is called.
  // Unlike the superclass implementation, this version adds a new request for each selected column
  // instead of a single request containing all the columns.
  virtual int RequestSelectedColumns();

protected:
  vtkUnivariateStatisticsAlgorithm();
  ~vtkUnivariateStatisticsAlgorithm();

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable* inData,
                       vtkDataObject* inMeta,
                       vtkTable* outData ); 

private:
  vtkUnivariateStatisticsAlgorithm(const vtkUnivariateStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkUnivariateStatisticsAlgorithm&);   // Not implemented
};

#endif

