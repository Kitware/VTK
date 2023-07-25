// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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

#include "vtkContingencyStatistics.h"
#include "vtkFiltersParallelStatisticsModule.h" // For export macro

#include <vector> // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPContingencyStatistics
  : public vtkContingencyStatistics
{
public:
  static vtkPContingencyStatistics* New();
  vtkTypeMacro(vtkPContingencyStatistics, vtkContingencyStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  void Learn(vtkTable*, vtkTable*, vtkMultiBlockDataSet*) override;

protected:
  vtkPContingencyStatistics();
  ~vtkPContingencyStatistics() override;

  /**
   * Reduce the collection of local contingency tables to the global one
   */
  bool Reduce(vtkIdType&, char*, vtkStdString&, vtkIdType&, vtkIdType*, std::vector<vtkIdType>&);

  /**
   * Broadcast reduced contingency table to all processes
   */
  bool Broadcast(vtkIdType, vtkStdString&, std::vector<vtkStdString>&, vtkIdType,
    std::vector<vtkIdType>&, vtkIdType);

  vtkMultiProcessController* Controller;

private:
  vtkPContingencyStatistics(const vtkPContingencyStatistics&) = delete;
  void operator=(const vtkPContingencyStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
