/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXvtkVTU.txx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXvtkVTU.txx
 *
 *  Created on: June 25, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXxmlVTU_txx
#define VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXxmlVTU_txx

#include "VTXvtkVTU.h"

#include "VTX/common/VTXHelper.h"

namespace vtx
{
namespace schema
{

template <class T>
void VTXvtkVTU::SetBlocksCommon(
  adios2::Variable<T> variable, types::DataArray& dataArray, const size_t step)
{
  dataArray.BlockCounts.clear();

  const size_t mpiSize = static_cast<size_t>(helper::MPIGetSize());
  const size_t mpiRank = static_cast<size_t>(helper::MPIGetRank());
  // set dataArray BlockIDs and BlockCounts
  // get global Blocks Info
  const std::vector<typename adios2::Variable<T>::Info> blocksInfo =
    this->Engine.BlocksInfo(variable, step);
  const size_t blocks = blocksInfo.size();

  // MPI partition assuming all blocks are balanced for now
  // TODO: partition weighted by the block size
  const size_t blocksPerRank =
    (mpiRank == mpiSize - 1) ? blocks / mpiSize + blocks % mpiSize : blocks / mpiSize;

  const size_t beginBlockID = mpiRank * blocksPerRank;
  const size_t endBlockID = beginBlockID + blocksPerRank;
  for (auto b = beginBlockID; b < endBlockID; ++b)
  {
    dataArray.BlockCounts[b] = blocksInfo[b].Count;
  }
}

} // end namespace schema
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_SCHEMA_VTK_VTXxmlVTU_txx */
