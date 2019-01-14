/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractDataArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPExtractDataArraysOverTime
 * @brief parallel version of vtkExtractDataArraysOverTime.
 *
 * vtkPExtractDataArraysOverTime adds distributed data support to
 * vtkExtractDataArraysOverTime.
 *
 * It combines results from all ranks and produce non-empty result only on rank 0.
 *
 * @caveats Caveats Caveats
 *
 * This filter's behavior when `ReportStatisticsOnly` is true is buggy and will
 * change in the future. When `ReportStatisticsOnly` currently, each rank
 * computes separate stats for local data. Consequently, this filter preserves
 * each processes results separately (by adding suffix **rank=<rank num>** to each
 * of the block names, as appropriate. In future, we plan to fix this to
 * correctly compute stats in parallel for each block.
 */

#ifndef vtkPExtractDataArraysOverTime_h
#define vtkPExtractDataArraysOverTime_h

#include "vtkExtractDataArraysOverTime.h"
#include "vtkFiltersParallelModule.h" // For export macro

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPExtractDataArraysOverTime : public vtkExtractDataArraysOverTime
{
public:
  static vtkPExtractDataArraysOverTime* New();
  vtkTypeMacro(vtkPExtractDataArraysOverTime, vtkExtractDataArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPExtractDataArraysOverTime();
  ~vtkPExtractDataArraysOverTime() override;

  void PostExecute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkMultiProcessController* Controller;

private:
  vtkPExtractDataArraysOverTime(const vtkPExtractDataArraysOverTime&) = delete;
  void operator=(const vtkPExtractDataArraysOverTime&) = delete;
  void ReorganizeData(vtkMultiBlockDataSet* dataset);
};

#endif
