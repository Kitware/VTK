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
// .NAME vtkDescriptiveStatistics - Base class for univariate statistics 
// algorithms
//
// .SECTION Description
// This class specializes statistics algorithms to the univariate case, where
// a number of columns of interest can be selected in the input data set.
// This is done by the means of the following functions:
//
// ResetColumns() - reset the list of columns of interest.
// Add/RemoveColum( i ) - try to add/remove column with index to the list.
// Add/RemoveColumnRange( i1, i2 ) - try to add/remove all columns with indices
// at least equal to i1 and strictly smaller to i2 to the list.
// The verb "try" is used in the sense that attempting to neither attempting to 
// repeat an existing entry nor to remove a non-existent entry will work.
// 
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkUnivariateStatisticsAlgorithm_h
#define __vtkUnivariateStatisticsAlgorithm_h

#include "vtkStatisticsAlgorithm.h"

class vtkUnivariateStatisticsAlgorithmPrivate;
class vtkTable;

class VTK_INFOVIS_EXPORT vtkUnivariateStatisticsAlgorithm : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkUnivariateStatisticsAlgorithm, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If \p all is true (default), then all input columns are selected, irrespective of 
  // the selection that may have been made using the Add/RemoveColumn[Range]() methods. 
  // If \p all is false, then only selected columns are of interest.
  void SelectAllColumns( bool all );

  // Description:
  // Reset list of columns of interest
  void ResetColumns();

  // Description:
  // Add column name \p namCol to the list of columns of interest
  // Warning: no name checking is performed on \p namCol; it is the user's
  // responsibility to use valid column names.
  void AddColumn( const char* namCol );

  // Description:
  // Remove (if it exists) column name \p namCol to the list of columns of interest
  void RemoveColumn( const char* namCol );

  // Description:
  // These methods are mostly provided for UI wrapping purposes. Although they can be
  // used in vanilla VTK code, this is not the recommended approach. Please utilize
  // AddColumn() or SelectAllColumns() instead.
  void BufferColumn( const char* );
  void SetAction( vtkIdType );

  // Description:
  // Set the column selection, depending on whether the all column selection mode is on or off.
  void SetColumnSelection( vtkTable* dataset );

protected:
  vtkUnivariateStatisticsAlgorithm();
  ~vtkUnivariateStatisticsAlgorithm();

  vtkUnivariateStatisticsAlgorithmPrivate* Internals;

private:
  vtkUnivariateStatisticsAlgorithm(const vtkUnivariateStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkUnivariateStatisticsAlgorithm&);   // Not implemented
};

#endif

