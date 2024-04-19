// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDIYExplicitAssigner
 * @brief assigner for use with DIY
 *
 * vtkDIYExplicitAssigner is a diy::StaticAssigner specialization that be used
 * where the block assignment is not strictly round-robin or contiguous which
 * assumes blocks equally split among ranks. This supports the case where each
 * rank has arbitrary number of blocks per rank. The constructor is provided the
 * mpi communicator and the number of local blocks. It performs parallel
 * communication to exchange information about blocks with all participating
 * ranks.
 *
 * vtkDIYExplicitAssigner also supports ability to pad each rank such that the
 * total number of blocks across all ranks is a power of two.
 */

#ifndef vtkDIYExplicitAssigner_h
#define vtkDIYExplicitAssigner_h

#include "vtkObject.h"
#include "vtkParallelDIYModule.h" // for export macros
// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

#ifdef _WIN32
#pragma warning(disable : 4275) /* non dll-interface class `diy::StaticAssigner` used as base for  \
                                   dll-interface class */
#endif

VTK_ABI_NAMESPACE_BEGIN
class VTKPARALLELDIY_EXPORT vtkDIYExplicitAssigner : public diy::StaticAssigner
{
public:
  vtkDIYExplicitAssigner(
    diy::mpi::communicator comm, int local_blocks, bool force_power_of_two = false);

  int rank(int gid) const override;
  void local_gids(int rank, std::vector<int>& gids) const override;

private:
  std::vector<int> IScanBlockCounts;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkDIYExplicitAssigner.h
