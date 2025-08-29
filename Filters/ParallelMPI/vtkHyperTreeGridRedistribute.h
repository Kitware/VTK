// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridRedistribute
 * @brief   Redistribute input HyperTreeGrid into requested number of partitions
 */

#ifndef vtkHyperTreeGridRedistribute_h
#define vtkHyperTreeGridRedistribute_h

#include "vtkHyperTreeGridAlgorithm.h"

#include "vtkFiltersParallelMPIModule.h" // For export macro
#include "vtkSmartPointer.h"             // For vtkSmartPointer
#include "vtkWeakPointer.h"              // for vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class vtkMPICommunicator;

class VTKFILTERSPARALLELMPI_EXPORT vtkHyperTreeGridRedistribute : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridRedistribute* New();
  vtkTypeMacro(vtkHyperTreeGridRedistribute, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* GetController();
  ///@}

protected:
  vtkHyperTreeGridRedistribute();
  ~vtkHyperTreeGridRedistribute() override;

  /**
   * Input must be either HTG or vtkPartitionedDataSet composed of HTG partitions.
   */
  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * Override RequestData, to make sure every HTG piece can be processed, hence avoiding that one
   * rank waits for the others which will actually never enter the filter.
   */
  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Main routine to redistribute trees and exchange cell data
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

private:
  vtkHyperTreeGridRedistribute(const vtkHyperTreeGridRedistribute&) = delete;
  void operator=(const vtkHyperTreeGridRedistribute&) = delete;

  /* Subroutines */
  void ExchangeHTGMetadata();
  void CollectLocalTreeIds();
  void BuildTargetPartMap();
  void ExchangeHyperTreeMetaData(vtkBitArray* descriptorSendBuffer,
    std::vector<int>& descriptorSizesReceivedBuffer, std::vector<int>& treeSizesSendBuffer,
    std::vector<int>& maskSizesSendBuffer, std::vector<int>& treeSizesReceivedBuffer,
    std::vector<int>& maskSizesReceivedBuffer, std::vector<int>& descriptorsByteOffsets);
  void BuildOutputTrees(vtkBitArray* descriptorSendBuffer,
    std::vector<int>& descriptorSizesReceivedBuffer, std::vector<int>& descriptorsByteOffsets);
  void ExchangeMask(
    std::vector<int>& maskSizesSendBuffer, std::vector<int>& maskSizesReceivedBuffer);
  void CollectCellArraySizes(std::vector<int>& treeSizesSendBuffer,
    std::vector<int>& treeSizesReceivedBuffer, std::vector<int>& cellsSentPerPartOffset,
    std::vector<int>& cellsReceivedPerPartOffset, std::vector<int>& nbCellDataSentPerPart,
    std::vector<int>& nbCellDataReceivedPerPart);
  void ExchangeCellArray(int arrayId, std::vector<int>& cellsSentPerPartOffset,
    std::vector<int>& cellsReceivedPerPartOffset, std::vector<int>& nbCellDataSentPerPart,
    std::vector<int>& nbCellDataReceivedPerPart);

  vtkHyperTreeGrid* InputHTG;
  vtkHyperTreeGrid* OutputHTG;
  vtkSmartPointer<vtkBitArray> OutMask;
  bool HasMask = false;
  int NumPartitions;

  std::vector<int> TreeTargetPartId; // Map Tree <-> Target Part Id
  std::vector<int> TreeIdsReceivedBuffer;
  std::vector<int> NbTreesReceivedPerPart;
  std::vector<int> NbTreesSentPerPart;
  std::vector<int> NbDescriptorsBytesPerPart;

  std::vector<vtkIdType> LocalTreeIds;
  std::vector<std::vector<vtkIdType>> TreesToSend; // Trees ids to send to each process

  vtkMPICommunicator* MPIComm = nullptr;
  vtkWeakPointer<vtkMultiProcessController> Controller;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridRedistribute */
