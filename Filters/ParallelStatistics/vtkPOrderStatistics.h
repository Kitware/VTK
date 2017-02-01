/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPOrderStatistics.h

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
/**
 * @class   vtkPOrderStatistics
 * @brief   A class for parallel univariate order statistics
 *
 * vtkPOrderStatistics is vtkOrderStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * .NOTE: It is assumed that the keys in the histogram table be contained in the set {0,...,n-1}
 * of successive integers, where n is the number of rows of the summary table.
 * If this requirement is not fulfilled, then the outcome of the parallel update of order
 * tables is unpredictable but will most likely be a crash.
 * Note that this requirement is consistent with the way histogram tables are constructed
 * by the (serial) superclass and thus, if you are using this class as it is intended to be ran,
 * then you do not have to worry about this requirement.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
*/

#ifndef vtkPOrderStatistics_h
#define vtkPOrderStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkOrderStatistics.h"

#include <map> // STL Header

class vtkIdTypeArray;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPOrderStatistics : public vtkOrderStatistics
{
 public:
  static vtkPOrderStatistics* New();
  vtkTypeMacro(vtkPOrderStatistics, vtkOrderStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  virtual void Learn( vtkTable*,
                      vtkTable*,
                      vtkMultiBlockDataSet* ) VTK_OVERRIDE;

 protected:
  vtkPOrderStatistics();
  ~vtkPOrderStatistics();

  /**
   * Reduce the collection of local histograms to the global one for data inputs
   */
  bool Reduce( vtkIdTypeArray*,
               vtkDataArray* );

  /**
   * Reduce the collection of local histograms to the global one for string inputs
   */
  bool Reduce( vtkIdTypeArray*,
               vtkIdType&,
               char*,
               std::map<vtkStdString,vtkIdType>& );

  /**
   * Broadcast reduced histogram to all processes in the case of string inputs
   */
  bool Broadcast( std::map<vtkStdString,vtkIdType>&,
                  vtkIdTypeArray*,
                  vtkStringArray*,
                  vtkIdType );

  vtkMultiProcessController* Controller;
 private:
  vtkPOrderStatistics(const vtkPOrderStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPOrderStatistics&) VTK_DELETE_FUNCTION;
};

#endif
