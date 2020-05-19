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

#ifndef IOSS_Ioss_DBUsage_h
#define IOSS_Ioss_DBUsage_h

#include "vtk_ioss_mangle.h"

namespace Ioss {

  /** \brief Specifies how an Ioss::DatabaseIO object will be used.
   */
  enum DatabaseUsage {
    WRITE_RESTART = 1, /**< The current state of the simulation, including model and field data
                            will be written to this database. */
    READ_RESTART = 2,  /**< A previous state of the simulation, including model and field data
                            will be read from this database. */
    WRITE_RESULTS = 4, /**< Mesh-based results data, such as nodal displacements, will be written
                            to this database. */
    READ_MODEL = 8,    /**< Model data such as node coordinates, element connectivities will be read
                            from this database. */
    WRITE_HISTORY =
        16, /**< Global results data, such as total energy, will be written to this database. */
    WRITE_HEARTBEAT =
        32 /**< Text-based results data for particular nodes, edges, faces, elements,
                or global variables at particular times will be written to this database. */
  };

  enum IfDatabaseExistsBehavior {
    DB_OVERWRITE,
    DB_APPEND,
    DB_APPEND_GROUP,
    DB_ABORT,
    DB_ADD_SUFFIX,
    DB_ADD_SUFFIX_OVERWRITE
  }; // Used if topology change in DB_OVERWRITE mode

  inline bool is_input_event(Ioss::DatabaseUsage db_usage)
  {
    return db_usage == Ioss::READ_MODEL || db_usage == Ioss::READ_RESTART;
  }
} // namespace Ioss
#endif // IOSS_Ioss_DBUsage_h
