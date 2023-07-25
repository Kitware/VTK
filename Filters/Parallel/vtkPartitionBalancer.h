// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPartitionBalancer
 * @brief Balances input partitioned data sets so each rank has the same number of data sets.
 *
 * This filter can be applied on `vtkPartitionedDataSet` or `vtkPartitionedDataSetCollection`.
 *
 * * Given an input `vtkPartitionedDataSet`, this filter adds
 * `nullptr` instances in the output `vtkPartitionedDataSet` following a pattern specified
 * as parameter. The output partitioned data set will have the same number of partitions across
 * all ranks.
 * * Given an input `vtkPartitionedDataSetCollection`, this filter is applied on each partitioned
 * data set separately, and is producing a `vtkPartitioneDataSetCollection`.
 *
 * If some input partitions are `nullptr`, the output will see this partition squeezed out.
 * The filter will treat the input partitioned data set as if this `nullptr` partition was non
 * existent.
 *
 * The way the output is laid out is driven by the parameter `Mode`;
 * * `Mode::Expand` generates, per partitioned data set, as many partitions as there are partitions
 * in the input across all ranks.
 * Given a valid partition (not `nullptr`) in the output partitioned data set at index
 * n in rank i, all partitioned data set of all ranks but i have a `nullptr` instance as index n.
 * Output partitions are sorted by rank number. i.e., for i < j, partition at rank i are indexed
 * before partitions of rank j. Here is an example. of what would be generated for a
 * given input. PDC holds for Partitioned Dataset Collection, and PD holds for Partitioned Dataset.
 * @verbatim
 * Input:
 * rank 0: PDC [ PD (DS0, DS1,     DS2) ] [PD (nullptr, DS100) ]
 * rank 1: PDC [ PD (DS3, nullptr, DS4) ] [PD ()               ]
 *
 * Output:
 * rank 0: PDC [ PD (DS0,     DS1,     DS2,     nullptr, nullptr) ] [PD (DS100)   ]
 * rank 1: PDC [ PD (nullptr, nullptr, nullptr, DS3,     DS4)     ] [PD (nullptr) ]
 * @endverbatim
 * * `Mode::Squash` generates, per input partitioned data set, the minimum number of partitions
 * possible, appending `nullptr` in ranks lacking partitions. Using the same example as above:
 * @verbatim
 * Input:
 * rank 0: PDC [ PD (DS0, DS1,     DS2) ] [PD (nullptr, DS100) ]
 * rank 1: PDC [ PD (DS3, nullptr, DS4) ] [PD ()               ]
 *
 * Output:
 * rank 0: PDC [ PD (DS0, DS1, DS2)     ] [PD (DS100)   ]
 * rank 1: PDC [ PD (DS3, DS4, nullptr) ] [PD (nullptr) ]
 * @endverbatim
 */

#ifndef vtkPartitionBalancer_h
#define vtkPartitionBalancer_h

#include "vtkFiltersParallelModule.h" // for export macros
#include "vtkPartitionedDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPartitionBalancer : public vtkPartitionedDataSetAlgorithm
{
public:
  static vtkPartitionBalancer* New();
  vtkTypeMacro(vtkPartitionBalancer, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Modes defining the layout of the output.
   */
  enum Mode
  {
    Expand,
    Squash
  };

  ///@{
  /**
   * Set / Get current layout of the output. Default value is `vtkPartitionBalancer::Squash`.
   */
  vtkSetClampMacro(Mode, int, vtkPartitionBalancer::Expand, vtkPartitionBalancer::Squash);
  vtkGetMacro(Mode, int);
  ///@}

  /**
   * Sets filter to expand mode. See example below.
   *
   * @verbatim
   * Input:
   * rank 0: PDC [ PD (DS0, DS1,     DS2) ] [PD (nullptr, DS100) ]
   * rank 1: PDC [ PD (DS3, nullptr, DS4) ] [PD ()               ]
   *
   * Output:
   * rank 0: PDC [ PD (DS0,     DS1,     DS2,     nullptr, nullptr) ] [PD (DS100)   ]
   * rank 1: PDC [ PD (nullptr, nullptr, nullptr, DS3,     DS4)     ] [PD (nullptr) ]
   * @endverbatim
   */
  void SetModeToExpand() { this->SetMode(vtkPartitionBalancer::Expand); }

  /**
   * Sets filter to squash mode. See example below.
   *
   * @verbatim
   * Input:
   * rank 0: PDC [ PD (DS0, DS1,     DS2) ] [PD (nullptr, DS100) ]
   * rank 1: PDC [ PD (DS3, nullptr, DS4) ] [PD ()               ]
   *
   * Output:
   * rank 0: PDC [ PD (DS0, DS1, DS2)     ] [PD (DS100)   ]
   * rank 1: PDC [ PD (DS3, DS4, nullptr) ] [PD (nullptr) ]
   * @endverbatim
   */
  void SetModeToSquash() { this->SetMode(vtkPartitionBalancer::Squash); }

protected:
  vtkPartitionBalancer();
  ~vtkPartitionBalancer() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Local controller.
   */
  vtkMultiProcessController* Controller;

  int Mode;

private:
  vtkPartitionBalancer(const vtkPartitionBalancer&) = delete;
  void operator=(const vtkPartitionBalancer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
