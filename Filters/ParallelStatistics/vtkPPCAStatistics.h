// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPPCAStatistics
 * @brief   A class for parallel principal component analysis
 *
 * vtkPPCAStatistics is vtkPCAStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay, David Thompson and Janine Bennett from
 * Sandia National Laboratories for implementing this class.
 */

#ifndef vtkPPCAStatistics_h
#define vtkPPCAStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkPCAStatistics.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPPCAStatistics : public vtkPCAStatistics
{
public:
  vtkTypeMacro(vtkPPCAStatistics, vtkPCAStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPPCAStatistics* New();

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPPCAStatistics();
  ~vtkPPCAStatistics() override;

  vtkMultiProcessController* Controller;

  // Execute the parallel calculations required by the Learn option.
  void Learn(vtkTable* inData, vtkTable* inParameters, vtkMultiBlockDataSet* outMeta) override;

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  void Test(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) override;

  vtkOrderStatistics* CreateOrderStatisticsInstance() override;

private:
  vtkPPCAStatistics(const vtkPPCAStatistics&) = delete;
  void operator=(const vtkPPCAStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
