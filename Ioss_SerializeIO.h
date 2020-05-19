// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef IOSS_Ioss_SerializeIO_h
#define IOSS_Ioss_SerializeIO_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>

namespace Ioss {
  class DatabaseIO;
} // namespace Ioss

namespace Ioss {

  /**
   * @brief Class <code>SerializeIO</code> is a sentry class which performs serialization
   * for mesh database I/O.
   *
   * This sentry guards serialization of parallel I/O routines.  At construction, it
   * blocks the processes via an MPI barrier, releasing them to execute in groups specified
   * by <code>s_groupSize</code>.  At destruction, it continues to block via MPI barriers
   * until all the processor have been released by the constructor.
   *
   * In the case where the constructor is called, and the sentry is already active and owned
   * by the processes group, the constructor and destructor simply fall through since the
   * serialization is already in place at a higher level.
   *
   * \note All ranks must call the SerializeIO constructor synchronously.
   * \note It is recommended to use RAII and keep the area protected by the SerializeIO as small as
   * possible.
   *
   * The flow is that the ranks are split into groups of the specified size. Assume 3 ranks of group
   * size 1.
   *
   * * First time through,
   *   - rank 0 falls through and
   *   - ranks 1, 2 sit at the barrier
   *   - rank 0 hits the destructor and then all 3 ranks are in the barrier so they all go to next
   * step
   *   - (rank 1,2 in constructor, rank 0 in destructor)
   * * `s_owner` is now equal to `m_groupRank` on rank 1, so it falls out of the do while;
   *   - rank 2 still in the constructor do while Barrier
   *   - rank 0 in the destructor do while Barrier
   *   - rank 1 does its work and calls destructor;
   *   - all ranks in Barrier, so they go to next step.
   * * `s_owner` now equal to `m_groupRank` on rank 2, so if falls out of the do while;
   *   - ranks 0,1 in destructor do while at the Barrier
   *   - rank 2 does it work and calls destructor
   *   - all ranks are now in the destructor Barrier, so they go to next step
   *   - all ranks clear the Destructor and go to next step.
   *
   */
  class SerializeIO
  {
  public:
    /**
     * Creates a new <code>SerializeIO</code> instance.
     *
     * @param database_io       a <code>DatabaseIO</code> variable ...
     */
    explicit SerializeIO(const DatabaseIO *database_io);
    SerializeIO(const SerializeIO &from) = delete;
    SerializeIO &operator=(const SerializeIO &from) = delete;
    ~SerializeIO();

    inline static int getOwner() { return s_owner; }

    inline static int getRank() { return s_rank; }

    inline static int getSize() { return s_size; }

    inline static int getGroupRank() { return s_groupRank; }

    inline static int getGroupSize() { return s_groupSize; }

    static void setGroupFactor(int factor);

    inline static bool isEnabled() { return s_groupFactor != 0; }

    inline static bool inBarrier() { return s_owner != -1; }

    inline static bool inMyGroup() { return s_owner == s_groupRank; }

  private:
    const DatabaseIO *m_databaseIO; ///< Database I/O pointer
#if defined(IOSS_THREADSAFE)
    static std::mutex m_;
#endif
    bool m_activeFallThru; ///< No barriers since my group is running

    static int s_groupFactor; ///< Grouping factor
    static int s_size;        ///< Number of processors
    static int s_rank;        ///< My processor rank
    static int s_groupSize;   ///< Number of groups
    static int s_groupRank;   ///< My group rank
    static int s_owner;       ///< Group currently running
  };

} // namespace Ioss

#endif // IOSS_Ioss_SerializeIO_h
