// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDIYDataExchanger
 * @brief exchange data-object among ranks.
 *
 * vtkDIYDataExchanger is a utility to exchange data-objects across multiple
 * ranks. The design is based on `MPI_Alltoall` enabling algorithms to exchange
 * data-objects between each other. The implementation uses DIY.
 *
 * Note, current implementation only supports exchanging vtkDataSet and
 * subclasses. That may change in the future.
 */

#ifndef vtkDIYDataExchanger_h
#define vtkDIYDataExchanger_h

#include "vtkObject.h"
#include "vtkParallelDIYModule.h" // for export macros
#include "vtkSmartPointer.h"      // for vtkSmartPointer

#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkMultiProcessController;

class VTKPARALLELDIY_EXPORT vtkDIYDataExchanger : public vtkObject
{
public:
  static vtkDIYDataExchanger* New();
  vtkTypeMacro(vtkDIYDataExchanger, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GetGlobalController is used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Exchange data between all ranks in the process group defined by the
   * `Controller`.
   *
   * Every rank builds a vector of datasets to send to other ranks. This is
   * `sendBuffer`. `sendCounts` lets the current rank specify which dataset is
   * targeted for which rank. For example, if an MPI group has 3 ranks and the
   * current process wants to send 2 datasets to rank 0, none to rank 1, and 3
   * datasets to rank 2, then the `sendBuffer` should be filled with 5 (2 + 3)
   * datasets and `sendCounts` should be of size 3 filled with `[2, 0, 3]`. The
   * first 2 datasets in sendBuffer will be sent to rank 0 while the following 3
   * datasets will be send to rank 2.
   *
   * This is a collective operation which must be called on all ranks in the MPI
   * group irrespective of whether any rank is sending or receiving data.
   *
   * The `recvBuffer` is filled with datasets received from other ranks.
   * `recvCounts` is similar to `sendCounts` in that it helps determine how many
   * datasets were received from each rank and their offset in the `recvBuffer`.
   *
   * @param[in] sendBuffer  collection of datasets to exchange
   * @param[in] sendCounts  array of counts for number of datasets to send to
   *                        each rank.
   * @param[out] recvBuffer datasets received from other ranks
   * @param[out] recvCounts array of counts for number of datasets received from
   *                        each rank.
   *
   * @returns true on success, else false
   */
  bool AllToAll(const std::vector<vtkSmartPointer<vtkDataSet>>& sendBuffer,
    const std::vector<int>& sendCounts, std::vector<vtkSmartPointer<vtkDataSet>>& recvBuffer,
    std::vector<int>& recvCounts);

protected:
  vtkDIYDataExchanger();
  ~vtkDIYDataExchanger() override;

private:
  vtkDIYDataExchanger(const vtkDIYDataExchanger&) = delete;
  void operator=(const vtkDIYDataExchanger&) = delete;

  vtkMultiProcessController* Controller;
};

VTK_ABI_NAMESPACE_END
#endif
