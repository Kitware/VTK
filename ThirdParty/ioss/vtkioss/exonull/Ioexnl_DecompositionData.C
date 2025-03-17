// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"
#include "exonull/Ioexnl_DecompositionData.h"
#if defined PARALLEL_AWARE_EXODUS
#include "Ioss_Field.h"           // for Field, etc
#include "Ioss_Map.h"             // for Map, MapContainer
#include "Ioss_PropertyManager.h" // for PropertyManager
#include "Ioss_Utils.h"
#include "exonull/Ioexnl_Utils.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <utility>

namespace Ioexnl {
  template DecompositionData<int>::DecompositionData(const Ioss::PropertyManager &props,
                                                     Ioss_MPI_Comm                communicator);
  template DecompositionData<int64_t>::DecompositionData(const Ioss::PropertyManager &props,
                                                         Ioss_MPI_Comm                communicator);

  template <typename INT>
  DecompositionData<INT>::DecompositionData(const Ioss::PropertyManager &props,
                                            Ioss_MPI_Comm                communicator)
      : DecompositionDataBase(communicator), m_decomposition(props, communicator)
  {
    Ioss::ParallelUtils pu(comm_);
    m_processor      = pu.parallel_rank();
    m_processorCount = pu.parallel_size();
  }

  template void DecompositionData<int>::create_implicit_global_map(
      const std::vector<int> &owning_proc, std::vector<int64_t> &global_implicit_map,
      Ioss::Map &node_map, int64_t *locally_owned_count, int64_t *processor_offset);
  template void DecompositionData<int64_t>::create_implicit_global_map(
      const std::vector<int> &owning_proc, std::vector<int64_t> &global_implicit_map,
      Ioss::Map &node_map, int64_t *locally_owned_count, int64_t *processor_offset);

  template <typename INT>
  void DecompositionData<INT>::create_implicit_global_map(const std::vector<int> &owning_proc,
                                                          std::vector<int64_t> &global_implicit_map,
                                                          Ioss::Map            &node_map,
                                                          int64_t              *locally_owned_count,
                                                          int64_t              *processor_offset)
  {
    m_decomposition.show_progress(__func__);
    // Used on composed output database...
    // If the node is locally owned, then its position is basically
    // determined by removing all shared nodes from the list and
    // then compressing the list. This location plus the proc_offset
    // gives its location in the global-implicit file.
    //
    // If it is shared, then need to communicate with the owning
    // processor to determine where that processor is putting it.
    //
    // First, iterate nodeOwningProcessor list and set the implicit
    // map for all locally-owned nodes and also determine how many
    // of my nodes are owned by which other processors.

    global_implicit_map.resize(owning_proc.size());

    std::vector<int64_t> snd_count(m_processorCount);
    std::vector<int64_t> rcv_count(m_processorCount);

    size_t position = 0;
    for (size_t i = 0; i < global_implicit_map.size(); i++) {
      snd_count[owning_proc[i]]++;
      if (owning_proc[i] == m_processor) {
        global_implicit_map[i] = position++;
      }
    }
    snd_count[m_processor] = 0;

    // The number of locally-owned nodes on this processor is 'position'
    *locally_owned_count = position;

    MPI_Allgather(locally_owned_count, 1, MPI_LONG_LONG_INT, Data(rcv_count), 1, MPI_LONG_LONG_INT,
                  comm_);
    m_decomposition.show_progress("\tAllgather finished");

    // Determine the offset of the nodes on this processor. The offset is the
    // total number of locally-owned nodes on all processors prior to this processor.
    *processor_offset = 0;
    for (int i = 0; i < m_processor; i++) {
      *processor_offset += rcv_count[i];
    }

    for (auto &i : global_implicit_map) {
      i += *processor_offset + 1;
    }

    // Now, tell the other processors how many nodes I will be sending
    // them (Nodes they own that I share with them)
    MPI_Alltoall(Data(snd_count), 1, MPI_LONG_LONG_INT, Data(rcv_count), 1, MPI_LONG_LONG_INT,
                 comm_);
    m_decomposition.show_progress("\tCommunication 1 finished");

    std::vector<int64_t> snd_offset(snd_count);
    Ioss::Utils::generate_index(snd_offset);
    std::vector<int64_t> snd_list(*snd_offset.rbegin() + *snd_count.rbegin());

    {
      std::vector<int64_t> tmp_disp(snd_offset);
      // Now create the list of nodes to send...
      for (size_t i = 0; i < global_implicit_map.size(); i++) {
        if (owning_proc[i] != m_processor) {
          int64_t global_id                    = node_map.map()[i + 1];
          snd_list[tmp_disp[owning_proc[i]]++] = global_id;
        }
      }
    }

    std::vector<int64_t> rcv_offset(rcv_count);
    Ioss::Utils::generate_index(rcv_offset);
    std::vector<int64_t> rcv_list(*rcv_offset.rbegin() + *rcv_count.rbegin());

    Ioss::MY_Alltoallv(snd_list, snd_count, snd_offset, rcv_list, rcv_count, rcv_offset, comm_);
    m_decomposition.show_progress("\tCommunication 2 finished");

    // Iterate rcv_list and convert global ids to the global-implicit position...
    for (auto &i : rcv_list) {
      int64_t local_id     = node_map.global_to_local(i) - 1;
      int64_t rcv_position = global_implicit_map[local_id];
      i                    = rcv_position;
    }

    // Send the data back now...
    Ioss::MY_Alltoallv(rcv_list, rcv_count, rcv_offset, snd_list, snd_count, snd_offset, comm_);
    m_decomposition.show_progress("\tCommunication 3 finished");

    // Fill in the remaining portions of the global_implicit_map...
    std::vector<int64_t> tmp_disp(snd_offset);
    for (size_t i = 0; i < global_implicit_map.size(); i++) {
      if (owning_proc[i] != m_processor) {
        int64_t implicit       = snd_list[tmp_disp[owning_proc[i]]++];
        global_implicit_map[i] = implicit;
      }
    }
  }
} // namespace Ioexnl
#else
IOSS_MAYBE_UNUSED const char ioss_exodus_decomposition_data_unused_symbol_dummy = '\0';
#endif
