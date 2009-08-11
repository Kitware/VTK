/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkBivariateStatisticsAlgorithm.h

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
// .NAME vtkBivariateStatistics - Base class for bivariate statistics 
// algorithms
//
// .SECTION Description
// This class specializes statistics algorithms to the bivariate case, where
// a number of pairs of columns of interest can be selected in the input data set.
// This is done by the means of the following functions:
//
// ResetColumns() - reset the list of columns of interest.
// Add/RemoveColum( namColX, namColY ) - try to add/remove column pair ( namColX,
// namColy ) to/from the list.
// SetColumnStatus ( namCol, status ) - mostly for UI wrapping purposes, try to 
// add/remove (depending on status) namCol from a list of buffered columns, from
// which all possible pairs are generated.
// The verb "try" is used in the sense that neither attempting to 
// repeat an existing entry nor to remove a non-existent entry will work.
// 
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkBivariateStatisticsAlgorithm_h
#define __vtkBivariateStatisticsAlgorithm_h

#include "vtkStatisticsAlgorithm.h"

class vtkTable;

class VTK_INFOVIS_EXPORT vtkBivariateStatisticsAlgorithm : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkBivariateStatisticsAlgorithm, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Reset list of column pairs of interest. 
  void ResetColumnPairs();

  // Description:
  // Convenience method to create a request with a single column name pair 
  //  (\p namColX, \p namColY) in a single call; this is the preferred method to select 
  // columns pairs, ensuring selection consistency (a pair of columns per request).
  // Warning: no name checking is performed on \p namCol; it is the user's
  // responsibility to use valid column names.
  void AddColumnPair( const char* namColX, const char* namColY );

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable* inData,
                              vtkDataObject* inMeta,
                              vtkTable* outData,
                              vtkDataObject* outMeta ); 

protected:
  vtkBivariateStatisticsAlgorithm();
  ~vtkBivariateStatisticsAlgorithm();

  vtkIdType NumberOfVariables;

private:
  vtkBivariateStatisticsAlgorithm(const vtkBivariateStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkBivariateStatisticsAlgorithm&);   // Not implemented
};

#endif

