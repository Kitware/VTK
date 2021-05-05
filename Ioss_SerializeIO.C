// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details
#include "Ioss_CodeTypes.h"     // for SEACAS_HAVE_MPI
#include <Ioss_DatabaseIO.h>    // for DatabaseIO
#include <Ioss_ParallelUtils.h> // for ParallelUtils
#include <Ioss_SerializeIO.h>
#include <Ioss_Utils.h> // for IOSS_ERROR, Ioss::WARNING()
#include <fmt/ostream.h>
#include <ostream> // for operator<<, etc
#include <string>  // for char_traits

namespace Ioss {

  int SerializeIO::s_owner       = -1;
  int SerializeIO::s_rank        = -1;
  int SerializeIO::s_size        = -1;
  int SerializeIO::s_groupSize   = -1;
  int SerializeIO::s_groupRank   = -1;
  int SerializeIO::s_groupFactor = 0;

#if defined(IOSS_THREADSAFE)
  std::mutex SerializeIO::m_;
#endif

  SerializeIO::SerializeIO(const DatabaseIO *database_io)
      : m_databaseIO(database_io), m_activeFallThru(true)

  {
    if (m_databaseIO->using_parallel_io()) {
      return;
    }
    IOSS_FUNC_ENTER(m_);

    const Ioss::ParallelUtils util = m_databaseIO->util();
    if (s_rank == -1) {
      s_rank = util.parallel_rank();
      s_size = util.parallel_size();
      if (s_groupFactor != 0) {
        s_groupRank = s_rank / s_groupFactor;
        s_groupSize = (s_size - 1) / s_groupFactor + 1;
      }
    }

    m_activeFallThru = s_owner != -1;
    if (!m_activeFallThru) {
      if (s_groupFactor > 0) {
        do {
          util.barrier();
        } while (++s_owner != s_groupRank);
        m_databaseIO->openDatabase__();
      }
      else {
        s_owner = s_groupRank;
      }
    }
  }

  SerializeIO::~SerializeIO()
  {
    if (m_databaseIO->using_parallel_io()) {
      return;
    }
    try {
      IOSS_FUNC_ENTER(m_);
      if (!m_activeFallThru) {
        if (s_groupFactor > 0) {
          m_databaseIO->closeDatabase__();
          s_owner                        = s_groupRank;
          const Ioss::ParallelUtils util = m_databaseIO->util();
          do {
            util.barrier();
          } while (++s_owner != s_groupSize);
          s_owner = -1;
        }
        else {
          s_owner = -1;
        }
      }
    }
    catch (...) {
    }
  }

  void SerializeIO::setGroupFactor(int factor)
  {
    IOSS_FUNC_ENTER(m_);
    if (s_rank != -1) {
      fmt::print(Ioss::WARNING(), "Mesh I/O serialization group factor cannot be changed "
                                  "once serialized I/O has begun");
    }
    else {
      s_groupFactor = factor;
    }
  }

} // namespace Ioss
