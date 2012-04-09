/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockDistribution.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
// .NAME vtkBlockDistribution - A helper class that manages a block distribution of N elements of data.
//
// .SECTION Description
//

#ifndef __vtkBlockDistribution_h
#define __vtkBlockDistribution_h

class vtkBlockDistribution
{
public:
  // Description:
  // Create a block distribution with N elements on P processors.
  vtkBlockDistribution(vtkIdType N, vtkIdType P);

  // Description:
  // Retrieves the number of elements for which this block distribution was
  // built.
  vtkIdType GetNumElements() { return this->NumElements; }

  // Description:
  // Retrieves the number of processors for which this block
  // distribution was built.
  vtkIdType GetNumProcessors() { return this->NumProcessors; }

  // Description:
  // Get the block size for the processor with the given rank. This is the
  // number of elements that the processor will store.
  vtkIdType GetBlockSize(vtkIdType rank);

  // Description:
  // Retrieve the process number in [0, GetNumProcessors()) where the element
  // with the given global index will be located.
  vtkIdType GetProcessorOfElement(vtkIdType globalIndex);

  // Description:
  // Retrieve the local index (offset) on the processor determined by
  // GetProcessorOfElement that refers to the given global index.
  vtkIdType GetLocalIndexOfElement(vtkIdType globalIndex);

  // Description:
  // Retrieve the first global index stored on the processor with the given
  // rank.
  vtkIdType GetFirstGlobalIndexOnProcessor(vtkIdType rank);

  // Description:
  // Retrieve the global index associated with the given local index on the
  // processor with the given rank.
  vtkIdType GetGlobalIndex(vtkIdType localIndex, vtkIdType rank);

private:
  vtkIdType NumElements;
  vtkIdType NumProcessors;
};

// ----------------------------------------------------------------------

inline vtkBlockDistribution::vtkBlockDistribution(vtkIdType N, vtkIdType P)
  : NumElements(N), NumProcessors(P)
{
}

// ----------------------------------------------------------------------

inline vtkIdType vtkBlockDistribution::GetBlockSize(vtkIdType rank)
{
  return (this->NumElements / this->NumProcessors)
       + (rank < this->NumElements % this->NumProcessors? 1 : 0);
}

// ----------------------------------------------------------------------

inline vtkIdType
vtkBlockDistribution::GetProcessorOfElement(vtkIdType globalIndex)
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

inline vtkIdType
vtkBlockDistribution::GetLocalIndexOfElement(vtkIdType globalIndex)
{
  vtkIdType rank = this->GetProcessorOfElement(globalIndex);
  return globalIndex - this->GetFirstGlobalIndexOnProcessor(rank);
}

// ----------------------------------------------------------------------

inline vtkIdType
vtkBlockDistribution::GetFirstGlobalIndexOnProcessor(vtkIdType rank)
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

inline vtkIdType
vtkBlockDistribution::GetGlobalIndex(vtkIdType localIndex, vtkIdType rank)
{
  return this->GetFirstGlobalIndexOnProcessor(rank) + localIndex;
}

#endif
// VTK-HeaderTest-Exclude: vtkBlockDistribution.h
