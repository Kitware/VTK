/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPAutoCorrelativeStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPAutoCorrelativeStatistics
 * @brief   A class for parallel auto-correlative statistics
 *
 * vtkPAutoCorrelativeStatistics is vtkAutoCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012.
*/

#ifndef vtkPAutoCorrelativeStatistics_h
#define vtkPAutoCorrelativeStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkAutoCorrelativeStatistics.h"

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPAutoCorrelativeStatistics : public vtkAutoCorrelativeStatistics
{
public:
  static vtkPAutoCorrelativeStatistics* New();
  vtkTypeMacro(vtkPAutoCorrelativeStatistics, vtkAutoCorrelativeStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* );

protected:
  vtkPAutoCorrelativeStatistics();
  ~vtkPAutoCorrelativeStatistics();

  vtkMultiProcessController* Controller;
private:
  vtkPAutoCorrelativeStatistics(const vtkPAutoCorrelativeStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPAutoCorrelativeStatistics&) VTK_DELETE_FUNCTION;
};

#endif
