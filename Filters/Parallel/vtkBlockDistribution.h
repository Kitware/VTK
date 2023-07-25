// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) 2008 The Trustees of Indiana University.
// SPDX-License-Identifier: BSD-3-Clause AND BSL-1.0
/**
 * @class   vtkBlockDistribution
 * @brief   A helper class that manages a block distribution of N elements of data.
 *
 *
 *
 */

#ifndef vtkBlockDistribution_h
#define vtkBlockDistribution_h

#include "vtkABINamespace.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBlockDistribution
{
public:
  /**
   * Create a block distribution with N elements on P processors.
   */
  vtkBlockDistribution(vtkIdType N, vtkIdType P);

  /**
   * Retrieves the number of elements for which this block distribution was
   * built.
   */
  vtkIdType GetNumElements() { return this->NumElements; }

  /**
   * Retrieves the number of processors for which this block
   * distribution was built.
   */
  vtkIdType GetNumProcessors() { return this->NumProcessors; }

  /**
   * Get the block size for the processor with the given rank. This is the
   * number of elements that the processor will store.
   */
  vtkIdType GetBlockSize(vtkIdType rank);

  /**
   * Retrieve the process number in [0, GetNumProcessors()) where the element
   * with the given global index will be located.
   */
  vtkIdType GetProcessorOfElement(vtkIdType globalIndex);

  /**
   * Retrieve the local index (offset) on the processor determined by
   * GetProcessorOfElement that refers to the given global index.
   */
  vtkIdType GetLocalIndexOfElement(vtkIdType globalIndex);

  /**
   * Retrieve the first global index stored on the processor with the given
   * rank.
   */
  vtkIdType GetFirstGlobalIndexOnProcessor(vtkIdType rank);

  /**
   * Retrieve the global index associated with the given local index on the
   * processor with the given rank.
   */
  vtkIdType GetGlobalIndex(vtkIdType localIndex, vtkIdType rank);

private:
  vtkIdType NumElements;
  vtkIdType NumProcessors;
};

// ----------------------------------------------------------------------

inline vtkBlockDistribution::vtkBlockDistribution(vtkIdType N, vtkIdType P)
  : NumElements(N)
  , NumProcessors(P)
{
}

// ----------------------------------------------------------------------

inline vtkIdType vtkBlockDistribution::GetBlockSize(vtkIdType rank)
{
  return (this->NumElements / this->NumProcessors) +
    (rank < this->NumElements % this->NumProcessors ? 1 : 0);
}

// ----------------------------------------------------------------------

inline vtkIdType vtkBlockDistribution::GetProcessorOfElement(vtkIdType globalIndex)
{
  vtkIdType smallBlockSize = this->NumElements / this->NumProcessors;
  vtkIdType cutoffProcessor = this->NumElements % this->NumProcessors;
  vtkIdType cutoffIndex = cutoffProcessor * (smallBlockSize + 1);

  if (globalIndex < cutoffIndex)
  {
    return globalIndex / (smallBlockSize + 1);
  }
  else
  {
    return cutoffProcessor + (globalIndex - cutoffIndex) / smallBlockSize;
  }
}

// ----------------------------------------------------------------------

inline vtkIdType vtkBlockDistribution::GetLocalIndexOfElement(vtkIdType globalIndex)
{
  vtkIdType rank = this->GetProcessorOfElement(globalIndex);
  return globalIndex - this->GetFirstGlobalIndexOnProcessor(rank);
}

// ----------------------------------------------------------------------

inline vtkIdType vtkBlockDistribution::GetFirstGlobalIndexOnProcessor(vtkIdType rank)
{
  vtkIdType estimate = rank * (this->NumElements / this->NumProcessors + 1);
  vtkIdType cutoffProcessor = this->NumElements % this->NumProcessors;
  if (rank < cutoffProcessor)
  {
    return estimate;
  }
  else
  {
    return estimate - (rank - cutoffProcessor);
  }
}

// ----------------------------------------------------------------------

inline vtkIdType vtkBlockDistribution::GetGlobalIndex(vtkIdType localIndex, vtkIdType rank)
{
  return this->GetFirstGlobalIndexOnProcessor(rank) + localIndex;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkBlockDistribution.h
