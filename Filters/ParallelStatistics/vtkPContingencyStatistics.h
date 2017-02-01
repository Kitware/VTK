/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPContingencyStatistics.h

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
 * @class   vtkPContingencyStatistics
 * @brief   A class for parallel bivariate contingency statistics
 *
 * vtkPContingencyStatistics is vtkContingencyStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * .NOTE: It is assumed that the keys in the contingency table be contained in the set {0,...,n-1}
 * of successive integers, where n is the number of rows of the summary table.
 * If this requirement is not fulfilled, then the outcome of the parallel update of contingency
 * tables is unpredictable but will most likely be a crash.
 * Note that this requirement is consistent with the way contingency tables are constructed
 * by the (serial) superclass and thus, if you are using this class as it is intended to be ran,
 * then you do not have to worry about this requirement.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
*/

#ifndef vtkPContingencyStatistics_h
#define vtkPContingencyStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkContingencyStatistics.h"

#include <vector> // STL Header

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPContingencyStatistics : public vtkContingencyStatistics
{
public:
  static vtkPContingencyStatistics* New();
  vtkTypeMacro(vtkPContingencyStatistics, vtkContingencyStatistics);
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
  vtkPContingencyStatistics();
  ~vtkPContingencyStatistics();

  /**
   * Reduce the collection of local contingency tables to the global one
   */
  bool Reduce( vtkIdType&,
               char*,
               vtkStdString&,
               vtkIdType&,
               vtkIdType*,
               std::vector<vtkIdType>& );

  /**
   * Broadcast reduced contingency table to all processes
   */
  bool Broadcast( vtkIdType,
                  vtkStdString&,
                  std::vector<vtkStdString>&,
                  vtkIdType,
                  std::vector<vtkIdType>&,
                  vtkIdType );

  vtkMultiProcessController* Controller;
private:
  vtkPContingencyStatistics(const vtkPContingencyStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPContingencyStatistics&) VTK_DELETE_FUNCTION;
};

#endif

