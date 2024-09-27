// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBatch
 * @brief   vtkBatch is a simple structure that holds a begin and end id.
 *
 * vtkBatch is a simple structure that holds a begin and end id of an element, e.g. cell or point.
 * vtkBatches is a vector with vtkBatch objects. vtkBatch has a template parameter that is used
 * to store additional information for each batch.
 *
 * The concept of batches is useful because it can be used to avoid spending memory by saving
 * information for each element of a batch, e.g. number of output cells/points. Instead, you can
 * save the aggregate information for all elements in the batch.
 *
 * Aside from saving memory, they are particularly useful because you can trim batches that should
 * be skipped in a follow-up step. For example, if you are processing cells, and some batches of
 * cells are not generating any output, you can trim those batches.
 *
 * Trimming batches is useful both for avoiding extra computation and also improving load balancing
 * in parallel processing.
 *
 * @sa vtkTableBasedClipDataSet, vtkExtractCells, vtkStaticFaceHashLinksTemplate,
 * vtkPolyDataPlaneCutter, vtkPolyDataPlaneClipper, vtkStructuredDataPlaneCutter
 */

#ifndef vtkBatch_h
#define vtkBatch_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSMPTools.h"
#include "vtkType.h"

#include <algorithm>
#include <functional>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
template <typename TBatchData>
struct vtkBatch
{
  vtkBatch() = default;
  ~vtkBatch() = default;

  vtkIdType BeginId;
  vtkIdType EndId;
  TBatchData Data;
};

//-----------------------------------------------------------------------------
template <typename TBatchData>
class vtkBatches
{
private:
  using vtkTBatch = vtkBatch<TBatchData>;
  using vector = std::vector<vtkTBatch>;
  using size_type = typename vector::size_type;
  using reference = typename vector::reference;
  using const_reference = typename vector::const_reference;
  using iterator = typename vector::iterator;
  using const_iterator = typename vector::const_iterator;

public:
  vtkBatches()
    : BatchSize(0)
  {
  }
  ~vtkBatches() = default;

  /**
   * Initialize the batches.
   */
  void Initialize(vtkIdType numberOfElements, unsigned int batchSize = 1000)
  {
    this->BatchSize = batchSize;

    const auto batchSizeSigned = static_cast<vtkIdType>(batchSize);
    const auto numberOfBatches = ((numberOfElements - 1) / batchSizeSigned) + 1;
    this->Batches.resize(static_cast<size_t>(numberOfBatches));
    const auto lastBatchId = numberOfBatches - 1;

    vtkSMPTools::For(0, numberOfBatches,
      [&](vtkIdType beginBatchId, vtkIdType endBatchId)
      {
        vtkIdType endIdValues[2] = { -1, numberOfElements };
        for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
        {
          auto& batch = this->Batches[batchId];
          batch.BeginId = batchId * batchSizeSigned;
          endIdValues[0] = (batchId + 1) * batchSizeSigned;
          batch.EndId = endIdValues[batchId == lastBatchId];
        }
      });
  }

  /**
   * Remove batches that should be skipped.
   */
  void TrimBatches(const std::function<bool(const vtkTBatch&)> shouldRemoveBatch)
  {
    const vtkIdType numberOfBatches = this->GetNumberOfBatches();
    if (numberOfBatches == 0)
    {
      return;
    }
    const vtkIdType numberOfThreads =
      std::min(static_cast<vtkIdType>(vtkSMPTools::GetEstimatedNumberOfThreads()), numberOfBatches);
    const vtkIdType lastThreadId = numberOfThreads - 1;
    const vtkIdType numberOfBatchesPerThread = numberOfBatches / numberOfThreads;

    std::vector<vtkIdType> tlInterEndBatchId;
    tlInterEndBatchId.resize(static_cast<size_t>(numberOfThreads));

    // perform batch trimming of batch segments
    vtkSMPTools::For(0, numberOfThreads,
      [&](vtkIdType beginThreadId, vtkIdType endThreadId)
      {
        for (vtkIdType threadId = beginThreadId; threadId < endThreadId; ++threadId)
        {
          const vtkIdType beginBatchId = threadId * numberOfBatchesPerThread;
          const vtkIdType endBatchId =
            threadId != lastThreadId ? (threadId + 1) * numberOfBatchesPerThread : numberOfBatches;
          auto beginBatchIter = this->Batches.begin() + beginBatchId;
          auto endBatchIter = this->Batches.begin() + endBatchId;
          auto newEndBatchIter = std::remove_if(beginBatchIter, endBatchIter, shouldRemoveBatch);
          tlInterEndBatchId[threadId] =
            beginBatchId + std::distance(beginBatchIter, newEndBatchIter);
        }
      });
    // compute the new end batch ids;
    std::vector<vtkIdType> tlNewEndBatchId;
    tlNewEndBatchId.resize(static_cast<size_t>(numberOfThreads));
    tlNewEndBatchId[0] = tlInterEndBatchId[0];
    for (vtkIdType threadId = 1; threadId < numberOfThreads; ++threadId)
    {
      const vtkIdType beginOldBatchId = threadId * numberOfBatchesPerThread;
      const auto& prevNewEndBatchId = tlNewEndBatchId[threadId - 1];
      const vtkIdType distance = tlInterEndBatchId[threadId] - beginOldBatchId;
      tlNewEndBatchId[threadId] = prevNewEndBatchId + distance;
      std::move_backward(this->Batches.begin() + beginOldBatchId,
        this->Batches.begin() + tlInterEndBatchId[threadId],
        this->Batches.begin() + tlNewEndBatchId[threadId]);
    }
    this->Batches.erase(this->Batches.begin() + tlNewEndBatchId[lastThreadId], this->Batches.end());
  }

  /**
   * Build offsets in place and returns the Global sum for a vtkBatch with Compact Batch Data
   *
   * Compact Batch Data are those that use one or more variables to calculate sums
   * and then converts them to offsets to save memory.
   *
   * @note For this function to work, you need to implement the following overloads to Batch Data:
   *
   * 1) TBatchData& operator+=(const TBatchData& rhs);
   * 2) TBatchData operator+(const TBatchData& rhs);
   */
  TBatchData BuildOffsetsAndGetGlobalSum()
  {
    const vtkIdType numberOfBatches = this->GetNumberOfBatches();
    if (numberOfBatches == 0)
    {
      return TBatchData{};
    }
    const vtkIdType numberOfThreads =
      std::min(static_cast<vtkIdType>(vtkSMPTools::GetEstimatedNumberOfThreads()), numberOfBatches);
    const vtkIdType lastThreadId = numberOfThreads - 1;
    const vtkIdType numberOfBatchesPerThread = numberOfBatches / numberOfThreads;

    // calculate the thread local sums;
    std::vector<TBatchData> tlSums;
    tlSums.resize(static_cast<size_t>(numberOfThreads));

    vtkSMPTools::For(0, numberOfThreads,
      [&](vtkIdType beginThreadId, vtkIdType endThreadId)
      {
        for (vtkIdType threadId = beginThreadId; threadId < endThreadId; ++threadId)
        {
          const vtkIdType beginBatchId = threadId * numberOfBatchesPerThread;
          const vtkIdType endBatchId =
            threadId != lastThreadId ? (threadId + 1) * numberOfBatchesPerThread : numberOfBatches;

          auto& threadSum = tlSums[threadId];
          for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
          {
            threadSum += this->Batches[batchId].Data;
          }
        }
      });

    // calculate the global sum;
    TBatchData globalSum;
    for (const auto& threadSum : tlSums)
    {
      globalSum += threadSum;
    }

    // calculate the thread local offsets using the thread local sums
    std::vector<TBatchData> tlOffsets;
    tlOffsets.resize(static_cast<size_t>(numberOfThreads));
    for (vtkIdType threadId = 1; threadId < numberOfThreads; ++threadId)
    {
      tlOffsets[threadId] = tlOffsets[threadId - 1] + tlSums[threadId - 1];
    }

    // convert the batch sums to offsets using the thread local offsets
    vtkSMPTools::For(0, numberOfThreads,
      [&](vtkIdType beginThreadId, vtkIdType endThreadId)
      {
        for (vtkIdType threadId = beginThreadId; threadId < endThreadId; ++threadId)
        {
          const vtkIdType beginBatchId = threadId * numberOfBatchesPerThread;
          const vtkIdType endBatchId =
            threadId != lastThreadId ? (threadId + 1) * numberOfBatchesPerThread : numberOfBatches;

          // store the first batch sum
          auto lastBatchData = this->Batches[beginBatchId].Data;
          // convert the first batch sum to an offset
          this->Batches[beginBatchId].Data = tlOffsets[threadId];
          for (vtkIdType batchId = beginBatchId + 1; batchId < endBatchId; ++batchId)
          {
            // store the current batch sum
            const auto currentBatchData = this->Batches[batchId].Data;
            // convert the current batch sum to an offset
            this->Batches[batchId].Data = this->Batches[batchId - 1].Data + lastBatchData;
            // store the current batch sum for the next iteration
            lastBatchData = std::move(currentBatchData);
          }
        }
      });

    return globalSum;
  }

  /**
   * Get the number of batches.
   */
  vtkIdType GetNumberOfBatches() const { return static_cast<vtkIdType>(this->Batches.size()); }

  /**
   * Get the batch size.
   */
  unsigned int GetBatchSize() const { return this->BatchSize; }

  ///@{
  /**
   * The following methods expose the underlying vector.
   */
  reference operator[](size_type pos) noexcept { return this->Batches[pos]; }
  const_reference operator[](size_type pos) const noexcept { return this->Batches[pos]; }
  iterator begin() noexcept { return this->Batches.begin(); }
  const_iterator begin() const noexcept { return this->Batches.begin(); }
  const_iterator cbegin() const noexcept { return this->Batches.cbegin(); }
  iterator end() noexcept { return this->Batches.end(); }
  const_iterator end() const noexcept { return this->Batches.end(); }
  const_iterator cend() const noexcept { return this->Batches.cend(); }
  ///@}

private:
  vector Batches;
  unsigned int BatchSize;
};
VTK_ABI_NAMESPACE_END

#endif // vtkBatch_h
// VTK-HeaderTest-Exclude: vtkBatch.h
