// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

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
    DB_MODIFY,
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
